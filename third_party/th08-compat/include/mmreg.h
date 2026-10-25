#pragma once
#include <windows.h>
#pragma pack(push, 1)
struct WAVEFORMATEX {
    WORD wFormatTag, nChannels;
    DWORD nSamplesPerSec, nAvgBytesPerSec;
    WORD nBlockAlign, wBitsPerSample, cbSize;
};
#pragma pack(pop)
using LPWAVEFORMATEX = WAVEFORMATEX *;
