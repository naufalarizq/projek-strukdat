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
    using namespace std;

    struct Aksi {
        string kodeMK;
        bool tambah;
    };

    struct MataKuliah {
        string kode;
        string nama;
        int sks;
        int kuota_max;
        int kuota_terisi = 0;
        map<string, char> nilaiMutu; // NIM -> Nilai
    };

    struct RiwayatNilai {
        string kodeMK;
        string namaMK;
        int sks;
        char nilai;
    };

    struct Mahasiswa {
        string nim;
        string nama;
        string password_hash;
        vector<string> krs;
        stack<Aksi> riwayat;
        vector<RiwayatNilai> riwayatNilai;
    };

    unordered_map<string, Mahasiswa> akun;
    unordered_map<string, MataKuliah> matkul;
    Mahasiswa* sesi = nullptr;

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
        }
        return 0.0;
    }

    void daftarMataKuliah() {
        matkul["IF101"] = {"IF101", "Algoritma", 3, 30};
        matkul["IF102"] = {"IF102", "Struktur Data", 3, 25};
        matkul["IF103"] = {"IF103", "Basis Data", 3, 20};
        matkul["IF104"] = {"IF104", "Jaringan Komputer", 3, 20};
        matkul["IF105"] = {"IF105", "Pemrograman Web", 3, 25};

        // Tambahkan contoh nilai dan riwayat
        matkul["IF101"].nilaiMutu["123"] = 'A';
        matkul["IF101"].nilaiMutu["124"] = 'B';
        matkul["IF101"].nilaiMutu["125"] = 'A';

        akun["123"].riwayatNilai.push_back({"IF101", "Algoritma", 3, 'A'});
        akun["123"].riwayatNilai.push_back({"IF102", "Struktur Data", 3, 'B'});
        akun["123"].riwayatNilai.push_back({"IF103", "Basis Data", 3, 'A'});
    }

    void menuUtama();

    void registerAkun() {
        string nim, nama, pass;
        cout << "Masukkan NIM: "; cin >> nim;
        if (akun.find(nim) != akun.end()) {
            cout << "Akun sudah terdaftar.\n";
            return;
        }
        cout << "Masukkan Nama: "; cin.ignore(); getline(cin, nama);
        cout << "Masukkan Password: "; getline(cin, pass);

        Mahasiswa mhs;
        mhs.nim = nim;
        mhs.nama = nama;
        mhs.password_hash = hashPassword(pass);
        akun[nim] = mhs;

        cout << "Registrasi berhasil. Silakan login.\n";
    }

    void login() {
        string nim, pass;
        cout << "NIM: "; cin >> nim;
        cout << "Password: "; cin.ignore(); getline(cin, pass);

        unordered_map<string, Mahasiswa>::iterator it = akun.find(nim);
        if (it != akun.end() && it->second.password_hash == hashPassword(pass)) {
            sesi = &it->second;
            cout << "Login berhasil. Selamat datang, " << sesi->nama << "!\n";
            menuUtama();
        } else {
            cout << "Login gagal.\n";
        }
    }

    void tambahMK() {
        string kode;
        cout << "Kode MK: "; cin >> kode;
        if (find(sesi->krs.begin(), sesi->krs.end(), kode) != sesi->krs.end()) {
            cout << "MK sudah diambil.\n";
            return;
        }
        if (matkul[kode].kuota_terisi >= matkul[kode].kuota_max) {
            cout << "Kuota penuh.\n";
            return;
        }
        sesi->krs.push_back(kode);
        matkul[kode].kuota_terisi++;
        sesi->riwayat.push({kode, true});
        cout << "MK ditambahkan.\n";
    }

    void batalMK() {
        string kode;
        cout << "Kode MK: "; cin >> kode;
        vector<string>::iterator it = find(sesi->krs.begin(), sesi->krs.end(), kode);
        if (it == sesi->krs.end()) {
            cout << "MK belum diambil.\n";
            return;
        }
        sesi->krs.erase(it);
        matkul[kode].kuota_terisi--;
        sesi->riwayat.push({kode, false});
        cout << "MK dibatalkan.\n";
    }

    void undo() {
        if (sesi->riwayat.empty()) {
            cout << "Tidak ada aksi untuk di-undo.\n";
            return;
        }
        Aksi aksi = sesi->riwayat.top(); sesi->riwayat.pop();
        if (aksi.tambah) {
            sesi->krs.erase(remove(sesi->krs.begin(), sesi->krs.end(), aksi.kodeMK), sesi->krs.end());
            matkul[aksi.kodeMK].kuota_terisi--;
            cout << "Undo tambah MK.\n";
        } else {
            sesi->krs.push_back(aksi.kodeMK);
            matkul[aksi.kodeMK].kuota_terisi++;
            cout << "Undo batal MK.\n";
        }
    }

    void tampilMK(bool byNama = false) {
        vector<MataKuliah> daftar;
        for (unordered_map<string, MataKuliah>::iterator it = matkul.begin(); it != matkul.end(); ++it) {
            daftar.push_back(it->second);
        }
        sort(daftar.begin(), daftar.end(), [=](MataKuliah a, MataKuliah b) {
            return byNama ? a.nama < b.nama : a.kode < b.kode;
        });
        for (size_t i = 0; i < daftar.size(); ++i) {
            MataKuliah& mk = daftar[i];
            cout << mk.kode << " - " << mk.nama << " (" << mk.kuota_terisi << "/" << mk.kuota_max << ")\n";
        }
    }

    void lihatKuota() {
        for (unordered_map<string, MataKuliah>::iterator it = matkul.begin(); it != matkul.end(); ++it) {
            cout << it->first << ": " << it->second.kuota_terisi << "/" << it->second.kuota_max << "\n";
        }
    }

    void lihatSebaran() {
        string kode;
        cout << "Kode MK: "; cin >> kode;
        MataKuliah& mk = matkul[kode];
        map<char, int> count;
        for (map<string, char>::iterator it = mk.nilaiMutu.begin(); it != mk.nilaiMutu.end(); ++it) {
            count[it->second]++;
        }
        cout << "A | " << string(count['A'], '#') << " (" << count['A'] << ")\n";
        cout << "B | " << string(count['B'], '#') << " (" << count['B'] << ")\n";
        cout << "C | " << string(count['C'], '#') << " (" << count['C'] << ")\n";
        cout << "D | " << string(count['D'], '#') << " (" << count['D'] << ")\n";
        cout << "E | " << string(count['E'], '#') << " (" << count['E'] << ")\n";

        ostringstream cmd;
        cmd << "gnome-terminal -- ./chart_viewer " << mk.nama << " " << count['A'] << " "
            << count['B'] << " " << count['C'] << " " << count['D'] << " " << count['E'];
        system(cmd.str().c_str());
    }

    void lihatRiwayatDanIPB() {
        cout << "\nRiwayat Mata Kuliah yang telah diambil:\n";
        float totalBobot = 0;
        int totalSKS = 0;
        for (size_t i = 0; i < sesi->riwayatNilai.size(); ++i) {
            RiwayatNilai r = sesi->riwayatNilai[i];
            cout << r.kodeMK << " - " << r.namaMK << " (" << r.sks << " sks) : " << r.nilai << "\n";
            totalBobot += nilaiMutuToAngka(r.nilai) * r.sks;
            totalSKS += r.sks;
        }
        if (totalSKS > 0) {
            float ipb = totalBobot / totalSKS;
            cout << fixed << setprecision(2);
            cout << "\nIPB (Indeks Prestasi Berdasar riwayat): " << ipb << "\n";
        } else {
            cout << "Belum ada data nilai.\n";
        }
    }

    void menuUtama() {
        int pilih;
        do {
            cout << "\nMenu:\n1. Tambah MK\n2. Batal MK\n3. Undo\n4. Lihat MK (Kode)\n5. Lihat MK (Nama)\n6. Kuota\n7. Sebaran Nilai\n8. Riwayat Nilai & IPB\n9. Logout\nPilih: ";
            cin >> pilih;
            switch (pilih) {
                case 1: tambahMK(); break;
                case 2: batalMK(); break;
                case 3: undo(); break;
                case 4: tampilMK(false); break;
                case 5: tampilMK(true); break;
                case 6: lihatKuota(); break;
                case 7: lihatSebaran(); break;
                case 8: lihatRiwayatDanIPB(); break;
                case 9: sesi = nullptr; cout << "Logout.\n"; break;
            }
        } while (sesi);
    }

    int main() {
        daftarMataKuliah();
        int pilih;
        do {
            cout << "\n1. Register\n2. Login\n3. Keluar\nPilih: ";
            cin >> pilih;
            switch (pilih) {
                case 1: registerAkun(); break;
                case 2: login(); break;
            }
        } while (pilih != 3);
        return 0;
    }