/*
    Custom Lantern -- Elden Ring 1.8.1
*/

#include "pch.h"
#include "ModUtils.h" // https://github.com/techiew/EldenRingMods/blob/master/ModUtils.h
#include "ini.h" // https://github.com/pulzed/mINI

using namespace ModUtils;
using namespace std;

const string INI_CONFIG_SECTION = "config";
const unsigned char SIGNATURE[16] = { 0x46, 0x58, 0x52, 0x00, 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x55, 0x9D, 0x04, 0x00 };

mINI::INIStructure _g_ini;
HANDLE _g_pHandle = 0;
__int64 _g_fxrBaseAddress = 0;
__int64 _g_pBaseAddress = 0;

void setParam(string param, int offset)
{
    string value = _g_ini.get(INI_CONFIG_SECTION).get(param);
    Log("%s value=%s offset=0x%X", param.c_str(), value.c_str(), offset);
    if (value != "")
    {
        float floatValue = std::stof(value);
        WriteProcessMemory(_g_pHandle, (LPVOID)(_g_fxrBaseAddress + offset), &floatValue, sizeof(floatValue), 0);
    }
}

__int64 getFXRBaseAddress() {
    __int64 address = _g_pBaseAddress + 0x04549108;
    __int64 result = 0;

    ReadProcessMemory(_g_pHandle, (LPCVOID)address, &result, sizeof(result), NULL);
    ReadProcessMemory(_g_pHandle, (LPCVOID)(result + 0x28), &result, sizeof(result), NULL);
    ReadProcessMemory(_g_pHandle, (LPCVOID)(result + 0x160), &result, sizeof(result), NULL);
    ReadProcessMemory(_g_pHandle, (LPCVOID)(result + 0x38), &result, sizeof(result), NULL);
    ReadProcessMemory(_g_pHandle, (LPCVOID)(result + 0xEA0), &result, sizeof(result), NULL);
    ReadProcessMemory(_g_pHandle, (LPCVOID)(result + 0x08), &result, sizeof(result), NULL);
    ReadProcessMemory(_g_pHandle, (LPCVOID)(result + 0x48), &result, sizeof(result), NULL);
    ReadProcessMemory(_g_pHandle, (LPCVOID)(result + 0xEC0), &result, sizeof(result), NULL);

    return result;
}

bool isSignatureValid()
{
    unsigned char buff[sizeof(SIGNATURE) + 1]{};
    ReadProcessMemory(_g_pHandle, (LPCVOID)(_g_fxrBaseAddress), &buff, sizeof(buff), NULL);

    for (size_t i = 0; i < sizeof(SIGNATURE); i++)
    {
        if (buff[i] != SIGNATURE[i])
        {
            return false;
        }
    }

    return true;
}

DWORD WINAPI Patch(LPVOID lpParam)
{
    Log("Activating Custom Lantern...");

    DWORD pid = GetCurrentProcessId();

    _g_pBaseAddress = GetProcessBaseAddress(pid);
    Log("Process name: %s", GetModuleName(false).c_str());
    Log("Process ID: %i", pid);
    Log("Process base address: 0x%I64X", _g_pBaseAddress);

    _g_pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    bool isSignValid = false;
    _g_fxrBaseAddress = 0;

    for (size_t i = 0; i < 30; i++)
    {
        _g_fxrBaseAddress = getFXRBaseAddress();
        if (isSignatureValid())
        {
            isSignValid = true;
            break;
        }

        Sleep(2500);
    }

    if (!isSignValid)
    {
        RaiseError("Unable to find the FXR base address. The signature in memory seems invalid. Try to relaunch the game.");

        return 1;
    }

    Log("FXR base address: 0x%I64X", _g_fxrBaseAddress);

    mINI::INIFile file(GetModuleFolderPath() + "\\custom-lantern.ini");
    file.read(_g_ini);

    /*
    * default values
    *
    struct color
    {
        float red = 1.0F;
        float green = 0.6313726F;
        float blue = 0.36862746F;
        float alpha = 1.0F;

        float sp_red = 1.0F;
        float sp_green = 0.6313726F;
        float sp_blue = 0.36862746F;
        float sp_alpha = 1.0F;

        float radius = 16.0F;
    } color;

    struct positionRotation
    {
        float x_pos = 0.0F;
        float y_pos = 0.0F;
        float z_pos = 0.0F;

        float x_rot = 0.0F;
        float y_rot = 0.0F;
        float z_rot = 0.0F;
    } positionRotation;

    float luminous_intensity = 1.25F;
    */

    Log("Write values in memory...", _g_fxrBaseAddress);

    // light color
    setParam("red", 0x1CDC);
    setParam("green", 0x1CE0);
    setParam("blue", 0x1CE4);
    setParam("alpha", 0x1CE8);

    // specular color
    setParam("sp_red", 0x1CEC);
    setParam("sp_green", 0x1CF0);
    setParam("sp_blue", 0x1CF4);
    setParam("sp_alpha", 0x1CF8);

    // light radius
    setParam("radius", 0x1CFC);

    // luminous intensity
    setParam("luminous_intensity", 0x1D0C);

    // position and rotation
    setParam("x_pos", 0x1C1C);
    setParam("y_pos", 0x1C20);
    setParam("z_pos", 0x1C24);
    setParam("x_rot", 0x1C28);
    setParam("y_rot", 0x1C2C);
    setParam("z_rot", 0x1C30);

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

