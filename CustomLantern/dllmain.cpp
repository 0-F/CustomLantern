/*
    Custom Lantern -- Elden Ring 1.8.1
*/

#include "pch.h"
#include "ModUtils.h" // https://github.com/techiew/EldenRingMods/blob/master/ModUtils.h
#include "ini.h" // https://github.com/pulzed/mINI

const std::string INI_CONFIG_SECTION = "config";
const unsigned char SIGNATURE[16] = { 0x46, 0x58, 0x52, 0x00, 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x55, 0x9D, 0x04, 0x00 };

mINI::INIStructure ini;
HANDLE pHandle = 0;
__int64 baseAddress = 0;

void setParam(std::string param, int offset) {
    std::string value = ini.get(INI_CONFIG_SECTION).get(param);
    ModUtils::Log("Set param \"%s\" value=\"%s\" offset=0x%X\n", param.c_str(), value.c_str(), offset);
    if (value != "")
    {
        float floatValue = std::stof(value);
        WriteProcessMemory(pHandle, (LPVOID)(baseAddress + offset), &floatValue, sizeof(floatValue), 0);
    }
}

DWORD WINAPI Patch(LPVOID lpParam)
{
    ModUtils::Log("Activating Custom Lantern...");

    DWORD pid = GetCurrentProcessId();

    __int64 processBaseAddress = ModUtils::GetProcessBaseAddress(pid);
    ModUtils::Log("Process name: %s", ModUtils::GetModuleName(false).c_str());
    ModUtils::Log("Process ID: %i", pid);
    ModUtils::Log("Process base address: 0x%I64X", processBaseAddress);

    pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    __int64 result = 0;
    __int64 address = processBaseAddress + 0x04549108;

    // get the base address of the FXR file copied in memory
    ReadProcessMemory(pHandle, (LPCVOID)address, &result, sizeof(result), NULL);
    ReadProcessMemory(pHandle, (LPCVOID)(result + 0x28), &result, sizeof(result), NULL);
    ReadProcessMemory(pHandle, (LPCVOID)(result + 0x160), &result, sizeof(result), NULL);
    ReadProcessMemory(pHandle, (LPCVOID)(result + 0x38), &result, sizeof(result), NULL);
    ReadProcessMemory(pHandle, (LPCVOID)(result + 0xEA0), &result, sizeof(result), NULL);
    ReadProcessMemory(pHandle, (LPCVOID)(result + 0x08), &result, sizeof(result), NULL);
    ReadProcessMemory(pHandle, (LPCVOID)(result + 0x48), &result, sizeof(result), NULL);
    ReadProcessMemory(pHandle, (LPCVOID)(result + 0xEC0), &result, sizeof(result), NULL);

    baseAddress = result;

    unsigned char buff[sizeof(SIGNATURE) + 1]{};
    ReadProcessMemory(pHandle, (LPCVOID)(baseAddress), &buff, sizeof(buff), NULL);

    for (size_t i = 0; i < sizeof(SIGNATURE); i++)
    {
        if (buff[i] != SIGNATURE[i])
        {
            ModUtils::Log("ERROR: signature in memory seems invalid.");

            exit(1);
        }
    }

    ModUtils::Log("FXR base address: 0x%I64X", result);

    mINI::INIFile file(ModUtils::GetModuleFolderPath() + "\\custom-lantern.ini");

    file.read(ini);

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

    ModUtils::CloseLog();

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
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

