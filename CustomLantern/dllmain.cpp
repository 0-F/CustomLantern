/*
    Custom Lantern -- Elden Ring 1.8.1
*/

#include <iomanip>
#include <iostream>
#include <string>
#include <chrono>
#include <windows.h>
#include "pch.h"
#include "ModUtils.h" // https://github.com/techiew/EldenRingMods/blob/master/ModUtils.h
#include "ini.h" // https://github.com/pulzed/mINI

#define _WriteMem(param, offset) WriteMem(param, #param, offset)
#define GetGeneralConfig(key) _g_cfg.key = ini[SECTION][#key]
#define GetLightConfig(key) _g_cfg.light.key = ini[SECTION][#key]

using namespace ModUtils;
using namespace std;

struct cfg {
    struct light // comments = default values
    {
        string red;   // 1.0
        string green; // 0.6313726
        string blue;  // 0.36862746
        string alpha; // 1.0

        string sp_red;   // 1.0
        string sp_green; // 0.6313726
        string sp_blue;  // 0.36862746
        string sp_alpha; // 1.0

        string radius; // 16.0

        string luminous_intensity; // 1.25

        string x_pos; // 0.0
        string y_pos; // 0.0
        string z_pos; // 0.0

        string x_rot; // 0.0
        string y_rot; // 0.0
        string z_rot; // 0.0

    } light;

    string load_delay;

    string start_address;
    string region_size;
    string protect;
    string type;
} _g_cfg;

mINI::INIStructure _g_iniCustomLantern;
uintptr_t _g_pBaseAddress = 0;

uintptr_t* _g_ptrFXRBaseAddress = nullptr;

void WriteMem(string value, string paramName, int offset)
{
    paramName = paramName.replace(0, 7, "");

    if (value != "")
    {
        Log("%s value=%s offset=0x%X", paramName.c_str(), value.c_str(), offset);
        *(float*)((unsigned char*)_g_ptrFXRBaseAddress + offset) = stof(value);
    }
}

uintptr_t* GetFXRBaseAddress()
{
    /*
        signature to find
          46 58 52 00 00 00 05 00
          01 00 00 00 55 9D 04 00

                    int16 int16 int32       int32
        F  X  R  \0 00 00 ver.  \1          ID
        46 58 52 00 00 00 05 00 01 00 00 00 55 9D 04 00

        effect ID = 55 9D 04 00 = 0x00049D55 = 302421
    */

    MEMORY_BASIC_INFORMATION mbi;

    uintptr_t startAddress = (_g_cfg.start_address != "") ? stoull(_g_cfg.start_address, 0, 16) : 0;
    uintptr_t regionSize = (_g_cfg.region_size != "") ? stoull(_g_cfg.region_size, 0, 16) : 0;
    DWORD protect = (_g_cfg.protect != "") ? stoul(_g_cfg.protect, 0, 16) : PAGE_READWRITE;
    DWORD type = (_g_cfg.type != "") ? stoul(_g_cfg.type, 0, 16) : MEM_PRIVATE;

    Log("Find the signature in memory");
    Log("start_address=0x%p", startAddress);
    Log("region_size=0x%p", regionSize);
    Log("protect=0x%X", protect);
    Log("type=0x%X", type);

    uintptr_t currentAddress = startAddress;

    for (size_t i = 0; i < 0xFFFF; i++)
    {
        if (!VirtualQuery((LPCVOID)currentAddress, &mbi, sizeof(mbi)))
        {
            DWORD error = GetLastError();
            if (error == ERROR_INVALID_PARAMETER) { break; }

            RaiseError("VirtualQuery() failed with error " + error);
            break;
        }

        if ((protect == 0 || mbi.Protect == protect) &&
            (type == 0 || mbi.Type == type) &&
            (regionSize == 0 || mbi.RegionSize == regionSize) &&
            (mbi.State == MEM_COMMIT))
        {
            uintptr_t baseAddress = (uintptr_t)mbi.BaseAddress;
            uintptr_t endAddress = baseAddress + mbi.RegionSize - 16;
            uintptr_t currentAddress = baseAddress;

            uintptr_t* ptrCurrAddr = (uintptr_t*)currentAddress;

            // find the signature in memory
            while ((uintptr_t)ptrCurrAddr < endAddress)
            {
                if (*ptrCurrAddr == (uint64_t)0x0005000000525846)
                {
                    if (*(ptrCurrAddr + 1) == (uint64_t)0x00049D5500000001)
                    {
                        return ptrCurrAddr;
                    }
                }
                ptrCurrAddr++;
            }
        }

        currentAddress += mbi.RegionSize;
    }

    return nullptr;
}

