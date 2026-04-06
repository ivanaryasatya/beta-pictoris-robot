import serial
import threading
import sys
import time
import json

# Load command map
try:
    with open("C:/Users/ROG/Documents/PlatformIO/Projects/beta_pictoris/tools/command_map.json", "r") as f:
        COMMAND_MAP = json.load(f)
except FileNotFoundError:
    print("[Error] File command_map.json tidak ditemukan!")
    COMMAND_MAP = {}

DEFAULT_PORT = "COM6"
DEFAULT_BAUD = 115200

ser = None
is_connected = False

def parse_command(input_text):
    """
    Convert command format:
    >target.command.param1.param2
    menjadi:
    shortcode.param1.param2
    """
    if not input_text.startswith(">"):
        return input_text

    try:
        command_body = input_text[1:]
        parts = command_body.split(".")

        if len(parts) < 2:
            return input_text

        key = ".".join(parts[:2])
        params = parts[2:]

        if key in COMMAND_MAP:
            shortcode = COMMAND_MAP[key]

            if params:
                param_str = ".".join(params)
                return f"{shortcode}.{param_str}"
            else:
                return shortcode

        else:
            print(f"[Sistem] Command tidak dikenal dalam map: {key}")
            return input_text

    except Exception as e:
        print(f"[Error] Gagal parse command: {e}")
        return input_text
    
def koneksi_dan_baca(port, baud):
    """
    Thread background: Bertugas menjaga koneksi dan membaca data.
    Jika putus, fungsi ini akan terus mencoba menyambung kembali.
    """
    global ser, is_connected
    
    while True:
        if not is_connected:
            try:
                ser = serial.Serial(port, baud, timeout=1)
                is_connected = True
                print(f"\n[Sistem] BERHASIL tersambung ke {port} @ {baud} bps.")
                print("[TX] > ", end="", flush=True)
            except serial.SerialException:
                time.sleep(2)
                continue

        try:
            if ser.in_waiting > 0:
                data_masuk = ser.readline().decode('utf-8', errors='ignore').strip()
                if data_masuk: 
                    print(f"\r[RX] {data_masuk}\n[TX] > ", end="", flush=True)
            time.sleep(0.01)
            
        except (serial.SerialException, OSError, AttributeError):
            is_connected = False
            print("\n[Sistem] KONEKSI TERPUTUS! Mencoba menyambung kembali...")
            if ser:
                ser.close()

print("=== Python Serial Monitor (Auto-Reconnect) ===")
input_port = input(f"Masukkan COM Port (Tekan Enter untuk {DEFAULT_PORT}): ")
port = input_port if input_port != "" else DEFAULT_PORT

input_baud = input(f"Masukkan Baud Rate (Tekan Enter untuk {DEFAULT_BAUD}): ")
baud = int(input_baud) if input_baud != "" else DEFAULT_BAUD

print(f"\n[Sistem] Mencari {port}... (Ketik 'x' untuk keluar)")

thread_baca = threading.Thread(target=koneksi_dan_baca, args=(port, baud))
thread_baca.daemon = True 
thread_baca.start()

try:
    while True:
        data_keluar = input()
        
        # 1. Cek apakah user ingin keluar
        if data_keluar.lower() == 'x':
            print("[Sistem] Menutup program...")
            break
            
        # 2. Cek apakah input diawali dengan '>'
        if not data_keluar.startswith(">"):
            print("[Error] Perintah ditolak! Input harus diawali dengan tanda '>' (contoh: >target.command)")
            print("[TX] > ", end="", flush=True)
            continue
            
        # 3. Proses pengiriman data
        if is_connected and ser and ser.is_open:
            try:
                # Dapatkan hasil parse (contoh: "aa.restart" atau "A9")
                parsed = parse_command(data_keluar)
                
                # Tampilkan hasil parse di layar (tidak dikirim)
                print(f"[Sistem] Konversi Kode : {parsed}")
                
                # Pisahkan shortcode dengan parameter
                parsed_parts = parsed.split(".")
                shortcode_hex = parsed_parts[0] # Mengambil "aa"
                
                # Ubah shortcode menjadi 1 byte (biner murni)
                try:
                    byte_data = bytes.fromhex(shortcode_hex)
                except ValueError:
                    # Akan error jika command tidak ada di map dan menghasilkan misal ">target"
                    print(f"[Error] '{shortcode_hex}' bukan format Heksadesimal 2 karakter yang valid!")
                    print("[TX] > ", end="", flush=True)
                    continue

                # Rangkai payload dan tampilkan visualisasi Hex ke layar
                if len(parsed_parts) > 1:
                    # Gabungkan kembali parameter (misal: ".restart")
                    params_str = "." + ".".join(parsed_parts[1:])
                    
                    # Tambahan Print untuk visualisasi Hex dan Parameter
                    print(f"[Sistem] Mengirim      : 0x{shortcode_hex.lower()}{params_str}")
                    
                    # byte_data berupa biner, params_str di-encode ke biner, lalu digabung
                    payload = byte_data + params_str.encode('utf-8') + b"\r\n"
                else:
                    # Tambahan Print untuk visualisasi Hex (tanpa parameter)
                    print(f"[Sistem] Mengirim      : 0x{shortcode_hex.lower()}")
                    
                    # Jika tidak ada parameter, hanya kirim byte heksanya saja
                    payload = byte_data + b"\r\n"
                
                # Kirim data biner campuran tersebut ke serial
                ser.write(payload)
                
                print("[TX] > ", end="", flush=True)
            except (serial.SerialException, OSError):
                print("[Sistem] Gagal mengirim data. Menunggu koneksi pulih...")
        else:
            print("[Sistem] Tidak ada koneksi. Data tidak terkirim.")
            print("[TX] > ", end="", flush=True)
            
except KeyboardInterrupt:
    print("\n[Sistem] Program dihentikan paksa (Ctrl+C).")
finally:
    if ser and ser.is_open:
        ser.close()
    print("[Sistem] Port serial telah ditutup.")