import serial
import serial.tools.list_ports
import threading
import csv
import os
import sys
import time
from datetime import datetime

# System Terminal Variables untuk non-blocking input
print_lock = threading.Lock()
current_input = ""
ser = None
is_connected = False

def safe_print(msg):
    global current_input
    with print_lock:
        # Hapus baris input saat ini
        sys.stdout.write("\r" + " " * (3 + len(current_input)) + "\r")
        # Print log RX / TX
        sys.stdout.write(msg + "\n")
        # Cetak ulang prompt
        sys.stdout.write(">> " + current_input)
        sys.stdout.flush()

# --- PERBAIKAN LOKASI FOLDER ---
# Mencari folder 'tools' secara absolut
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Set folder log ke 'beta_pictoris/serialLog'
LOG_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "serialLog"))

def get_timestamp_full():
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]
    
def get_filename_timestamp():
    """Format waktu untuk nama file: YYYY-MM-DD_HH-mm-ss"""
    return datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

def list_active_ports():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

def write_to_csv(file_path, time, mcu_time, device, msg_type, content):
    """Fungsi untuk mencatat data ke file CSV"""
    with open(file_path, mode='a', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow([time, mcu_time, device, msg_type, content])

def rx_thread(port, baudrate, log_file):
    """Thread untuk menjaga koneksi dan menangani data masuk (RX)"""
    global ser, is_connected
    while True:
        if not is_connected:
            try:
                ser = serial.Serial(port, baudrate, timeout=1)
                is_connected = True
                msg = f"[SYSTEM] BERHASIL tersambung ke {port} @ {baudrate} bps."
                if os.name == 'nt':
                    safe_print(msg)
                else:
                    print(f"\r{msg}\n>> ", end="", flush=True)
            except serial.SerialException:
                time.sleep(2)
                continue

        try:
            if ser and ser.in_waiting > 0:
                raw_data = ser.readline().decode('utf-8', errors='replace').strip()
                if raw_data:
                    timestamp = get_timestamp_full()

                    # Nilai default
                    mcu_time = "N/A"
                    device = "Unknown"
                    payload = raw_data

                    # --- Parsing ---
                    # Format: E12345-message atau N12345-message
                    if (raw_data.startswith('E') or raw_data.startswith('N')) and '-' in raw_data:
                        try:
                            separator_index = raw_data.find('-')
                            device_char = raw_data[0]
                            time_str = raw_data[1:separator_index]
                            
                            int(time_str) # Validasi bahwa ini adalah angka
                            
                            # Jika validasi berhasil, perbarui nilai
                            device = "ESP32" if device_char == 'E' else "NANO"
                            mcu_time = time_str
                            payload = raw_data[separator_index+1:]
                        except (ValueError, IndexError):
                            # Jika parsing gagal, biarkan nilai default.
                            pass
                    
                    # --- Tampilkan di Terminal ---
                    mcu_time_display = f"[{mcu_time}ms]" if mcu_time != "N/A" else "[N/A]"
                    msg = f"[{timestamp}] {mcu_time_display} [{device}] [RX] -> {payload}"
                    if os.name == 'nt':
                        safe_print(msg)
                    else:
                        print(f"\r{msg}")
                        print(">> ", end="", flush=True)
                    # --- Catat ke CSV ---
                    write_to_csv(log_file, timestamp, mcu_time, device, "RX", payload)
        except (serial.SerialException, OSError, AttributeError):
            is_connected = False
            msg = "[SYSTEM] KONEKSI TERPUTUS! Mencoba menyambung kembali..."
            if os.name == 'nt':
                safe_print(msg)
            else:
                print(f"\r{msg}\n>> ", end="", flush=True)
            if ser:
                ser.close()
            time.sleep(1)

def main():
    print("=== Python Serial Terminal & CSV Logger ===")
    
    # Pastikan folder log ada
    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR)
        print(f"[SYSTEM] Folder log dibuat: {LOG_DIR}")

    # Pilih Port
    ports = list_active_ports()
    print(f"Port tersedia: {ports}")
    port_input = input("Masukkan COM Port (Default COM6): ").upper()
    port = port_input if port_input else "COM6"
    
    # Pilih Baud Rate
    baud_input = input("Masukkan Baud Rate (Default 115200): ")
    baudrate = int(baud_input) if baud_input else 115200

    try:
        # Buat file CSV baru saat program dimulai
        file_name = f"{get_filename_timestamp()}.csv"
        full_log_path = os.path.join(LOG_DIR, file_name)
        
        # Tulis Header CSV
        with open(full_log_path, mode='w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(["Waktu PC", "Waktu MCU (ms)", "Perangkat", "Jenis Komunikasi", "Pesan"])
        
        print(f"\n[SYSTEM] Mencari {port}... (Program tidak akan berhenti jika terputus)")
        print(f"[SYSTEM] Logging ke: {full_log_path}")
        print("[SYSTEM] Ketik pesan lalu Enter. Ketik 'exit' untuk keluar.\n")

        # Jalankan thread pembaca (RX) & penjaga koneksi
        thread = threading.Thread(target=rx_thread, args=(port, baudrate, full_log_path), daemon=True)
        thread.start()

        # Loop utama pengirim (TX)
        if os.name == 'nt':
            import msvcrt
            global current_input
            current_input = ""
            sys.stdout.write(">> ")
            sys.stdout.flush()
            
            while True:
                if msvcrt.kbhit():
                    char = msvcrt.getch()
                    if char in (b'\x03', b'\x1a'): # Ctrl+C, Ctrl+Z
                        raise KeyboardInterrupt
                    elif char == b'\r':
                        with print_lock:
                            sys.stdout.write("\n")
                            tx_data = current_input
                            current_input = ""
                        
                        if tx_data.lower() == 'exit':
                            break
                        
                        if tx_data:
                            if is_connected and ser and ser.is_open:
                                try:
                                    ser.write((tx_data + '\n').encode('utf-8'))
                                    timestamp = get_timestamp_full()
                                    msg = f"[{timestamp}] [TX] Sent: {tx_data}"
                                    safe_print(msg)
                                    write_to_csv(full_log_path, timestamp, "N/A", "PC", "TX", tx_data)
                                except (serial.SerialException, OSError):
                                    safe_print("[SYSTEM] Gagal mengirim data. Koneksi terputus!")
                            else:
                                safe_print("[SYSTEM] Tidak ada koneksi. Data tidak terkirim.")
                        else:
                            with print_lock:
                                sys.stdout.write(">> ")
                                sys.stdout.flush()
                    elif char in (b'\x08', b'\x7f'): # Backspace
                        with print_lock:
                            if len(current_input) > 0:
                                current_input = current_input[:-1]
                                sys.stdout.write("\b \b")
                                sys.stdout.flush()
                    elif char in (b'\xe0', b'\x00'): # special keys like arrows
                        msvcrt.getch()
                    else:
                        try:
                            # Tampilkan karakter yang diketik
                            char_str = char.decode('utf-8')
                            if char_str.isprintable():
                                with print_lock:
                                    current_input += char_str
                                    sys.stdout.write(char_str)
                                    sys.stdout.flush()
                        except UnicodeDecodeError:
                            pass
                else:
                    time.sleep(0.01)
        else:
            while True:
                tx_data = input(">> ")
                
                if tx_data.lower() == 'exit':
                    break
                
                if tx_data:
                    if is_connected and ser and ser.is_open:
                        try:
                            ser.write((tx_data + '\n').encode('utf-8'))
                            timestamp = get_timestamp_full()
                            print(f"[{timestamp}] [TX] Sent: {tx_data}")
                            write_to_csv(full_log_path, timestamp, "N/A", "PC", "TX", tx_data)
                        except (serial.SerialException, OSError):
                            print("[SYSTEM] Gagal mengirim data. Koneksi terputus!")
                    else:
                        print("[SYSTEM] Tidak ada koneksi. Data tidak terkirim.")

    except KeyboardInterrupt:
        print(f"\n[SYSTEM] Menutup koneksi...")
    finally:
        if ser and ser.is_open:
            ser.close()
        print("[SYSTEM] Selesai.")

if __name__ == "__main__":
    main()