void ReadGeneralConfig(string path)
{
    //
    // config.ini
    //

    const string SECTION = "config";

    // create a file instance
    mINI::INIFile file(path + "\\config.ini");

    // create a data structure
    mINI::INIStructure ini;

    vector<string> keys{ "load_delay", "start_address", "region_size", "protect", "type" };

    if (!file.read(ini))
    {
        for (size_t i = 0; i < keys.size(); i++)
        {
            // populate the structure
            ini[SECTION][keys[i]] = "";

            // generate an INI file (overwrites any previous file)
            file.generate(ini);
        }
    }

    // read values
    GetGeneralConfig(load_delay);
    GetGeneralConfig(start_address);
    GetGeneralConfig(region_size);
    GetGeneralConfig(protect);
    GetGeneralConfig(type);

    // write ini
    file.write(ini);
}

void ReadLightConfig(string path)
{
    //
    // custom-lantern.ini
    //

    const string SECTION = "config";

    // create a file instance
    mINI::INIFile file(path + "\\custom-lantern.ini");

    // create a data structure
    mINI::INIStructure ini;

    vector<string> keys{
        "red", "green", "blue", "alpha",
        "sp_red", "sp_green", "sp_blue", "sp_alpha",
        "radius", "luminous_intensity",
        "x_pos", "y_pos", "z_pos",
        "x_rot", "y_rot", "z_rot" };

    if (!file.read(ini))
    {
        for (size_t i = 0; i < keys.size(); i++)
        {
            // populate the structure
            ini[SECTION][keys[i]] = "";

            // generate an INI file (overwrites any previous file)
            file.generate(ini);
        }
    }

    // read values
    GetLightConfig(red);
    GetLightConfig(green);
    GetLightConfig(blue);
    GetLightConfig(alpha);
    GetLightConfig(sp_red);
    GetLightConfig(sp_green);
    GetLightConfig(sp_blue);
    GetLightConfig(sp_alpha);
    GetLightConfig(radius);
    GetLightConfig(luminous_intensity);
    GetLightConfig(x_pos);
    GetLightConfig(y_pos);
    GetLightConfig(z_pos);
    GetLightConfig(x_rot);
    GetLightConfig(y_rot);
    GetLightConfig(z_rot);

    // write ini
    file.write(ini);
}

void ReadConfig()
{
    Log("Read configuration");

    const string CONFIG = "config";

    string path = GetModuleFolderPath();

    // config.ini
    ReadGeneralConfig(path);

    // custom-lantern.ini
    ReadLightConfig(path);
}

DWORD WINAPI Patch(LPVOID lpParam)
{
    Log("Activating Custom Lantern...");

    ReadConfig();

    unsigned long loadDelay = (_g_cfg.load_delay != "") ? stoul(_g_cfg.load_delay) : 15000;
    Log("Wait %lu ms", loadDelay);
    Sleep(loadDelay);

    DWORD pid = GetCurrentProcessId();
    _g_pBaseAddress = GetProcessBaseAddress(pid);

    Log("Process name: %s", GetModuleName(false).c_str());
    Log("Process ID: %i", pid);
    Log("Process base address: 0x%p", _g_pBaseAddress);

    auto chronoStart = chrono::high_resolution_clock::now();

    _g_ptrFXRBaseAddress = GetFXRBaseAddress();
    if (!_g_ptrFXRBaseAddress)
    {
        RaiseError("Unable to find the signature in memory");
        CloseLog();

        return 1;
    }

    auto chronoEnd = chrono::high_resolution_clock::now();
    chrono::milliseconds duration = chrono::duration_cast<chrono::milliseconds>(chronoEnd - chronoStart);

    Log("Found signature at: 0x%p in %llu milliseconds", _g_ptrFXRBaseAddress, duration.count());
    Log("Write values in memory if any...");

    // light color
    _WriteMem(_g_cfg.light.red, 0x1CDC);
    _WriteMem(_g_cfg.light.green, 0x1CE0);
    _WriteMem(_g_cfg.light.blue, 0x1CE4);
    _WriteMem(_g_cfg.light.alpha, 0x1CE8);

    // specular color
    _WriteMem(_g_cfg.light.sp_red, 0x1CEC);
    _WriteMem(_g_cfg.light.sp_green, 0x1CF0);
    _WriteMem(_g_cfg.light.sp_blue, 0x1CF4);
    _WriteMem(_g_cfg.light.sp_alpha, 0x1CF8);

    // light radius
    _WriteMem(_g_cfg.light.radius, 0x1CFC);

    // luminous intensity
    _WriteMem(_g_cfg.light.luminous_intensity, 0x1D0C);

    // position
    _WriteMem(_g_cfg.light.x_pos, 0x1C1C);
    _WriteMem(_g_cfg.light.y_pos, 0x1C20);
    _WriteMem(_g_cfg.light.z_pos, 0x1C24);

    // rotation
    _WriteMem(_g_cfg.light.x_rot, 0x1C28);
    _WriteMem(_g_cfg.light.y_rot, 0x1C2C);
    _WriteMem(_g_cfg.light.z_rot, 0x1C30);

    Log("Patch applied (^.^)/ ");
    CloseLog();

    return 0;
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Patch, 0, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
