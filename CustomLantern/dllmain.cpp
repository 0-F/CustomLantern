/*
    Custom Lantern -- Elden Ring 1.8.1
*/

#include <iomanip>
#include <iostream>

#include "pch.h"
#include "ModUtils.h" // https://github.com/techiew/EldenRingMods/blob/master/ModUtils.h
#include "ini.h" // https://github.com/pulzed/mINI

#define _WriteMem(param, offset) WriteMem(param, #param, offset)

using namespace ModUtils;
using namespace std;

struct cfg {
    struct light // comments = default values
    {
        struct color
        {
            string red;   // 1.0
            string green; // 0.6313726
            string blue;  // 0.36862746
            string alpha; // 1.0

            string sp_red;   // 1.0
            string sp_green; // 0.6313726
            string sp_blue;  // 0.36862746
            string sp_alpha; // 1.0

        } color;

        string radius; // 16.0

        struct position
        {
            string x; // 0.0
            string y; // 0.0
            string z; // 0.0
        } position;

        struct rotation
        {
            string x; // 0.0
            string y; // 0.0
            string z; // 0.0
        } rotation;

        string luminous_intensity; // 1.25
    } light;

    unsigned int loadDelay = 0;
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

uintptr_t* GetFXRBaseAddress
() {
    uintptr_t startAddress = *(uintptr_t*)(_g_pBaseAddress + 0x3ACC460);
    uintptr_t endAddress = startAddress + 0xA0000000 - 16;
    uintptr_t currentAddress = startAddress;

    uint64_t* ptrCurrAddr = (uintptr_t*)currentAddress;

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

    ptrCurrAddr = nullptr;

    return nullptr;
}

void ReadConfig()
{
    Log("Read configuration...");

    string path = GetModuleFolderPath();

    //
    // config.ini
    //

    // create a file instance
    mINI::INIFile fileCfg(path + "\\config.ini");

    // create a data structure
    mINI::INIStructure iniCfg;

    if (!fileCfg.read(iniCfg))
    {
        // populate the structure
        iniCfg["config"]["load_delay"] = "10000";

        // generate an INI file (overwrites any previous file)
        fileCfg.generate(iniCfg);
    }

    string loadDelay = iniCfg["config"]["load_delay"];
    if (loadDelay != "")
    {
        _g_cfg.loadDelay = stoi(loadDelay);
    }
    else
    {
        _g_cfg.loadDelay = 10000;
    }

    //
    // custom-lantern.ini
    //

    mINI::INIFile fileCL(path + "\\custom-lantern.ini");

    mINI::INIStructure iniCL;

    if (!fileCL.read(iniCL))
    {
        iniCL["config"]["red"] = "";
        iniCL["config"]["green"] = "";
        iniCL["config"]["blue"] = "";
        iniCL["config"]["alpha"] = "";

        iniCL["config"]["sp_red"] = "";
        iniCL["config"]["sp_green"] = "";
        iniCL["config"]["sp_blue"] = "";
        iniCL["config"]["sp_alpha"] = "";

        iniCL["config"]["radius"] = "";

        iniCL["config"]["luminous_intensity"] = "";

        iniCL["config"]["x_pos"] = "";
        iniCL["config"]["y_pos"] = "";
        iniCL["config"]["z_pos"] = "";

        iniCL["config"]["x_rot"] = "";
        iniCL["config"]["y_rot"] = "";
        iniCL["config"]["z_rot"] = "";

        fileCL.generate(iniCL);
    }

    // light color
    _g_cfg.light.color.red = iniCL["config"]["red"];
    _g_cfg.light.color.green = iniCL["config"]["green"];
    _g_cfg.light.color.blue = iniCL["config"]["blue"];
    _g_cfg.light.color.alpha = iniCL["config"]["alpha"];

    // specular color
    _g_cfg.light.color.sp_red = iniCL["config"]["sp_red"];
    _g_cfg.light.color.sp_green = iniCL["config"]["sp_green"];
    _g_cfg.light.color.sp_blue = iniCL["config"]["sp_blue"];
    _g_cfg.light.color.sp_alpha = iniCL["config"]["sp_alpha"];

    // light radius
    _g_cfg.light.radius = iniCL["config"]["radius"];

    // luminous intensity
    _g_cfg.light.luminous_intensity = iniCL["config"]["luminous_intensity"];

    // position
    _g_cfg.light.position.x = iniCL["config"]["x_pos"];
    _g_cfg.light.position.y = iniCL["config"]["y_pos"];
    _g_cfg.light.position.z = iniCL["config"]["z_pos"];

    // rotation
    _g_cfg.light.rotation.x = iniCL["config"]["x_rot"];
    _g_cfg.light.rotation.y = iniCL["config"]["y_rot"];
    _g_cfg.light.rotation.z = iniCL["config"]["z_rot"];

    fileCL.write(iniCL);
}

DWORD WINAPI Patch(LPVOID lpParam)
{
    Log("Activating Custom Lantern...");

    ReadConfig();

    Log("Wait %ims", _g_cfg.loadDelay);
    Sleep(_g_cfg.loadDelay);

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
    _WriteMem(_g_cfg.light.color.red, 0x1CDC);
    _WriteMem(_g_cfg.light.color.green, 0x1CE0);
    _WriteMem(_g_cfg.light.color.blue, 0x1CE4);
    _WriteMem(_g_cfg.light.color.alpha, 0x1CE8);

    // specular color
    _WriteMem(_g_cfg.light.color.sp_red, 0x1CEC);
    _WriteMem(_g_cfg.light.color.sp_green, 0x1CF0);
    _WriteMem(_g_cfg.light.color.sp_blue, 0x1CF4);
    _WriteMem(_g_cfg.light.color.sp_alpha, 0x1CF8);

    // light radius
    _WriteMem(_g_cfg.light.radius, 0x1CFC);

    // luminous intensity
    _WriteMem(_g_cfg.light.luminous_intensity, 0x1D0C);

    // position
    _WriteMem(_g_cfg.light.position.x, 0x1C1C);
    _WriteMem(_g_cfg.light.position.y, 0x1C20);
    _WriteMem(_g_cfg.light.position.z, 0x1C24);

    // rotation
    _WriteMem(_g_cfg.light.rotation.x, 0x1C28);
    _WriteMem(_g_cfg.light.rotation.y, 0x1C2C);
    _WriteMem(_g_cfg.light.rotation.z, 0x1C30);

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
