/*
    Custom Lantern -- Elden Ring 1.8.1
*/

#include <iomanip>
#include <iostream>
#include <string>

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

    string base_address;
    string region_size;
    string protect;
} _g_cfg;

mINI::INIStructure _g_iniCustomLantern;
uintptr_t _g_pBaseAddress = 0;

uintptr_t* _g_ptrFXRBaseAddress = nullptr;

void WriteMem(std::string value, std::string paramName, int offset)
{
    paramName = paramName.replace(0, 7, "");

    if (value != "")
    {
        Log("Write in memory %s value=%s offset=0x%X", paramName.c_str(), value.c_str(), offset);
        *(float*)((unsigned char*)_g_ptrFXRBaseAddress + offset) = std::stof(value);
    }
}

uintptr_t* GetFXRBaseAddress()
{
    // example:
    // [pointer_offset_start_address] = 3ABC010
    // [process_base_address]+3ABC010 -> 7FF35AB00000
    // [start_address] = 7FF35AB00000

    const unsigned short int LOOP_LIMIT = 9999;

    uintptr_t currentAddress = 0;
    MEMORY_BASIC_INFORMATION mbi;

    // signature to find
    //   46 58 52 00 00 00 05 00
    //   01 00 00 00 55 9D 04 00

    /*				int16 int16 int32       int32
        F  X  R  \0 00 00 ver.  \1          ID
        46 58 52 00 00 00 05 00 01 00 00 00 55 9D 04 00
    */

    // effect ID = 55 9D 04 00 = 0x00049D55 = 302421

    for (size_t i = 0; i < LOOP_LIMIT; i++)
    {
        if (!VirtualQuery((LPCVOID)currentAddress, &mbi, sizeof(mbi)))
        {
            DWORD error = GetLastError();
            if (error == ERROR_INVALID_PARAMETER)
            {
                Log("Reached end of scannable memory.");
            }
            else
            {
                Log("VirtualQuery failed, error code: %i.", error);
            }
            break;
        }

        uintptr_t protection = (uintptr_t)mbi.Protect;
        uintptr_t state = (uintptr_t)mbi.State;

        if ((mbi.Protect == PAGE_READWRITE) && (mbi.State == MEM_COMMIT))
        {
            uintptr_t startAddress = (uintptr_t)mbi.BaseAddress;
            uintptr_t endAddress = startAddress + mbi.RegionSize - 16;
            uintptr_t currentAddress = startAddress;

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

    vector<string> keys{ "load_delay", "base_address", "region_size", "protect" };

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
    GetGeneralConfig(base_address);
    GetGeneralConfig(region_size);
    GetGeneralConfig(protect);

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
    Log("Read configuration...");

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

    if (_g_cfg.load_delay != "")
    {
        unsigned long loadDelay = stol(_g_cfg.load_delay);
        Log("Wait %lu ms...", loadDelay);
        Sleep(loadDelay);
    }

    DWORD pid = GetCurrentProcessId();
    _g_pBaseAddress = GetProcessBaseAddress(pid);

    Log("Process name: %s", GetModuleName(false).c_str());
    Log("Process ID: %i", pid);
    Log("Process base address: 0x%p", _g_pBaseAddress);

    _g_ptrFXRBaseAddress = GetFXRBaseAddress();
    if (!_g_ptrFXRBaseAddress)
    {
        RaiseError("Unable to find the FXR base address.");
        CloseLog();

        return 1;
    }

    Log("FXR base address: 0x%p", _g_ptrFXRBaseAddress);
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

    Log("Done.");
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
