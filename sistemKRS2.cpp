#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <map>
#include <sstream>
#include <iomanip>
#include <limits>
#include <ctime>
#include <fstream>
using namespace std;

// ANSI color codes for terminal
const string RESET = "\033[0m";
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string BLUE = "\033[34m";
const string MAGENTA = "\033[35m";
const string CYAN = "\033[36m";
const string BOLD = "\033[1m";

struct Aksi {
    string kodeMK;
    bool tambah;
    time_t waktu;
};

struct MataKuliah {
    string kode;
    string nama;
    int sks;
    int kuota_max;
    int kuota_terisi = 0;
    vector<string> prasyarat; // Kode MK prasyarat
    map<string, char> nilaiMutu; // NIM -> Nilai
};

struct RiwayatNilai {
    string kodeMK;
    string namaMK;
    int sks;
    char nilai;
    string semester;
};

struct Mahasiswa {
    string nim;
    string nama;
    string password_hash;
    string prodi;
    int semester;
    vector<string> krs;
    stack<Aksi> riwayat;
    vector<RiwayatNilai> riwayatNilai;
    int max_sks = 24; // Default max SKS
};

unordered_map<string, Mahasiswa> akun;
unordered_map<string, MataKuliah> matkul;
Mahasiswa* sesi = nullptr;
bool isAdmin = false;

// Fungsi untuk membersihkan layar
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Fungsi untuk menunggu input sebelum melanjutkan
void pressEnterToContinue() {
    cout << "\n" << YELLOW << "Tekan Enter untuk melanjutkan..." << RESET;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getchar();
}

string getCurrentDateTime() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    
    ostringstream oss;
    oss << setfill('0') 
        << setw(2) << ltm->tm_mday << "/"
        << setw(2) << 1 + ltm->tm_mon << "/"
        << 1900 + ltm->tm_year << " "
        << setw(2) << ltm->tm_hour << ":"
        << setw(2) << ltm->tm_min;
    
    return oss.str();
}

string formatTimeStamp(time_t t) {
    tm* ltm = localtime(&t);
    
    ostringstream oss;
    oss << setfill('0') 
        << setw(2) << ltm->tm_mday << "/"
        << setw(2) << 1 + ltm->tm_mon << "/"
        << 1900 + ltm->tm_year << " "
        << setw(2) << ltm->tm_hour << ":"
        << setw(2) << ltm->tm_min;
    
    return oss.str();
}

string hashPassword(const string& pw) {
    hash<string> hasher;
    return to_string(hasher(pw));
}

float nilaiMutuToAngka(char nilai) {
    switch(nilai) {
        case 'A': return 4.0;
        case 'B': return 3.0;
        case 'C': return 2.0;
        case 'D': return 1.0;
        case 'E': return 0.0;
        default: return 0.0;
    }
}

