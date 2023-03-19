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

    unsigned long loadDelay = 0;
    uintptr_t ptrOffsetStartAddress = 0;
    uint64_t maxMemSizeToScan = 0xA0000000;
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

    uintptr_t startAddress = *(uintptr_t*)(_g_pBaseAddress + _g_cfg.ptrOffsetStartAddress);
    uintptr_t endAddress = startAddress + _g_cfg.maxMemSizeToScan - 16;
    uintptr_t currentAddress = startAddress;

    uint64_t* ptrCurrAddr = (uintptr_t*)currentAddress;

    // signature to find
    //   46 58 52 00 00 00 05 00
    //   01 00 00 00 55 9D 04 00

    /*				int16 int16 int32       int32
        F  X  R  \0 00 00 ver.  \1          ID
        46 58 52 00 00 00 05 00 01 00 00 00 55 9D 04 00
    */

    // effect ID = 55 9D 04 00 = 0x00049D55 = 302421

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

    const string CONFIG = "config";

    struct str
    {
        string loadDelay = "load_delay";
        string ptrOffsetStartAddress = "pointer_offset_start_address";
        string maxMemSizeToScan = "max_memory_size_to_scan";

        string red = "red";
        string green = "green";
        string blue = "blue";
        string alpha = "alpha";

        string sp_red = "sp_red";
        string sp_green = "sp_green";
        string sp_blue = "sp_blue";
        string sp_alpha = "sp_alpha";

        string radius = "radius";

        string luminous_intensity = "luminous_intensity";

        string x_pos = "x_pos";
        string y_pos = "y_pos";
        string z_pos = "z_pos";

        string x_rot = "x_rot";
        string y_rot = "y_rot";
        string z_rot = "z_rot";
    } const STR;

    // default config values
    struct dflt
    {
        string loadDelay = "15000";
        string ptrOffsetStartAddress = "0x3ACC4C0";
        string maxMemSizeToScan = "0xA0000000";
    } const DFLT;

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
        iniCfg[CONFIG][STR.loadDelay] = DFLT.loadDelay;
        iniCfg[CONFIG][STR.ptrOffsetStartAddress] = DFLT.ptrOffsetStartAddress;
        iniCfg[CONFIG][STR.maxMemSizeToScan] = DFLT.maxMemSizeToScan;

        // generate an INI file (overwrites any previous file)
        fileCfg.generate(iniCfg);
    }

    // read values
    string strLoadDelay = iniCfg[CONFIG][STR.loadDelay];
    string strPtrOffsetStartAddress = iniCfg[CONFIG][STR.ptrOffsetStartAddress];
    string strMaxMemSizeToScan = iniCfg[CONFIG][STR.maxMemSizeToScan];

    // set default values if needed
    if (strLoadDelay == "") { iniCfg[CONFIG][STR.loadDelay] = DFLT.loadDelay; }
    if (strPtrOffsetStartAddress == "") { iniCfg[CONFIG][STR.ptrOffsetStartAddress] = DFLT.ptrOffsetStartAddress; }
    if (strMaxMemSizeToScan == "") { iniCfg[CONFIG][STR.maxMemSizeToScan] = DFLT.maxMemSizeToScan; }

    // write ini
    fileCfg.write(iniCfg);

    _g_cfg.loadDelay = stol(strLoadDelay);
    _g_cfg.ptrOffsetStartAddress = stoll(strPtrOffsetStartAddress, nullptr, 16); // hexacidemal
    _g_cfg.maxMemSizeToScan = stoll(strMaxMemSizeToScan, nullptr, 16); // hexadecimal

    //
    // custom-lantern.ini
    //

    mINI::INIFile fileCL(path + "\\custom-lantern.ini");

    mINI::INIStructure iniCL;

    if (!fileCL.read(iniCL))
    {
        iniCL[CONFIG][STR.red] = "";
        iniCL[CONFIG][STR.green] = "";
        iniCL[CONFIG][STR.blue] = "";
        iniCL[CONFIG][STR.alpha] = "";

        iniCL[CONFIG][STR.sp_red] = "";
        iniCL[CONFIG][STR.sp_green] = "";
        iniCL[CONFIG][STR.sp_blue] = "";
        iniCL[CONFIG][STR.sp_alpha] = "";

        iniCL[CONFIG][STR.radius] = "";

        iniCL[CONFIG][STR.luminous_intensity] = "";

        iniCL[CONFIG][STR.x_pos] = "";
        iniCL[CONFIG][STR.y_pos] = "";
        iniCL[CONFIG][STR.z_pos] = "";

        iniCL[CONFIG][STR.x_rot] = "";
        iniCL[CONFIG][STR.y_rot] = "";
        iniCL[CONFIG][STR.z_rot] = "";

        fileCL.generate(iniCL);
    }

    // light color
    _g_cfg.light.color.red = iniCL[CONFIG][STR.red];
    _g_cfg.light.color.green = iniCL[CONFIG][STR.green];
    _g_cfg.light.color.blue = iniCL[CONFIG][STR.blue];
    _g_cfg.light.color.alpha = iniCL[CONFIG][STR.alpha];

    // specular color
    _g_cfg.light.color.sp_red = iniCL[CONFIG][STR.sp_red];
    _g_cfg.light.color.sp_green = iniCL[CONFIG][STR.sp_green];
    _g_cfg.light.color.sp_blue = iniCL[CONFIG][STR.sp_blue];
    _g_cfg.light.color.sp_alpha = iniCL[CONFIG][STR.sp_alpha];

    // light radius
    _g_cfg.light.radius = iniCL[CONFIG][STR.radius];

    // luminous intensity
    _g_cfg.light.luminous_intensity = iniCL[CONFIG][STR.luminous_intensity];

    // position
    _g_cfg.light.position.x = iniCL[CONFIG][STR.x_pos];
    _g_cfg.light.position.y = iniCL[CONFIG][STR.y_pos];
    _g_cfg.light.position.z = iniCL[CONFIG][STR.z_pos];

    // rotation
    _g_cfg.light.rotation.x = iniCL[CONFIG][STR.x_rot];
    _g_cfg.light.rotation.y = iniCL[CONFIG][STR.y_rot];
    _g_cfg.light.rotation.z = iniCL[CONFIG][STR.z_rot];

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
