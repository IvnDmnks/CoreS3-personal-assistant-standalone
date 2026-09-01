#pragma once
#include "globals.h"

//* --- TEXT TO SPEECH ---
class AudioFileSourceWiFiClient : public AudioFileSource {
    private: 
        WiFiClientSecure *client;
    public:
        AudioFileSourceWiFiClient(WiFiClientSecure *c) : client(c) {}
        virtual bool open(const char* filename) override { return true; }
        virtual uint32_t read(void *data, uint32_t len) override {
            if(!client) return 0;
            return client->read((uint8_t*)data, len);
        }
        virtual bool seek(int32_t offset, int direction) override { return false; }
        virtual bool close() override { return true; }
        virtual bool isOpen() override { return client && (client->connected() || client->available()); }
        virtual uint32_t getSize() override { return 0; }
        virtual uint32_t getPos() override { return 0; }
};

void playSDSound(const char* fileName);
void playTTS(String text);