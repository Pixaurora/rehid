#pragma once
#include <3ds.h>

struct KeyObject
{
    uint32_t oldkey; // Old Key Combo
    uint32_t newkey; // New Key Combo
};

struct TouchObject
{
    uint32_t key; 
    uint16_t x;
    uint16_t y; 
};

struct key_s
{
    char key[10];
    uint32_t val;
};

class Remapper
{
    public:
        void GenerateFileLocation();
        uint32_t Remap(uint32_t hidtstate);
        Result ReadConfigFile();
        void ParseConfigFile();
        KeyObject m_remapkeyobjects[10]; // Support upto 10 remapable key combos
        TouchObject m_remaptouchobjects[10]; // Support upto 10 key > touch binds
        uint8_t m_keyentries;
        uint8_t m_touchentries;
        int16_t m_touchoveridex = 0;
        int16_t m_touchoveridey = 0;

    private:
        char *m_filedata;
        uint64_t m_filedatasize;
        char m_fileloc[40];
};