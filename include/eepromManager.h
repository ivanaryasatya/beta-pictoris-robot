#pragma once
#include <Arduino.h>
#include <EEPROM.h>

class EEPROMManager {

private:
    int size;
    int findEnd() {
        for(int i=0;i<size;i++){
            if(EEPROM.read(i)==0xFF) return i;
        }
        return size;
    }

    void compact(){
        int writeIndex = 0;
        for(int i=0;i<size;i++){
            byte c = EEPROM.read(i);
            if(c != 0xFF){
                EEPROM.write(writeIndex++,c);
            }

        }

        for(int i=writeIndex;i<size;i++){
            EEPROM.write(i,0xFF);
        }

        EEPROM.commit();
    }

public:

    void begin(int eepromSize){
        size = eepromSize;
        EEPROM.begin(size);
    }

    /* =========================
        SAVE DATA
    ========================= */
    bool save(char pointer,String data){
        // Hindari menulis ulang jika datanya sudah sama persis (menghemat usia/write cycle EEPROM)
        if(get(pointer) == data){
            return true; 
        }

        // Cek apakah pointer (key) sudah ada
        bool isExist = false;
        bool isStart = true;
        for(int i=0;i<size;i++){
            char c = EEPROM.read(i);
            if(c == 0xFF) break; // Berhenti jika blok kosong

            if(isStart && c == pointer){
                isExist = true;
                break;
            }
            isStart = (c == '\n');
        }

        // Jika ada, hapus data lama terlebih dahulu (mekanisme replace/edit)
        if(isExist){
            remove(pointer);
        }

        String payload = String(pointer);
        payload += data;
        payload += "\n";
        int end = findEnd();
        if(end + payload.length() >= size){
            compact();
            end = findEnd();
            if(end + payload.length() >= size){
                return false;
            }
        }
        for(int i=0;i<payload.length();i++){
            EEPROM.write(end+i,payload[i]);
        }
        EEPROM.commit();
        return true;
    }
    /* =========================
        GET DATA BY POINTER
    ========================= */
    String get(char pointer){
        String buffer="";
        bool reading=false;
        bool isStart=true;
        
        for(int i=0;i<size;i++){
            char c = EEPROM.read(i);
            if(c == 0xFF) break;

            if(isStart && c==pointer){
                buffer="";
                reading=true;
                isStart = false; // Hindari deteksi ulang
                continue;
            }
            
            if(reading){
                if(c=='\n'){
                    return buffer; // Ambil data pertama yang sesuai
                }
                buffer += c;
            }
            
            isStart = (c == '\n');
        }
        return "";
    }

    /* =========================
        DELETE DATA
    ========================= */

    void remove(char pointer){
        bool deleting=false;
        bool isStart=true;
        
        for(int i=0; i < size; i++){
            char c = EEPROM.read(i);
            if(c == 0xFF) break; // Tidak ada data lebih lanjut

            // Mulai menghapus jika ini awal record dan counternya pas
            if(isStart && c==pointer){
                deleting=true;
            }
            
            if(deleting){
                EEPROM.write(i,0xFF); // Timpa keseluruhan data dengan 0xFF
            }
            
            if(c=='\n'){
                deleting=false; // Berhenti menghapus record ini
                isStart=true;   // Set index berikutnya sebagai awal record berpotensi
            } else {
                isStart=false;
            }
        }
        EEPROM.commit();
        compact();
    }
    /* =========================
        GET ALL DATA
    ========================= */

    String getAll(){
        String all="";
        for(int i=0;i<size;i++){
            char c = EEPROM.read(i);
            if(c!=0xFF){
                all += c;
            }
        }
        return all;
    }
    /* =========================
        EEPROM INFO
    ========================= */
    int used(){
        return findEnd();
    }
    int free(){
        return size - findEnd();
    }
};