// Validasi input numerik
template <typename T>
T getNumericInput(const string& prompt) {
    T value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        } else {
            cout << RED << "Error: Masukan harus berupa angka." << RESET << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

void printHeader() {
    clearScreen();
    cout << BOLD << BLUE << "----------------------------------------------------------------" << RESET << endl;
    cout << BOLD << BLUE << "|                                                              |" << RESET << endl;
    cout << BOLD << BLUE << "|             SISTEM INFORMASI AKADEMIK UNIVERSITAS            |" << RESET << endl;
    cout << BOLD << BLUE << "|                   PENGISIAN KARTU RENCANA STUDI              |" << RESET << endl;
    cout << BOLD << BLUE << "|                                                              |" << RESET << endl;
    cout << BOLD << BLUE << "----------------------------------------------------------------" << RESET << endl;
    
    if (sesi) {
        cout << CYAN << "Mahasiswa: " << sesi->nama << " (" << sesi->nim << ")" << RESET << endl;
        cout << CYAN << "Program Studi: " << sesi->prodi << " - Semester " << sesi->semester << RESET << endl;
        cout << CYAN << "Tanggal: " << getCurrentDateTime() << RESET << endl;
        cout << CYAN << "----------------------------------------------------------------" << RESET << endl;
    }
}

void saveDataToFile() {
    ofstream outMhs("mahasiswa.dat");
    for (const auto& pair : akun) {
        const Mahasiswa& mhs = pair.second;
        outMhs << mhs.nim << "|" << mhs.nama << "|" << mhs.password_hash << "|" 
               << mhs.prodi << "|" << mhs.semester << "|" << mhs.max_sks << endl;
    }
    outMhs.close();
    
    ofstream outMatkul("matakuliah.dat");
    for (const auto& pair : matkul) {
        const MataKuliah& mk = pair.second;
        outMatkul << mk.kode << "|" << mk.nama << "|" << mk.sks << "|" 
                  << mk.kuota_max << "|" << mk.kuota_terisi;
        
        // Save prerequisites
        outMatkul << "|";
        for (size_t i = 0; i < mk.prasyarat.size(); i++) {
            outMatkul << mk.prasyarat[i];
            if (i < mk.prasyarat.size() - 1) outMatkul << ",";
        }
        
        outMatkul << endl;
    }
    outMatkul.close();
    
    // Save KRS and grades separately
    ofstream outKRS("krs.dat");
    for (const auto& pair : akun) {
        const Mahasiswa& mhs = pair.second;
        outKRS << mhs.nim;
        for (const string& kode : mhs.krs) {
            outKRS << "|" << kode;
        }
        outKRS << endl;
    }
    outKRS.close();
    
    ofstream outNilai("nilai.dat");
    for (const auto& pair : akun) {
        const Mahasiswa& mhs = pair.second;
        for (const RiwayatNilai& nilai : mhs.riwayatNilai) {
            outNilai << mhs.nim << "|" << nilai.kodeMK << "|" << nilai.nilai 
                    << "|" << nilai.semester << endl;
        }
    }
    outNilai.close();
}

void daftarMataKuliah() {
    // Mata kuliah semester 1
    matkul["IF101"] = {"IF101", "Algoritma dan Pemrograman", 3, 30};
    matkul["IF102"] = {"IF102", "Kalkulus I", 3, 25};
    matkul["IF103"] = {"IF103", "Pengantar Teknologi Informasi", 2, 20};
    matkul["IF104"] = {"IF104", "Fisika Dasar", 3, 20};
    matkul["IF105"] = {"IF105", "Bahasa Inggris", 2, 25};
    
    // Mata kuliah semester 2
    matkul["IF201"] = {"IF201", "Struktur Data", 3, 30};
    matkul["IF202"] = {"IF202", "Kalkulus II", 3, 25};
    matkul["IF203"] = {"IF203", "Sistem Digital", 3, 20};
    matkul["IF204"] = {"IF204", "Aljabar Linear", 3, 20};
    matkul["IF205"] = {"IF205", "Rekayasa Perangkat Lunak", 3, 25};
    
    // Mata kuliah semester 3
    matkul["IF301"] = {"IF301", "Basis Data", 3, 30};
    matkul["IF302"] = {"IF302", "Jaringan Komputer", 3, 25};
    matkul["IF303"] = {"IF303", "Pemrograman Berbasis Objek", 3, 20};
    matkul["IF304"] = {"IF304", "Sistem Operasi", 3, 20};
    matkul["IF305"] = {"IF305", "Metode Numerik", 3, 25};
    
    // Mata kuliah semester 4
    matkul["IF401"] = {"IF401", "Pemrograman Web", 3, 30};
    matkul["IF402"] = {"IF402", "Grafika Komputer", 3, 25};
    matkul["IF403"] = {"IF403", "Kecerdasan Buatan", 3, 20};
    matkul["IF404"] = {"IF404", "Interaksi Manusia dan Komputer", 3, 20};
    matkul["IF405"] = {"IF405", "Statistika", 3, 25};
    
    // Set prerequisites
    matkul["IF201"].prasyarat.push_back("IF101");
    matkul["IF202"].prasyarat.push_back("IF102");
    matkul["IF301"].prasyarat.push_back("IF201");
    matkul["IF302"].prasyarat.push_back("IF203");
    matkul["IF303"].prasyarat.push_back("IF101");
    matkul["IF401"].prasyarat.push_back("IF301");
    matkul["IF402"].prasyarat.push_back("IF204");
    matkul["IF403"].prasyarat.push_back("IF303");
}

void loadDataFromFile() {
    // Try to load files, if they exist
    ifstream inMhs("mahasiswa.dat");
    if (inMhs.is_open()) {
        string line;
        while (getline(inMhs, line)) {
            stringstream ss(line);
            string nim, nama, hash, prodi, semStr, maxSksStr;
            
            getline(ss, nim, '|');
            getline(ss, nama, '|');
            getline(ss, hash, '|');
            getline(ss, prodi, '|');
            getline(ss, semStr, '|');
            getline(ss, maxSksStr, '|');
            
            Mahasiswa mhs;
            mhs.nim = nim;
            mhs.nama = nama;
            mhs.password_hash = hash;
            mhs.prodi = prodi;
            mhs.semester = stoi(semStr);
            mhs.max_sks = stoi(maxSksStr);
            
            akun[nim] = mhs;
        }
        inMhs.close();
    }
    
    // Add admin account if not exists
    if (akun.find("admin") == akun.end()) {
        Mahasiswa admin;
        admin.nim = "admin";
        admin.nama = "Administrator";
        admin.password_hash = hashPassword("admin123");
        admin.prodi = "Admin";
        admin.semester = 0;
        akun["admin"] = admin;
    }
    
    // Load mata kuliah
    ifstream inMatkul("matakuliah.dat");
    if (inMatkul.is_open()) {
        string line;
        while (getline(inMatkul, line)) {
            stringstream ss(line);
            string kode, nama, sksStr, maxStr, terisiStr, prasyaratStr;
            
            getline(ss, kode, '|');
            getline(ss, nama, '|');
            getline(ss, sksStr, '|');
            getline(ss, maxStr, '|');
            getline(ss, terisiStr, '|');
            getline(ss, prasyaratStr);
            
            MataKuliah mk;
            mk.kode = kode;
            mk.nama = nama;
            mk.sks = stoi(sksStr);
            mk.kuota_max = stoi(maxStr);
            mk.kuota_terisi = stoi(terisiStr);
            
            // Parse prerequisites
            stringstream prasyaratSS(prasyaratStr);
            string prasyarat;
            while (getline(prasyaratSS, prasyarat, ',')) {
                if (!prasyarat.empty()) {
                    mk.prasyarat.push_back(prasyarat);
                }
            }
            
            matkul[kode] = mk;
        }
        inMatkul.close();
    } else {
        // Jika file tidak ada, buat data default
        daftarMataKuliah();
    }
    
    // Load KRS
    ifstream inKRS("krs.dat");
    if (inKRS.is_open()) {
        string line;
        while (getline(inKRS, line)) {
            stringstream ss(line);
            string nim, kode;
            
            getline(ss, nim, '|');
            while (getline(ss, kode, '|')) {
                akun[nim].krs.push_back(kode);
            }
        }
        inKRS.close();
    }
    
    // Load Nilai
    ifstream inNilai("nilai.dat");
    if (inNilai.is_open()) {
        string line;
        while (getline(inNilai, line)) {
            stringstream ss(line);
            string nim, kodeMK, nilaiStr, semester;
            
            getline(ss, nim, '|');
            getline(ss, kodeMK, '|');
            getline(ss, nilaiStr, '|');
            getline(ss, semester);
            
            if (akun.find(nim) != akun.end() && matkul.find(kodeMK) != matkul.end()) {
                char nilai = nilaiStr[0];
                matkul[kodeMK].nilaiMutu[nim] = nilai;
                
                RiwayatNilai rn;
                rn.kodeMK = kodeMK;
                rn.namaMK = matkul[kodeMK].nama;
                rn.sks = matkul[kodeMK].sks;
                rn.nilai = nilai;
                rn.semester = semester;
                
                akun[nim].riwayatNilai.push_back(rn);
            }
        }
        inNilai.close();
    }
}

void menuUtama();

void registerAkun() {
    printHeader();
    cout << BOLD << CYAN << "== REGISTRASI AKUN MAHASISWA ==" << RESET << endl;
    
    string nim, nama, pass, prodi;
    int semester;
    
    cout << "Masukkan NIM: "; cin >> nim;
    if (akun.find(nim) != akun.end()) {
        cout << RED << "Akun dengan NIM tersebut sudah terdaftar." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    cin.ignore();
    cout << "Masukkan Nama Lengkap: "; getline(cin, nama);
    cout << "Masukkan Program Studi: "; getline(cin, prodi);
    
    semester = getNumericInput<int>("Masukkan Semester (1-8): ");
    if (semester < 1 || semester > 8) {
        cout << RED << "Semester harus antara 1-8." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    cout << "Masukkan Password: "; getline(cin, pass);
    if (pass.length() < 6) {
        cout << RED << "Password minimal 6 karakter." << RESET << endl;
        pressEnterToContinue();
        return;
    }

    Mahasiswa mhs;
    mhs.nim = nim;
    mhs.nama = nama;
    mhs.password_hash = hashPassword(pass);
    mhs.prodi = prodi;
    mhs.semester = semester;
    
    // Set max SKS berdasarkan semester
    if (semester <= 2) mhs.max_sks = 20;
    else mhs.max_sks = 24;
    
    akun[nim] = mhs;
    saveDataToFile();

    cout << GREEN << "Registrasi berhasil. Silakan login." << RESET << endl;
    pressEnterToContinue();
}

void login() {
    printHeader();
    cout << BOLD << CYAN << "== LOGIN ==" << RESET << endl;
    
    string nim, pass;
    cout << "NIM: "; cin >> nim;
    cin.ignore();
    cout << "Password: "; getline(cin, pass);

    unordered_map<string, Mahasiswa>::iterator it = akun.find(nim);
    if (it != akun.end() && it->second.password_hash == hashPassword(pass)) {
        sesi = &it->second;
        isAdmin = (nim == "admin");
        cout << GREEN << "Login berhasil. Selamat datang, " << sesi->nama << "!" << RESET << endl;
        pressEnterToContinue();
        menuUtama();
    } else {
        cout << RED << "Login gagal. NIM atau password salah." << RESET << endl;
        pressEnterToContinue();
    }
}

bool cekPrasyarat(const string& kodeMK) {
    const MataKuliah& mk = matkul[kodeMK];
    
    if (mk.prasyarat.empty()) return true;
    
    for (const string& prasyarat : mk.prasyarat) {
        bool lulus = false;
        for (const RiwayatNilai& nilai : sesi->riwayatNilai) {
            if (nilai.kodeMK == prasyarat && nilai.nilai != 'E' && nilai.nilai != 'D') {
                lulus = true;
                break;
            }
        }
        if (!lulus) {
            cout << RED << "Prasyarat " << prasyarat << " (" << matkul[prasyarat].nama << ") belum terpenuhi." << RESET << endl;
            return false;
        }
    }
    return true;
}

int hitungTotalSKS() {
    int total = 0;
    for (const string& kode : sesi->krs) {
        total += matkul[kode].sks;
    }
    return total;
}

void tambahMK() {
    printHeader();
    cout << BOLD << CYAN << "== TAMBAH MATA KULIAH ==" << RESET << endl;
    
    // Tampilkan daftar mata kuliah yang tersedia dan belum diambil
    cout << YELLOW << "Daftar Mata Kuliah yang Tersedia:" << RESET << endl;
    cout << "┌───────┬─────────────────────────────────┬─────┬────────┬─────────┐" << endl;
    cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │ Kuota  │ Status  │" << endl;
    cout << "├───────┼─────────────────────────────────┼─────┼────────┼─────────┤" << endl;
    
    vector<MataKuliah*> tersedia;
    for (auto& pair : matkul) {
        MataKuliah& mk = pair.second;
        if (find(sesi->krs.begin(), sesi->krs.end(), mk.kode) == sesi->krs.end()) {
            tersedia.push_back(&mk);
        }
    }
    
    sort(tersedia.begin(), tersedia.end(), [](MataKuliah* a, MataKuliah* b) {
        return a->kode < b->kode;
    });
    
    for (MataKuliah* mk : tersedia) {
        string status;
        if (mk->kuota_terisi >= mk->kuota_max) {
            status = RED + "PENUH" + RESET;
        } else {
            status = GREEN + "TERSEDIA" + RESET;
        }
        
        cout << "│ " << setw(5) << left << mk->kode 
             << "│ " << setw(31) << left << mk->nama 
             << "│ " << setw(3) << right << mk->sks 
             << " │ " << setw(3) << right << mk->kuota_terisi << "/" << setw(2) << left << mk->kuota_max 
             << " │ " << setw(7) << left << status << " │" << endl;
    }
    cout << "└───────┴─────────────────────────────────┴─────┴────────┴─────────┘" << endl;
    
    // Total SKS saat ini
    int totalSKS = hitungTotalSKS();
    cout << "Total SKS saat ini: " << totalSKS << "/" << sesi->max_sks << endl;
    
    string kode;
    cout << "\nMasukkan Kode MK (atau 0 untuk batal): "; cin >> kode;
    
    if (kode == "0") {
        return;
    }
    
    // Validasi input
    if (matkul.find(kode) == matkul.end()) {
        cout << RED << "Kode mata kuliah tidak valid." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    if (find(sesi->krs.begin(), sesi->krs.end(), kode) != sesi->krs.end()) {
        cout << RED << "Mata kuliah sudah terdaftar dalam KRS Anda." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    if (matkul[kode].kuota_terisi >= matkul[kode].kuota_max) {
        cout << RED << "Kuota mata kuliah penuh." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    // Validasi total SKS
    if (totalSKS + matkul[kode].sks > sesi->max_sks) {
        cout << RED << "Total SKS akan melebihi batas maksimum (" << sesi->max_sks << ")." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    // Validasi prasyarat
    if (!cekPrasyarat(kode)) {
        pressEnterToContinue();
        return;
    }

    // Tambahkan mata kuliah ke KRS
    sesi->krs.push_back(kode);
    matkul[kode].kuota_terisi++;
    
    // Simpan ke riwayat untuk undo
    Aksi aksi;
    aksi.kodeMK = kode;
    aksi.tambah = true;
    aksi.waktu = time(0);
    sesi->riwayat.push(aksi);
    
    saveDataToFile();
    
    cout << GREEN << "Mata kuliah " << kode << " - " << matkul[kode].nama 
         << " berhasil ditambahkan ke KRS." << RESET << endl;
    pressEnterToContinue();
}

void batalMK() {
    printHeader();
    cout << BOLD << CYAN << "== BATAL MATA KULIAH ==" << RESET << endl;
    
    if (sesi->krs.empty()) {
        cout << YELLOW << "Belum ada mata kuliah yang terdaftar dalam KRS." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    // Tampilkan daftar mata kuliah yang sudah diambil
    cout << "Daftar Mata Kuliah dalam KRS:" << endl;
    cout << "┌───────┬─────────────────────────────────┬─────┐" << endl;
    cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │" << endl;
    cout << "├───────┼─────────────────────────────────┼─────┤" << endl;
    
    for (const string& kode : sesi->krs) {
        cout << "│ " << setw(5) << left << kode 
             << "│ " << setw(31) << left << matkul[kode].nama 
             << "│ " << setw(3) << right << matkul[kode].sks << " │" << endl;
    }
    cout << "└───────┴─────────────────────────────────┴─────┘" << endl;
    
    // Total SKS
    int totalSKS = hitungTotalSKS();
    cout << "Total SKS: " << totalSKS << endl;
    
    string kode;
    cout << "\nMasukkan Kode MK yang ingin dibatalkan (atau 0 untuk batal): "; cin >> kode;
    
    if (kode == "0") {
        return;
    }
    
    vector<string>::iterator it = find(sesi->krs.begin(), sesi->krs.end(), kode);
    if (it == sesi->krs.end()) {
        cout << RED << "Mata kuliah tidak terdaftar dalam KRS Anda." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    // Hapus mata kuliah dari KRS
    sesi->krs.erase(it);
    matkul[kode].kuota_terisi--;
    
    // Simpan ke riwayat untuk undo
    Aksi aksi;
    aksi.kodeMK = kode;
    aksi.tambah = false;
    aksi.waktu = time(0);
    sesi->riwayat.push(aksi);
    
    saveDataToFile();
    
    cout << GREEN << "Mata kuliah " << kode << " - " << matkul[kode].nama 
         << " berhasil dibatalkan dari KRS." << RESET << endl;
    pressEnterToContinue();
}

void undo() {
    printHeader();
    cout << BOLD << CYAN << "== UNDO AKSI TERAKHIR ==" << RESET << endl;
    
    if (sesi->riwayat.empty()) {
        cout << YELLOW << "Tidak ada aksi yang dapat dibatalkan." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    Aksi aksi = sesi->riwayat.top(); 
    sesi->riwayat.pop();
    
    string waktuStr = formatTimeStamp(aksi.waktu);
    
    if (aksi.tambah) {
        vector<string>::iterator it = find(sesi->krs.begin(), sesi->krs.end(), aksi.kodeMK);
        if (it != sesi->krs.end()) {
            sesi->krs.erase(it);
            matkul[aksi.kodeMK].kuota_terisi--;
            
            cout << GREEN << "Undo berhasil: Pembatalan penambahan mata kuliah " 
                 << aksi.kodeMK << " - " << matkul[aksi.kodeMK].nama 
                 << " (dilakukan pada " << waktuStr << ")" << RESET << endl;
        }
    } else {
        sesi->krs.push_back(aksi.kodeMK);
        matkul[aksi.kodeMK].kuota_terisi++;
        
        cout << GREEN << "Undo berhasil: Pembatalan penghapusan mata kuliah " 
             << aksi.kodeMK << " - " << matkul[aksi.kodeMK].nama 
             << " (dilakukan pada " << waktuStr << ")" << RESET << endl;
    }
    
    saveDataToFile();
    pressEnterToContinue();
}

void lihatKRS() {
    printHeader();
    cout << BOLD << CYAN << "== KARTU RENCANA STUDI (KRS) ==" << RESET << endl;
    
    if (sesi->krs.empty()) {
        cout << YELLOW << "Belum ada mata kuliah yang terdaftar dalam KRS." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    // Tampilkan KRS dalam bentuk tabel
    cout << "┌───────┬─────────────────────────────────┬─────┬────────┐" << endl;
    cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │ Status │" << endl;
    cout << "├───────┼─────────────────────────────────┼─────┼────────┤" << endl;
    
    int totalSKS = 0;
    for (const string& kode : sesi->krs) {
        MataKuliah& mk = matkul[kode];
        totalSKS += mk.sks;
        
        cout << "│ " << setw(5) << left << kode 
             << "│ " << setw(31) << left << mk.nama
             << "│ " << setw(3) << right << mk.sks << " │ " 
             << setw(6) << left << "Aktif" << " │" << endl;
    }
    cout << "└───────┴─────────────────────────────────┴─────┴────────┘" << endl;
    
    cout << "Total SKS: " << totalSKS << "/" << sesi->max_sks << endl;
    
    // Tampilkan jadwal kuliah (simulasi)
    cout << "\nJadwal Kuliah:" << endl;
    cout << "┌─────────────┬───────────┬─────────────────────────────────┐" << endl;
    cout << "│    Hari     │   Waktu   │       Mata Kuliah               │" << endl;
    cout << "├─────────────┼───────────┼─────────────────────────────────┤" << endl;
    
    const char* hari[] = {"Senin", "Selasa", "Rabu", "Kamis", "Jumat"};
    const char* waktu[] = {"08:00-09:40", "10:00-11:40", "13:00-14:40", "15:00-16:40"};
    
    int i = 0;
    for (const string& kode : sesi->krs) {
        cout << "│ " << setw(11) << left << hari[i % 5] 
             << "│ " << setw(9) << left << waktu[(i/5) % 4] 
             << "│ " << setw(5) << left << kode << " - " 
             << setw(25) << left << matkul[kode].nama << " │" << endl;
        i++;
    }
    cout << "└─────────────┴───────────┴─────────────────────────────────┘" << endl;
    
    pressEnterToContinue();
}

void tampilMK(bool byNama = false) {
    printHeader();
    string sortBy = byNama ? "NAMA" : "KODE";
    cout << BOLD << CYAN << "== DAFTAR MATA KULIAH (SORT BY " << sortBy << ") ==" << RESET << endl;
    
    vector<MataKuliah> daftar;
    for (const auto& pair : matkul) {
        daftar.push_back(pair.second);
    }
    
    sort(daftar.begin(), daftar.end(), [=](const MataKuliah& a, const MataKuliah& b) {
        return byNama ? a.nama < b.nama : a.kode < b.kode;
    });
    
    cout << "----------------------------------------------------------------------" << endl;
    cout << "| Kode  | Nama Mata Kuliah                | SKS | Kuota  | Prasyarat |" << endl;
    cout << "----------------------------------------------------------------------" << endl;
    
    for (const MataKuliah& mk : daftar) {
        string prasyarat;
        for (size_t i = 0; i < mk.prasyarat.size(); i++) {
            prasyarat += mk.prasyarat[i];
            if (i < mk.prasyarat.size() - 1) prasyarat += ", ";
        }
        
        cout << "│ " << setw(5) << left << mk.kode 
             << "│ " << setw(31) << left << mk.nama 
             << "│ " << setw(3) << right << mk.sks 
             << " │ " << setw(3) << right << mk.kuota_terisi << "/" << setw(2) << left << mk.kuota_max 
             << " │ " << setw(9) << left << prasyarat << " │" << endl;
    }
    cout << "└───────┴─────────────────────────────────┴─────┴────────┴───────────┘" << endl;
    
    int totalMK = daftar.size();
    int totalSKS = 0;
    for (const MataKuliah& mk : daftar) {
        totalSKS += mk.sks;
    }
    
    cout << "Total mata kuliah: " << totalMK << endl;
    cout << "Total SKS seluruh mata kuliah: " << totalSKS << endl;
    
    pressEnterToContinue();
}

void lihatKuota() {
    printHeader();
    cout << BOLD << CYAN << "== INFORMASI KUOTA MATA KULIAH ==" << RESET << endl;
    
    cout << "----------------------------------------------------------------------" << endl;
    cout << "| Kode  | Nama Mata Kuliah               |    Kuota   |  Persentase  |" << endl;
    cout << "----------------------------------------------------------------------" << endl;
    
    vector<MataKuliah> daftar;
    for (const auto& pair : matkul) {
        daftar.push_back(pair.second);
    }
    
    sort(daftar.begin(), daftar.end(), [](const MataKuliah& a, const MataKuliah& b) {
        // Sort by utilization percentage (descending)
        return (static_cast<float>(a.kuota_terisi) / a.kuota_max) > 
               (static_cast<float>(b.kuota_terisi) / b.kuota_max);
    });
    
    for (const MataKuliah& mk : daftar) {
        float persentase = (static_cast<float>(mk.kuota_terisi) / mk.kuota_max) * 100;
        string status;
        
        if (persentase >= 90) {
            status = RED + to_string(static_cast<int>(persentase)) + "%" + RESET;
        } else if (persentase >= 70) {
            status = YELLOW + to_string(static_cast<int>(persentase)) + "%" + RESET;
        } else {
            status = GREEN + to_string(static_cast<int>(persentase)) + "%" + RESET;
        }
        
        cout << "│ " << setw(5) << left << mk.kode 
             << "│ " << setw(31) << left << mk.nama 
             << "│ " << setw(5) << right << mk.kuota_terisi << "/" << setw(4) << left << mk.kuota_max 
             << " │ " << setw(9) << left << status << " │" << endl;
    }
    cout << "└───────┴─────────────────────────────────┴────────────┴───────────┘" << endl;
    
    pressEnterToContinue();
}

void lihatSebaran() {
    printHeader();
    cout << BOLD << CYAN << "== SEBARAN NILAI MATA KULIAH ==" << RESET << endl;
    
    // Tampilkan daftar mata kuliah
    cout << "Pilih mata kuliah untuk melihat sebaran nilai:" << endl;
    cout << "----------------------------------------------------------------------" << endl;
    cout << "| Kode     | Nama Mata Kuliah                      |   Jumlah Nilai  |" << endl;
    cout << "----------------------------------------------------------------------" << endl;
    
    vector<pair<string, int>> mkWithGrades;
    for (const auto& pair : matkul) {
        if (!pair.second.nilaiMutu.empty()) {
            mkWithGrades.push_back({pair.first, pair.second.nilaiMutu.size()});
        }
    }
    
    sort(mkWithGrades.begin(), mkWithGrades.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
        return a.second > b.second; // Sort by number of grades (descending)
    });
    
    for (const pair<string, int>& mk : mkWithGrades) {
        cout << "│ " << setw(5) << left << mk.first
             << "│ " << setw(31) << left << matkul[mk.first].nama
             << "│ " << setw(11) << right << mk.second << " │" << endl;
    }
    cout << "└───────┴─────────────────────────────────┴─────────────┘" << endl;
    
    string kode;
    cout << "\nMasukkan Kode MK (atau 0 untuk batal): "; cin >> kode;
    
    if (kode == "0") {
        return;
    }
    
    if (matkul.find(kode) == matkul.end()) {
        cout << RED << "Kode mata kuliah tidak valid." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    MataKuliah& mk = matkul[kode];
    
    if (mk.nilaiMutu.empty()) {
        cout << YELLOW << "Belum ada data nilai untuk mata kuliah ini." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    map<char, int> count;
    for (const auto& nilai : mk.nilaiMutu) {
        count[nilai.second]++;
    }
    
    // Hitung statistik
    int total = mk.nilaiMutu.size();
    float rata2 = 0;
    for (const auto& nilai : mk.nilaiMutu) {
        rata2 += nilaiMutuToAngka(nilai.second);
    }
    rata2 /= total;
    
    // Tampilkan sebaran nilai dalam bentuk grafik bar
    cout << "\nSebaran Nilai untuk " << mk.kode << " - " << mk.nama << "\n" << endl;
    cout << "Total Mahasiswa: " << total << endl;
    cout << "Rata-rata Nilai: " << fixed << setprecision(2) << rata2 << endl << endl;
    
    const int maxBarWidth = 40;
    
    cout << "Grafik Sebaran Nilai:" << endl;
    cout << "┌───┬───────────────────────────────────────────┬───────┐" << endl;
    cout << "│ A │ ";
    int barWidth = count['A'] * maxBarWidth / total;
    cout << string(barWidth, '#') << string(maxBarWidth - barWidth, ' ');
    cout << " │ " << setw(5) << right << count['A'] << " │" << endl;
    
    cout << "│ B │ ";
    barWidth = count['B'] * maxBarWidth / total;
    cout << string(barWidth, '#') << string(maxBarWidth - barWidth, ' ');
    cout << " │ " << setw(5) << right << count['B'] << " │" << endl;
    
    cout << "│ C │ ";
    barWidth = count['C'] * maxBarWidth / total;
    cout << string(barWidth, '#') << string(maxBarWidth - barWidth, ' ');
    cout << " │ " << setw(5) << right << count['C'] << " │" << endl;
    
    cout << "│ D │ ";
    barWidth = count['D'] * maxBarWidth / total;
    cout << string(barWidth, '#') << string(maxBarWidth - barWidth, ' ');
    cout << " │ " << setw(5) << right << count['D'] << " │" << endl;
    
    cout << "│ E │ ";
    barWidth = count['E'] * maxBarWidth / total;
    cout << string(barWidth, '#') << string(maxBarWidth - barWidth, ' ');
    cout << " │ " << setw(5) << right << count['E'] << " │" << endl;
    cout << "└───┴───────────────────────────────────────────┴───────┘" << endl;
    
    // Tampilkan persentase
    cout << "\nPersentase:" << endl;
    cout << "A: " << fixed << setprecision(1) << (count['A'] * 100.0 / total) << "%" << endl;
    cout << "B: " << fixed << setprecision(1) << (count['B'] * 100.0 / total) << "%" << endl;
    cout << "C: " << fixed << setprecision(1) << (count['C'] * 100.0 / total) << "%" << endl;
    cout << "D: " << fixed << setprecision(1) << (count['D'] * 100.0 / total) << "%" << endl;
    cout << "E: " << fixed << setprecision(1) << (count['E'] * 100.0 / total) << "%" << endl;
    
    pressEnterToContinue();
}

void lihatRiwayatDanIPB() {
    printHeader();
    cout << BOLD << CYAN << "== RIWAYAT NILAI DAN INDEKS PRESTASI ==" << RESET << endl;
    
    if (sesi->riwayatNilai.empty()) {
        cout << YELLOW << "Belum ada data nilai." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    // Sort riwayat nilai by semester
    sort(sesi->riwayatNilai.begin(), sesi->riwayatNilai.end(), 
         [](const RiwayatNilai& a, const RiwayatNilai& b) {
             return a.semester < b.semester;
         });
    
    // Group by semester
    map<string, vector<RiwayatNilai>> bySemester;
    for (const RiwayatNilai& nilai : sesi->riwayatNilai) {
        bySemester[nilai.semester].push_back(nilai);
    }
    
    float totalBobot = 0;
    int totalSKS = 0;
    
    // Tampilkan per semester
    for (const auto& semester : bySemester) {
        cout << "\nSemester " << semester.first << ":" << endl;
        cout << "┌───────┬─────────────────────────────────┬─────┬────────┐" << endl;
        cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │ Nilai  │" << endl;
        cout << "├───────┼─────────────────────────────────┼─────┼────────┤" << endl;
        
        float semBobot = 0;
        int semSKS = 0;
        
        for (const RiwayatNilai& nilai : semester.second) {
            string nilaiStr;
            if (nilai.nilai == 'A') nilaiStr = GREEN + "A" + RESET;
            else if (nilai.nilai == 'B') nilaiStr = CYAN + "B" + RESET;
            else if (nilai.nilai == 'C') nilaiStr = YELLOW + "C" + RESET;
            else if (nilai.nilai == 'D') nilaiStr = MAGENTA + "D" + RESET;
            else nilaiStr = RED + "E" + RESET;
            
            cout << "│ " << setw(5) << left << nilai.kodeMK 
                 << "│ " << setw(31) << left << nilai.namaMK 
                 << "│ " << setw(3) << right << nilai.sks 
                 << " │ " << setw(6) << left << nilaiStr << " │" << endl;
            
            semBobot += nilaiMutuToAngka(nilai.nilai) * nilai.sks;
            semSKS += nilai.sks;
        }
        cout << "└───────┴─────────────────────────────────┴─────┴────────┘" << endl;
        
        float ip = semBobot / semSKS;
        cout << "IP Semester " << semester.first << ": " << fixed << setprecision(2) << ip << endl;
        
        totalBobot += semBobot;
        totalSKS += semSKS;
    }
    
    // Hitung IPK
    float ipk = totalBobot / totalSKS;
    cout << "\n" << BOLD << "IPK (Indeks Prestasi Kumulatif): " << fixed << setprecision(2) << ipk << RESET << endl;
    
    // Tampilkan predikat
    string predikat;
    if (ipk >= 3.5) predikat = GREEN + "Dengan Pujian (Cum Laude)" + RESET;
    else if (ipk >= 3.0) predikat = CYAN + "Sangat Memuaskan" + RESET;
    else if (ipk >= 2.5) predikat = YELLOW + "Memuaskan" + RESET;
    else predikat = RED + "Cukup" + RESET;
    
    cout << "Predikat: " << predikat << endl;
    
    pressEnterToContinue();
}

void simulasiIPK() {
    printHeader();
    cout << BOLD << CYAN << "== SIMULASI IPK ==" << RESET << endl;
    
    // Hitung IPK saat ini
    float totalBobot = 0;
    int totalSKS = 0;
    
    for (const RiwayatNilai& nilai : sesi->riwayatNilai) {
        totalBobot += nilaiMutuToAngka(nilai.nilai) * nilai.sks;
        totalSKS += nilai.sks;
    }
    
    float ipkSaatIni = 0;
    if (totalSKS > 0) {
        ipkSaatIni = totalBobot / totalSKS;
    }
    
    cout << "IPK saat ini: " << fixed << setprecision(2) << ipkSaatIni << endl;
    
    // Simulasi dengan mata kuliah di KRS
    if (sesi->krs.empty()) {
        cout << YELLOW << "Belum ada mata kuliah di KRS untuk simulasi." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    cout << "\nSimulasi IPK dengan mata kuliah di KRS:" << endl;
    cout << "┌───────┬─────────────────────────────────┬─────┬────────┐" << endl;
    cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │ Nilai  │" << endl;
    cout << "├───────┼─────────────────────────────────┼─────┼────────┤" << endl;
    
    vector<pair<string, char>> simulasi;
    int simSKS = 0;
    float simBobot = 0;
    
    for (const string& kode : sesi->krs) {
        MataKuliah& mk = matkul[kode];
        
        char nilai = 'A';  // Default simulasi dengan nilai A
        cout << "│ " << setw(5) << left << kode 
             << "│ " << setw(31) << left << mk.nama 
             << "│ " << setw(3) << right << mk.sks 
             << " │ ";
        
        cout << "A/B/C/D/E? ";
        char inputNilai;
        cin >> inputNilai;
        
        if (inputNilai == 'A' || inputNilai == 'B' || inputNilai == 'C' || 
            inputNilai == 'D' || inputNilai == 'E') {
            nilai = inputNilai;
        }
        
        cout << setw(6) << left << nilai << " │" << endl;
        
        simulasi.push_back({kode, nilai});
        simSKS += mk.sks;
        simBobot += nilaiMutuToAngka(nilai) * mk.sks;
    }
    cout << "└───────┴─────────────────────────────────┴─────┴────────┘" << endl;
    
    // Hitung IPK simulasi
    float ipkSimulasi = (totalBobot + simBobot) / (totalSKS + simSKS);
    
    cout << "\nHasil Simulasi:" << endl;
    cout << "Total SKS setelah semester ini: " << (totalSKS + simSKS) << endl;
    cout << "IPK simulasi: " << fixed << setprecision(2) << ipkSimulasi << endl;
    
    // Tampilkan predikat
    string predikat;
    if (ipkSimulasi >= 3.5) predikat = GREEN + "Dengan Pujian (Cum Laude)" + RESET;
    else if (ipkSimulasi >= 3.0) predikat = CYAN + "Sangat Memuaskan" + RESET;
    else if (ipkSimulasi >= 2.5) predikat = YELLOW + "Memuaskan" + RESET;
    else predikat = RED + "Cukup" + RESET;
    
    cout << "Predikat: " << predikat << endl;
    
    pressEnterToContinue();
}

void cariMataKuliah() {
    printHeader();
    cout << BOLD << CYAN << "== PENCARIAN MATA KULIAH ==" << RESET << endl;
    
    string keyword;
    cout << "Masukkan kata kunci (kode/nama): ";
    cin.ignore();
    getline(cin, keyword);
    
    // Ubah keyword menjadi lowercase untuk pencarian case-insensitive
    transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
    
    vector<MataKuliah> hasil;
    for (const auto& pair : matkul) {
        string kodeLower = pair.first;
        string namaLower = pair.second.nama;
        
        transform(kodeLower.begin(), kodeLower.end(), kodeLower.begin(), ::tolower);
        transform(namaLower.begin(), namaLower.end(), namaLower.begin(), ::tolower);
        
        if (kodeLower.find(keyword) != string::npos || namaLower.find(keyword) != string::npos) {
            hasil.push_back(pair.second);
        }
    }
    
    if (hasil.empty()) {
        cout << YELLOW << "Tidak ada mata kuliah yang sesuai dengan kata kunci." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    cout << "\nHasil Pencarian (" << hasil.size() << " mata kuliah):" << endl;
    cout << "┌───────┬─────────────────────────────────┬─────┬────────┬───────────┐" << endl;
    cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │ Kuota  │ Prasyarat │" << endl;
    cout << "├───────┼─────────────────────────────────┼─────┼────────┼───────────┤" << endl;
    
    for (const MataKuliah& mk : hasil) {
        string prasyarat;
        for (size_t i = 0; i < mk.prasyarat.size(); i++) {
            prasyarat += mk.prasyarat[i];
            if (i < mk.prasyarat.size() - 1) prasyarat += ", ";
        }
        
        cout << "│ " << setw(5) << left << mk.kode 
             << "│ " << setw(31) << left << mk.nama 
             << "│ " << setw(3) << right << mk.sks 
             << " │ " << setw(3) << right << mk.kuota_terisi << "/" << setw(2) << left << mk.kuota_max 
             << " │ " << setw(9) << left << prasyarat << " │" << endl;
    }
    cout << "└───────┴─────────────────────────────────┴─────┴────────┴───────────┘" << endl;
    
    pressEnterToContinue();
}

void exportKRS() {
    printHeader();
    cout << BOLD << CYAN << "== EXPORT KRS ==" << RESET << endl;
    
    if (sesi->krs.empty()) {
        cout << YELLOW << "Belum ada mata kuliah yang terdaftar dalam KRS." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    string filename = sesi->nim + "_KRS.txt";
    ofstream file(filename);
    
    if (!file.is_open()) {
        cout << RED << "Gagal membuat file export." << RESET << endl;
        pressEnterToContinue();
        return;
    }
    
    file << "====================================================" << endl;
    file << "             KARTU RENCANA STUDI (KRS)              " << endl;
    file << "====================================================" << endl;
    file << "NIM      : " << sesi->nim << endl;
    file << "Nama     : " << sesi->nama << endl;
    file << "Prodi    : " << sesi->prodi << endl;
    file << "Semester : " << sesi->semester << endl;
    file << "Tanggal  : " << getCurrentDateTime() << endl;
    file << "====================================================" << endl;
    file << endl;
    
    file << "DAFTAR MATA KULIAH:" << endl;
    file << "----------------------------------------------------" << endl;
    file << "| Kode  | Nama Mata Kuliah                  | SKS |" << endl;
    file << "----------------------------------------------------" << endl;
    
    int totalSKS = 0;
    for (const string& kode : sesi->krs) {
        MataKuliah& mk = matkul[kode];
        totalSKS += mk.sks;
        
        file << "| " << setw(5) << left << kode 
             << "| " << setw(34) << left << mk.nama 
             << "| " << setw(3) << right << mk.sks << " |" << endl;
    }
    file << "----------------------------------------------------" << endl;
    file << "Total SKS: " << totalSKS << "/" << sesi->max_sks << endl;
    file << endl;
    
    file << "JADWAL KULIAH:" << endl;
    file << "----------------------------------------------------" << endl;
    file << "| Hari       | Waktu      | Mata Kuliah           |" << endl;
    file << "----------------------------------------------------" << endl;
    
    const char* hari[] = {"Senin", "Selasa", "Rabu", "Kamis", "Jumat"};
    const char* waktu[] = {"08:00-09:40", "10:00-11:40", "13:00-14:40", "15:00-16:40"};
    
    int i = 0;
    for (const string& kode : sesi->krs) {
        file << "| " << setw(10) << left << hari[i % 5] 
             << "| " << setw(10) << left << waktu[(i/5) % 4] 
             << "| " << setw(5) << left << kode << " - " 
             << setw(15) << left << matkul[kode].nama << " |" << endl;
        i++;
    }
    file << "----------------------------------------------------" << endl;
    
    file.close();
    
    cout << GREEN << "KRS berhasil diexport ke file " << filename << RESET << endl;
    pressEnterToContinue();
}

void menuAdmin() {
    int pilih;
    do {
        printHeader();
        cout << BOLD << CYAN << "== MENU ADMINISTRATOR ==" << RESET << endl;
        cout << "1. Kelola Mata Kuliah" << endl;
        cout << "2. Kelola Mahasiswa" << endl;
        cout << "3. Input Nilai" << endl;
        cout << "4. Laporan Pengisian KRS" << endl;
        cout << "5. Kembali ke Menu Utama" << endl;
        cout << "Pilih: ";
        cin >> pilih;
        
        switch (pilih) {
            case 1: {
                // Menu Kelola Mata Kuliah
                int subPilih;
                do {
                    printHeader();
                    cout << BOLD << CYAN << "== KELOLA MATA KULIAH ==" << RESET << endl;
                    cout << "1. Lihat Semua Mata Kuliah" << endl;
                    cout << "2. Tambah Mata Kuliah" << endl;
                    cout << "3. Edit Mata Kuliah" << endl;
                    cout << "4. Hapus Mata Kuliah" << endl;
                    cout << "5. Kembali" << endl;
                    cout << "Pilih: ";
                    cin >> subPilih;
                    
                    switch (subPilih) {
                        case 1:
                            tampilMK(false);
                            break;
                        case 2: {
                            printHeader();
                            cout << BOLD << CYAN << "== TAMBAH MATA KULIAH ==" << RESET << endl;
                            
                            MataKuliah mk;
                            cout << "Kode MK: "; cin >> mk.kode;
                            
                            if (matkul.find(mk.kode) != matkul.end()) {
                                cout << RED << "Mata kuliah dengan kode tersebut sudah ada." << RESET << endl;
                                pressEnterToContinue();
                                break;
                            }
                            
                            cin.ignore();
                            cout << "Nama MK: "; getline(cin, mk.nama);
                            
                            mk.sks = getNumericInput<int>("SKS: ");
                            mk.kuota_max = getNumericInput<int>("Kuota Maksimum: ");
                            mk.kuota_terisi = 0;
                            
                            // Tambahkan prasyarat jika ada
                            string tambahPrasyarat;
                            cout << "Tambahkan Prasyarat? (y/n): "; cin >> tambahPrasyarat;
                            
                            if (tambahPrasyarat == "y" || tambahPrasyarat == "Y") {
                                while (true) {
                                    string kodePrasyarat;
                                    cout << "Kode MK Prasyarat (0 untuk selesai): ";
                                    cout << "Kode MK Prasyarat (0 untuk selesai): ";
                                    cin >> kodePrasyarat;
                                    
                                    if (kodePrasyarat == "0") break;
                                    
                                    if (matkul.find(kodePrasyarat) == matkul.end()) {
                                        cout << RED << "Mata kuliah prasyarat tidak ditemukan." << RESET << endl;
                                        continue;
                                    }
                                    
                                    mk.prasyarat.push_back(kodePrasyarat);
                                    cout << GREEN << "Prasyarat " << kodePrasyarat << " ditambahkan." << RESET << endl;
                                }
                            }
                            
                            matkul[mk.kode] = mk;
                            saveDataToFile();
                            
                            cout << GREEN << "Mata kuliah berhasil ditambahkan." << RESET << endl;
                            pressEnterToContinue();
                            break;
                        }
                        case 3: {
                            printHeader();
                            cout << BOLD << CYAN << "== EDIT MATA KULIAH ==" << RESET << endl;
                            
                            string kode;
                            cout << "Masukkan Kode MK yang akan diedit: "; cin >> kode;
                            
                            if (matkul.find(kode) == matkul.end()) {
                                cout << RED << "Mata kuliah tidak ditemukan." << RESET << endl;
                                pressEnterToContinue();
                                break;
                            }
                            
                            MataKuliah& mk = matkul[kode];
                            cout << "Data Mata Kuliah:" << endl;
                            cout << "Kode: " << mk.kode << endl;
                            cout << "Nama: " << mk.nama << endl;
                            cout << "SKS: " << mk.sks << endl;
                            cout << "Kuota: " << mk.kuota_terisi << "/" << mk.kuota_max << endl;
                            
                            cout << "Prasyarat: ";
                            for (size_t i = 0; i < mk.prasyarat.size(); i++) {
                                cout << mk.prasyarat[i];
                                if (i < mk.prasyarat.size() - 1) cout << ", ";
                            }
                            cout << endl;
                            
                            cin.ignore();
                            cout << "\nEdit Nama MK (kosongkan jika tidak ingin mengubah): ";
                            string nama;
                            getline(cin, nama);
                            if (!nama.empty()) mk.nama = nama;
                            
                            cout << "Edit SKS (0 jika tidak ingin mengubah): ";
                            int sks;
                            cin >> sks;
                            if (sks > 0) mk.sks = sks;
                            
                            cout << "Edit Kuota Maksimum (0 jika tidak ingin mengubah): ";
                            int kuota;
                            cin >> kuota;
                            if (kuota > 0) mk.kuota_max = kuota;
                            
                            // Edit prasyarat jika diperlukan
                            string editPrasyarat;
                            cout << "Edit Prasyarat? (y/n): "; cin >> editPrasyarat;
                            
                            if (editPrasyarat == "y" || editPrasyarat == "Y") {
                                mk.prasyarat.clear();
                                while (true) {
                                    string kodePrasyarat;
                                    cout << "Kode MK Prasyarat (0 untuk selesai): ";
                                    cin >> kodePrasyarat;
                                    
                                    if (kodePrasyarat == "0") break;
                                    
                                    if (matkul.find(kodePrasyarat) == matkul.end()) {
                                        cout << RED << "Mata kuliah prasyarat tidak ditemukan." << RESET << endl;
                                        continue;
                                    }
                                    
                                    mk.prasyarat.push_back(kodePrasyarat);
                                    cout << GREEN << "Prasyarat " << kodePrasyarat << " ditambahkan." << RESET << endl;
                                }
                            }
                            
                            saveDataToFile();
                            cout << GREEN << "Mata kuliah berhasil diupdate." << RESET << endl;
                            pressEnterToContinue();
                            break;
                        }
                        case 4: {
                            printHeader();
                            cout << BOLD << CYAN << "== HAPUS MATA KULIAH ==" << RESET << endl;
                            
                            string kode;
                            cout << "Masukkan Kode MK yang akan dihapus: "; cin >> kode;
                            
                            if (matkul.find(kode) == matkul.end()) {
                                cout << RED << "Mata kuliah tidak ditemukan." << RESET << endl;
                                pressEnterToContinue();
                                break;
                            }
                            
                            // Cek apakah mata kuliah sedang digunakan dalam KRS
                            bool digunakan = false;
                            for (auto& pair : akun) {
                                Mahasiswa& mhs = pair.second;
                                for (const string& mkKode : mhs.krs) {
                                    if (mkKode == kode) {
                                        digunakan = true;
                                        break;
                                    }
                                }
                                if (digunakan) break;
                            }
                            
                            if (digunakan) {
                                cout << RED << "Mata kuliah sedang digunakan dalam KRS mahasiswa." << RESET << endl;
                                cout << "Hapus mata kuliah dari KRS semua mahasiswa? (y/n): ";
                                
                                string konfirmasi;
                                cin >> konfirmasi;
                                
                                if (konfirmasi != "y" && konfirmasi != "Y") {
                                    cout << YELLOW << "Penghapusan dibatalkan." << RESET << endl;
                                    pressEnterToContinue();
                                    break;
                                }
                                
                                // Hapus mata kuliah dari KRS semua mahasiswa
                                for (auto& pair : akun) {
                                    Mahasiswa& mhs = pair.second;
                                    mhs.krs.erase(remove(mhs.krs.begin(), mhs.krs.end(), kode), mhs.krs.end());
                                }
                            }
                            
                            // Cek jika mata kuliah digunakan sebagai prasyarat
                            vector<string> digunakan_sebagai_prasyarat;
                            for (auto& pair : matkul) {
                                for (const string& prasyarat : pair.second.prasyarat) {
                                    if (prasyarat == kode) {
                                        digunakan_sebagai_prasyarat.push_back(pair.first);
                                        break;
                                    }
                                }
                            }
                            
                            if (!digunakan_sebagai_prasyarat.empty()) {
                                cout << YELLOW << "Mata kuliah digunakan sebagai prasyarat untuk: ";
                                for (size_t i = 0; i < digunakan_sebagai_prasyarat.size(); i++) {
                                    cout << digunakan_sebagai_prasyarat[i];
                                    if (i < digunakan_sebagai_prasyarat.size() - 1) cout << ", ";
                                }
                                cout << RESET << endl;
                                
                                cout << "Hapus prasyarat dari mata kuliah tersebut? (y/n): ";
                                string konfirmasi;
                                cin >> konfirmasi;
                                
                                if (konfirmasi != "y" && konfirmasi != "Y") {
                                    cout << YELLOW << "Penghapusan dibatalkan." << RESET << endl;
                                    pressEnterToContinue();
                                    break;
                                }
                                
                                // Hapus prasyarat dari mata kuliah lain
                                for (auto& pair : matkul) {
                                    pair.second.prasyarat.erase(
                                        remove(pair.second.prasyarat.begin(), pair.second.prasyarat.end(), kode),
                                        pair.second.prasyarat.end()
                                    );
                                }
                            }
                            
                            // Hapus mata kuliah
                            matkul.erase(kode);
                            saveDataToFile();
                            
                            cout << GREEN << "Mata kuliah berhasil dihapus." << RESET << endl;
                            pressEnterToContinue();
                            break;
                        }
                        case 5:
                            break;
                        default:
                            cout << RED << "Pilihan tidak valid." << RESET << endl;
                            pressEnterToContinue();
                    }
                } while (subPilih != 5);
                break;
            }
            case 2: {
                // Menu Kelola Mahasiswa
                int subPilih;
                do {
                    printHeader();
                    cout << BOLD << CYAN << "== KELOLA MAHASISWA ==" << RESET << endl;
                    cout << "1. Lihat Semua Mahasiswa" << endl;
                    cout << "2. Tambah Mahasiswa" << endl;
                    cout << "3. Edit Mahasiswa" << endl;
                    cout << "4. Hapus Mahasiswa" << endl;
                    cout << "5. Reset Password" << endl;
                    cout << "6. Kembali" << endl;
                    cout << "Pilih: ";
                    cin >> subPilih;
                    
                    switch (subPilih) {
                        case 1: {
                            printHeader();
                            cout << BOLD << CYAN << "== DAFTAR MAHASISWA ==" << RESET << endl;
                            
                            cout << "┌────────────┬────────────────────────────┬────────────────┬─────────┐" << endl;
                            cout << "│ NIM        │ Nama                       │ Program Studi  │ Semester│" << endl;
                            cout << "├────────────┼────────────────────────────┼────────────────┼─────────┤" << endl;
                            
                            for (const auto& pair : akun) {
                                if (pair.first != "admin") {
                                    cout << "│ " << setw(10) << left << pair.second.nim 
                                         << "│ " << setw(26) << left << pair.second.nama 
                                         << "│ " << setw(14) << left << pair.second.prodi 
                                         << "│ " << setw(7) << right << pair.second.semester << " │" << endl;
                                }
                            }
                            cout << "└────────────┴────────────────────────────┴────────────────┴─────────┘" << endl;
                            
                            pressEnterToContinue();
                            break;
                        }
                        case 2: {
                            // Tambah Mahasiswa (sama dengan registerAkun)
                            registerAkun();
                            break;
                        }
                        case 3: {
                            printHeader();
                            cout << BOLD << CYAN << "== EDIT MAHASISWA ==" << RESET << endl;
                            
                            string nim;
                            cout << "Masukkan NIM mahasiswa yang akan diedit: "; cin >> nim;
                            
                            if (nim == "admin" || akun.find(nim) == akun.end()) {
                                cout << RED << "Mahasiswa tidak ditemukan." << RESET << endl;
                                pressEnterToContinue();
                                break;
                            }
                            
                            Mahasiswa& mhs = akun[nim];
                            cout << "Data Mahasiswa:" << endl;
                            cout << "NIM: " << mhs.nim << endl;
                            cout << "Nama: " << mhs.nama << endl;
                            cout << "Program Studi: " << mhs.prodi << endl;
                            cout << "Semester: " << mhs.semester << endl;
                            cout << "Max SKS: " << mhs.max_sks << endl;
                            
                            cin.ignore();
                            cout << "\nEdit Nama (kosongkan jika tidak ingin mengubah): ";
                            string nama;
                            getline(cin, nama);
                            if (!nama.empty()) mhs.nama = nama;
                            
                            cout << "Edit Program Studi (kosongkan jika tidak ingin mengubah): ";
                            string prodi;
                            getline(cin, prodi);
                            if (!prodi.empty()) mhs.prodi = prodi;
                            
                            cout << "Edit Semester (0 jika tidak ingin mengubah): ";
                            int semester;
                            cin >> semester;
                            if (semester > 0) mhs.semester = semester;
                            
                            cout << "Edit Max SKS (0 jika tidak ingin mengubah): ";
                            int maxSks;
                            cin >> maxSks;
                            if (maxSks > 0) mhs.max_sks = maxSks;
                            
                            saveDataToFile();
                            cout << GREEN << "Data mahasiswa berhasil diupdate." << RESET << endl;
                            pressEnterToContinue();
                            break;
                        }
                        case 4: {
                            printHeader();
                            cout << BOLD << CYAN << "== HAPUS MAHASISWA ==" << RESET << endl;
                            
                            string nim;
                            cout << "Masukkan NIM mahasiswa yang akan dihapus: "; cin >> nim;
                            
                            if (nim == "admin" || akun.find(nim) == akun.end()) {
                                cout << RED << "Mahasiswa tidak ditemukan." << RESET << endl;
                                pressEnterToContinue();
                                break;
                            }
                            
                            cout << "Yakin ingin menghapus mahasiswa " << nim << " - " << akun[nim].nama << "? (y/n): ";
                            string konfirmasi;
                            cin >> konfirmasi;
                            
                            if (konfirmasi == "y" || konfirmasi == "Y") {
                                // Kurangi kuota_terisi untuk setiap mata kuliah yang diambil
                                for (const string& kode : akun[nim].krs) {
                                    matkul[kode].kuota_terisi--;
                                }
                                
                                akun.erase(nim);
                                saveDataToFile();
                                
                                cout << GREEN << "Mahasiswa berhasil dihapus." << RESET << endl;
                            } else {
                                cout << YELLOW << "Penghapusan dibatalkan." << RESET << endl;
                            }
                            
                            pressEnterToContinue();
                            break;
                        }
                        case 5: {
                            printHeader();
                            cout << BOLD << CYAN << "== RESET PASSWORD MAHASISWA ==" << RESET << endl;
                            
                            string nim;
                            cout << "Masukkan NIM mahasiswa yang akan direset passwordnya: "; cin >> nim;
                            
                            if (akun.find(nim) == akun.end()) {
                                cout << RED << "Mahasiswa tidak ditemukan." << RESET << endl;
                                pressEnterToContinue();
                                break;
                            }
                            
                            string newPass = nim;  // Password default adalah NIM
                            akun[nim].password_hash = hashPassword(newPass);
                            saveDataToFile();
                            
                            cout << GREEN << "Password mahasiswa " << nim << " berhasil direset." << RESET << endl;
                            cout << "Password baru: " << newPass << endl;
                            pressEnterToContinue();
                            break;
                        }
                        case 6:
                            break;
                        default:
                            cout << RED << "Pilihan tidak valid." << RESET << endl;
                            pressEnterToContinue();
                    }
                } while (subPilih != 6);
                break;
            }
            case 3: {
                // Input Nilai
                printHeader();
                cout << BOLD << CYAN << "== INPUT NILAI MAHASISWA ==" << RESET << endl;
                
                string nim;
                cout << "Masukkan NIM mahasiswa: "; cin >> nim;
                
                if (akun.find(nim) == akun.end()) {
                    cout << RED << "Mahasiswa tidak ditemukan." << RESET << endl;
                    pressEnterToContinue();
                    break;
                }
                
                Mahasiswa& mhs = akun[nim];
                cout << "Mahasiswa: " << mhs.nim << " - " << mhs.nama << endl;
                
                // Tampilkan mata kuliah yang diambil
                cout << "\nMata Kuliah yang diambil:" << endl;
                cout << "┌───────┬─────────────────────────────────┬─────┐" << endl;
                cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │" << endl;
                cout << "├───────┼─────────────────────────────────┼─────┤" << endl;
                
                for (const string& kode : mhs.krs) {
                    cout << "│ " << setw(5) << left << kode 
                         << "│ " << setw(31) << left << matkul[kode].nama 
                         << "│ " << setw(3) << right << matkul[kode].sks << " │" << endl;
                }
                cout << "└───────┴─────────────────────────────────┴─────┘" << endl;
                
                string kodeMK;
                cout << "\nMasukkan Kode MK yang akan diinput nilainya (atau 0 untuk batal): "; cin >> kodeMK;
                
                if (kodeMK == "0") break;
                
                if (find(mhs.krs.begin(), mhs.krs.end(), kodeMK) == mhs.krs.end()) {
                    cout << RED << "Mata kuliah tidak terdaftar dalam KRS mahasiswa." << RESET << endl;
                    pressEnterToContinue();
                    break;
                }
                
                char nilai;
                cout << "Masukkan Nilai (A/B/C/D/E): "; cin >> nilai;
                
                if (nilai != 'A' && nilai != 'B' && nilai != 'C' && nilai != 'D' && nilai != 'E') {
                    cout << RED << "Nilai tidak valid." << RESET << endl;
                    pressEnterToContinue();
                    break;
                }
                
                // Input nilai
                matkul[kodeMK].nilaiMutu[nim] = nilai;
                
                // Tambahkan ke riwayat nilai
                RiwayatNilai rn;
                rn.kodeMK = kodeMK;
                rn.namaMK = matkul[kodeMK].nama;
                rn.sks = matkul[kodeMK].sks;
                rn.nilai = nilai;
                rn.semester = to_string(mhs.semester);
                
                // Hapus dari riwayat nilai jika sudah ada
                for (auto it = mhs.riwayatNilai.begin(); it != mhs.riwayatNilai.end(); ) {
                    if (it->kodeMK == kodeMK) {
                        it = mhs.riwayatNilai.erase(it);
                    } else {
                        ++it;
                    }
                }
                
                mhs.riwayatNilai.push_back(rn);
                
                // Hapus dari KRS jika sudah dinilai
                mhs.krs.erase(remove(mhs.krs.begin(), mhs.krs.end(), kodeMK), mhs.krs.end());
                matkul[kodeMK].kuota_terisi--;
                
                saveDataToFile();
                
                cout << GREEN << "Nilai berhasil diinput." << RESET << endl;
                pressEnterToContinue();
                break;
            }
            case 4: {
                // Laporan Pengisian KRS
                printHeader();
                cout << BOLD << CYAN << "== LAPORAN PENGISIAN KRS ==" << RESET << endl;
                
                // Tampilkan statistik pengambilan mata kuliah
                cout << "Statistik Pengambilan Mata Kuliah:" << endl;
                cout << "┌───────┬─────────────────────────────────┬─────┬────────┬───────────┐" << endl;
                cout << "│ Kode  │ Nama Mata Kuliah               │ SKS │ Kuota  │ Persentase│" << endl;
                cout << "├───────┼─────────────────────────────────┼─────┼────────┼───────────┤" << endl;
                
                vector<pair<string, float>> utilization;
                for (const auto& pair : matkul) {
                    float persentase = (static_cast<float>(pair.second.kuota_terisi) / pair.second.kuota_max) * 100;
                    utilization.push_back({pair.first, persentase});
                }
                
                sort(utilization.begin(), utilization.end(), [](const pair<string, float>& a, const pair<string, float>& b) {
                    return a.second > b.second; // Sort by percentage (descending)
                });
                
                for (const auto& pair : utilization) {
                    const MataKuliah& mk = matkul[pair.first];
                    string status;
                    
                    if (pair.second >= 90) {
                        status = RED + to_string(static_cast<int>(pair.second)) + "%" + RESET;
                    } else if (pair.second >= 70) {
                        status = YELLOW + to_string(static_cast<int>(pair.second)) + "%" + RESET;
                    } else {
                        status = GREEN + to_string(static_cast<int>(pair.second)) + "%" + RESET;
                    }
                    
                    cout << "│ " << setw(5) << left << mk.kode 
                         << "│ " << setw(31) << left << mk.nama 
                         << "│ " << setw(3) << right << mk.sks 
                         << " │ " << setw(3) << right << mk.kuota_terisi << "/" << setw(2) << left << mk.kuota_max 
                         << " │ " << setw(9) << left << status << " │" << endl;
                }
                cout << "└───────┴─────────────────────────────────┴─────┴────────┴───────────┘" << endl;
                
                // Tampilkan jumlah mahasiswa per semester
                cout << "\nJumlah Mahasiswa per Semester:" << endl;
                cout << "┌─────────┬─────────────┐" << endl;
                cout << "│ Semester│ Jumlah Mhs  │" << endl;
                cout << "├─────────┼─────────────┤" << endl;
                
                map<int, int> mhsPerSemester;
                for (const auto& pair : akun) {
                    if (pair.first != "admin") {
                        mhsPerSemester[pair.second.semester]++;
                    }
                }
                
                for (const auto& pair : mhsPerSemester) {
                    cout << "│ " << setw(7) << right << pair.first 
                         << " │ " << setw(11) << right << pair.second << " │" << endl;
                }
                cout << "└─────────┴─────────────┘" << endl;
                
                // Tampilkan jumlah pengambilan mata kuliah
                cout << "\nJumlah Pengambilan Mata Kuliah per Mahasiswa:" << endl;
                cout << "┌────────────┬─────────────────────────────────┬───────────┐" << endl;
                cout << "│ NIM        │ Nama                            │ Jumlah MK │" << endl;
                cout << "├────────────┼─────────────────────────────────┼───────────┤" << endl;
                
                vector<pair<string, int>> krsCount;
                for (const auto& pair : akun) {
                    if (pair.first != "admin") {
                        krsCount.push_back({pair.first, pair.second.krs.size()});
                    }
                }
                
                sort(krsCount.begin(), krsCount.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
                    return a.second > b.second; // Sort by count (descending)
                });
                
                for (const auto& pair : krsCount) {
                    cout << "│ " << setw(10) << left << pair.first 
                         << "│ " << setw(33) << left << akun[pair.first].nama 
                         << "│ " << setw(9) << right << pair.second << " │" << endl;
                }
                cout << "└────────────┴─────────────────────────────────┴───────────┘" << endl;
                
                pressEnterToContinue();
                break;
            }
            case 5:
                break;
            default:
                cout << RED << "Pilihan tidak valid." << RESET << endl;
                pressEnterToContinue();
        }
    } while (pilih != 5);
}

void menuUtama() {
    int pilih;
    do {
        printHeader();
        
        if (isAdmin) {
            cout << BOLD << CYAN << "== MENU ADMINISTRATOR ==" << RESET << endl;
            cout << "1. Kelola Mata Kuliah" << endl;
            cout << "2. Kelola Mahasiswa" << endl;
            cout << "3. Input Nilai" << endl;
            cout << "4. Laporan Pengisian KRS" << endl;
            cout << "5. Logout" << endl;
            cout << "Pilih: ";
            cin >> pilih;
            
            switch (pilih) {
                case 1:
                case 2:
                case 3:
                case 4:
                    menuAdmin();
                    break;
                case 5:
                    sesi = nullptr;
                    isAdmin = false;
                    cout << GREEN << "Logout berhasil." << RESET << endl;
                    pressEnterToContinue();
                    break;
                default:
                    cout << RED << "Pilihan tidak valid." << RESET << endl;
                    pressEnterToContinue();
            }
        } else {
            cout << BOLD << CYAN << "== MENU UTAMA ==" << RESET << endl;
            cout << "1. Tambah Mata Kuliah" << endl;
            cout << "2. Batal Mata Kuliah" << endl;
            cout << "3. Undo Aksi Terakhir" << endl;
            cout << "4. Lihat KRS" << endl;
            cout << "5. Lihat Daftar Mata Kuliah" << endl;
            cout << "6. Lihat Kuota Mata Kuliah" << endl;
            cout << "7. Lihat Sebaran Nilai" << endl;
            cout << "8. Lihat Riwayat Nilai dan IPK" << endl;
            cout << "9. Simulasi IPK" << endl;
            cout << "10. Cari Mata Kuliah" << endl;
            cout << "11. Export KRS" << endl;
            cout << "12. Logout" << endl;
            cout << "Pilih: ";
            cin >> pilih;
            
            switch (pilih) {
                case 1:
                    tambahMK();
                    break;
                case 2:
                    batalMK();
                    break;
                case 3:
                    undo();
                    break;
                case 4:
                    lihatKRS();
                    break;
                case 5: {
                    int subPilih;
                    cout << "\nUrutkan berdasarkan: " << endl;
                    cout << "1. Kode MK" << endl;
                    cout << "2. Nama MK" << endl;
                    cout << "Pilih: ";
                    cin >> subPilih;
                    
                    if (subPilih == 1) {
                        tampilMK(false);
                    } else if (subPilih == 2) {
                        tampilMK(true);
                    } else {
                        cout << RED << "Pilihan tidak valid." << RESET << endl;
                        pressEnterToContinue();
                    }
                    break;
                }
                case 6:
                    lihatKuota();
                    break;
                case 7:
                    lihatSebaran();
                    break;
                case 8:
                    lihatRiwayatDanIPB();
                    break;
                case 9:
                    simulasiIPK();
                    break;
                case 10:
                    cariMataKuliah();
                    break;
                case 11:
                    exportKRS();
                    break;
                case 12:
                    sesi = nullptr;
                    cout << GREEN << "Logout berhasil." << RESET << endl;
                    pressEnterToContinue();
                    break;
                default:
                    cout << RED << "Pilihan tidak valid." << RESET << endl;
                    pressEnterToContinue();
            }
        }
    } while (pilih != (isAdmin ? 5 : 12));
}

int main() {
    loadDataFromFile();
    int pilih;
    do {
        printHeader();
        cout << BOLD << CYAN << "== SELAMAT DATANG DI SISTEM KRS ==" << RESET << endl;
        cout << "1. Login" << endl;
        cout << "2. Register" << endl;
        cout << "3. Keluar" << endl;
        cout << "Pilih: ";
        cin >> pilih;
        
        switch (pilih) {
            case 1:
                login();
                break;
            case 2:
                registerAkun();
                break;
            case 3:
                cout << GREEN << "Terima kasih telah menggunakan sistem KRS." << RESET << endl;
                pressEnterToContinue();
                break;
            default:
                cout << RED << "Pilihan tidak valid." << RESET << endl;
                pressEnterToContinue();
        }
    } while (pilih != 3);
    
    return 0;
}