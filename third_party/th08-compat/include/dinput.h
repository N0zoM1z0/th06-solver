#pragma once
#include <windows.h>
struct IDirectInput8;
struct IDirectInputDevice8;
using LPDIRECTINPUT8 = IDirectInput8 *;
using LPDIRECTINPUTDEVICE8 = IDirectInputDevice8 *;
struct DIDEVCAPS {
    DWORD dwSize, dwFlags, dwDevType, dwAxes, dwButtons, dwPOVs;
    DWORD dwFFSamplePeriod, dwFFMinTimeResolution, dwFirmwareRevision;
    DWORD dwHardwareRevision, dwFFDriverVersion;
};
