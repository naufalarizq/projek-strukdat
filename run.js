const { exec } = require('child_process');
const os = require('os');

// Mengecek sistem operasi untuk menentukan cara membuka terminal
if (os.platform() === 'win32') {
    // Jika di Windows, menggunakan `start` untuk membuka terminal baru
    exec('start cmd /K "program.exe"', (error, stdout, stderr) => {
        if (error) {
            console.error(`exec error: ${error}`);
            return;
        }
        if (stderr) {
            console.error(`stderr: ${stderr}`);
            return;
        }
        console.log(`stdout: ${stdout}`);
    });
} else if (os.platform() === 'linux' || os.platform() === 'darwin') {
    // Jika di Linux atau macOS, menggunakan `gnome-terminal`
    exec('gnome-terminal -- bash -c "./program; exec bash"', (error, stdout, stderr) => {
        if (error) {
            console.error(`exec error: ${error}`);
            return;
        }
        if (stderr) {
            console.error(`stderr: ${stderr}`);
            return;
        }
        console.log(`stdout: ${stdout}`);
    });
}
