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
#define GetGeneralConfig(key) cfg.key = ini[SECTION][#key]
#define GetLightConfig(key) cfg.light.key = ini[SECTION][#key]

using namespace ModUtils;
using namespace std;

struct offsets
{
    int color;
    int sp_color;
    int radius;
    int intensity;
    int position;
} offsets;

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

        string intensity; // 1.25

        string x; // 0.0
        string y; // 0.0
        string z; // 0.0

        // deprecated
        string luminous_intensity; // 1.25
        string x_pos; // 0.0
        string y_pos; // 0.0
        string z_pos; // 0.0
    } light;

    string load_delay;

    string start_address;
    string region_size;
    string protect;
    string type;
    string filename;
} cfg;

uintptr_t pBaseAddress = 0;

uintptr_t* ptrFXRBaseAddress = nullptr;

void GetOffsets()
{
    // The offset of the second "55 9D 04 00" in the FXR is different if the FXR was written by SoulsFormats.
    // Others offsets are also different in this case.

    // FXRBaseAddress+0x1D84 = 55 9D 04 00
    // NON modded FXR
    if (*(uint32_t*)((unsigned char*)ptrFXRBaseAddress + 0x1D84) == (uint32_t)0x00049D55)
    {
        offsets.color = 0x1CDC;
        offsets.position = 0x1C1C;
        Log("FXR seems to be original.");
    }
    // FXRBaseAddress+0x1C0C = 55 9D 04 00
    // modded FXR
    else if (*(uint32_t*)((unsigned char*)ptrFXRBaseAddress + 0x1C0C) == (uint32_t)0x00049D55)
    {
        offsets.color = 0x1D50;
        offsets.position = 0x1C58;
        Log("FXR seems to be modded by a third-party mod.");
    }
    else
    {
        offsets.color = 0x1CDC;
        offsets.position = 0x1C1C;
        Log("Unable to find the second signature in memory. The mod may not work properly. Are you using a third-party mod?");
    }

    offsets.sp_color = offsets.color + 0x10;
    offsets.radius = offsets.color + 0x20;
    offsets.intensity = offsets.color + 0x30;
}

void WriteMem(string value, string paramName, int offset)
{
    // replace "cfg.light." by "";
    paramName = paramName.replace(0, 10, "");

    if (value != "")
    {
        Log("%s value=%s offset=0x%X", paramName.c_str(), value.c_str(), offset);
        *(float*)((unsigned char*)ptrFXRBaseAddress + offset) = stof(value);
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

    uintptr_t startAddress = (cfg.start_address != "") ? stoull(cfg.start_address, 0, 16) : 0;
    uintptr_t regionSize = (cfg.region_size != "") ? stoull(cfg.region_size, 0, 16) : 0;
    DWORD protect = (cfg.protect != "") ? stoul(cfg.protect, 0, 16) : PAGE_READWRITE;
    DWORD type = (cfg.type != "") ? stoul(cfg.type, 0, 16) : MEM_PRIVATE;

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

    vector<string> keys{ "load_delay", "start_address", "region_size", "protect", "type", "filename" };

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
    GetGeneralConfig(filename);

    if (cfg.filename.empty())
    {
        cfg.filename = "custom-lantern.ini";
        ini[SECTION]["filename"] = cfg.filename;
    }

    // write ini
    file.write(ini);
}

void ReadLightConfig(string path)
{
    //
    // custom-lantern.ini (by default)
    //

    const string SECTION = "config";

    // create a file instance
    mINI::INIFile file(path + "\\" + cfg.filename);

    // create a data structure
    mINI::INIStructure ini;

    vector<string> keys{
        "red", "green", "blue", "alpha",
        "sp_red", "sp_green", "sp_blue", "sp_alpha",
        "radius", "intensity",
        "x", "y", "z" };

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
    GetLightConfig(intensity);
    GetLightConfig(x);
    GetLightConfig(y);
    GetLightConfig(z);

    // write ini
    file.write(ini);

    // deprecated
    GetLightConfig(luminous_intensity);
    GetLightConfig(x_pos);
    GetLightConfig(y_pos);
    GetLightConfig(z_pos);
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

DWORD WINAPI Patch(LPVOID hModule)
{
    Log("Activating Custom Lantern...");

    ReadConfig();

    unsigned long loadDelay = (cfg.load_delay != "") ? stoul(cfg.load_delay) : 15000;
    Log("Wait %lu ms", loadDelay);
    Sleep(loadDelay);

    DWORD pid = GetCurrentProcessId();
    pBaseAddress = GetProcessBaseAddress(pid);

    Log("Process name: %s", GetModuleName(false).c_str());
    Log("Process ID: %i", pid);
    Log("Process base address: 0x%p", pBaseAddress);

    auto chronoStart = chrono::high_resolution_clock::now();

    ptrFXRBaseAddress = GetFXRBaseAddress();
    if (!ptrFXRBaseAddress)
    {
        RaiseError("Unable to find the signature in memory. Read the full logs in Game\\mods\\CustomLantern\\log.txt");
        CloseLog();

        return 1;
    }

    auto chronoEnd = chrono::high_resolution_clock::now();
    chrono::milliseconds duration = chrono::duration_cast<chrono::milliseconds>(chronoEnd - chronoStart);

    Log("Found signature at: 0x%p in %llu milliseconds", ptrFXRBaseAddress, duration.count());

    GetOffsets();

    Log("Write values in memory if any...");

    // light color
    _WriteMem(cfg.light.red, offsets.color);
    _WriteMem(cfg.light.green, offsets.color + 4);
    _WriteMem(cfg.light.blue, offsets.color + 8);
    _WriteMem(cfg.light.alpha, offsets.color + 12);

    // specular color
    _WriteMem(cfg.light.sp_red, offsets.sp_color);
    _WriteMem(cfg.light.sp_green, offsets.sp_color + 4);
    _WriteMem(cfg.light.sp_blue, offsets.sp_color + 8);
    _WriteMem(cfg.light.sp_alpha, offsets.sp_color + 12);

    // light radius
    _WriteMem(cfg.light.radius, offsets.radius);

    // luminous intensity
    _WriteMem(cfg.light.intensity, offsets.intensity);

    // position
    _WriteMem(cfg.light.x, offsets.position);
    _WriteMem(cfg.light.y, offsets.position + 4);
    _WriteMem(cfg.light.z, offsets.position + 8);

    // deprecated
    if (cfg.light.intensity == "") { _WriteMem(cfg.light.luminous_intensity, 0x1D0C); } // luminous_intensity deprecated
    if (cfg.light.x == "") { _WriteMem(cfg.light.x_pos, 0x1C1C); } // x_pos deprecated
    if (cfg.light.y == "") { _WriteMem(cfg.light.y_pos, 0x1C20); } // y_pos deprecated
    if (cfg.light.z == "") { _WriteMem(cfg.light.z_pos, 0x1C24); } // z_pos deprecated

    Log("Patch applied (^.^)/ ");
    CloseLog();

    // unload DLL
    FreeLibraryAndExitThread((HMODULE)hModule, 0);

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
        HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Patch, hModule, 0, nullptr);
        if (hThread)
        {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
