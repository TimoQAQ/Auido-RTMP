//  WTFPL
//
//  Defines the entry point for the console application.
//  WTFPL

// targetver.h
//#include "stdafx.h"

#include <windows.h>
#include "CommDlg.h"
//  #include "PrSht.h"
#include "MMSystem.h"
#include <fstream>
#include <cstdlib>
#include <iostream>
//  #include "resource.h"

#define NSEC 15;
using namespace std;

HWAVEIN hWaveIn;
WAVEHDR WaveInHdr;
MMRESULT result;
WAVEFORMATEX pFormat;

void CheckMMIOError(DWORD code);
void SaveWaveFile();

// int _tmain(int argc, _TCHAR* argv[]) // main()
int main()
{

    // Declare local varibles
    int samplesperchan;
    int sampleRate;
    int *waveIn;

    cout << "*********************************************\n";
    cout << "Configuring the Sound Hardware:\n";
    cout << "*********************************************\n";
    cout << "Enter 65536 in the 1st blank and 44100 in the second if you have no idea what to input.\n";
    cout << "Enter the number of Samples/Channel:\n";
    cin >> samplesperchan;
    cout << "Enter the Sampling Rate:\n";
    cin >> sampleRate;

    pFormat.wFormatTag = WAVE_FORMAT_PCM; // simple, uncompressed format
    pFormat.nChannels = 1;                // 1=mono, 2=stereo
    pFormat.nSamplesPerSec = sampleRate;  // 44100
    pFormat.wBitsPerSample = 16;          // 16 for high quality, 8 for telephone-grade
    pFormat.nBlockAlign = pFormat.nChannels * pFormat.wBitsPerSample / 8;
    pFormat.nAvgBytesPerSec = pFormat.nChannels * pFormat.wBitsPerSample / 8;
    pFormat.cbSize = 0;
    result = waveInOpen(&hWaveIn, WAVE_MAPPER, &pFormat, 0L, 0L, WAVE_FORMAT_DIRECT);
             waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD_PTR)wait, 0L, CALLBACK_EVENT);


    if (result)
    {
        char fault[256];
        waveInGetErrorText(result, fault, 256);
        MessageBoxW(NULL, L"Failed to open waveform input device.", NULL, NULL);
        return 14444;
    }

    int nSec = NSEC;
    waveIn = new int[sampleRate * nSec];
    WaveInHdr.lpData = (LPSTR)waveIn;
    WaveInHdr.dwBufferLength = sampleRate * nSec * pFormat.nBlockAlign;
    WaveInHdr.dwBytesRecorded = 0;
    WaveInHdr.dwUser = 0L;
    WaveInHdr.dwFlags = 0L;
    WaveInHdr.dwLoops = 0L;
    waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

    result = waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
    if (result)
    {
        MessageBoxW(NULL, L"Failed to read block from device", NULL, NULL);
        return 10001;
    }

    result = waveInStart(hWaveIn);
    if (result)
    {
        MessageBoxW(NULL, L"Failed to start recording", NULL, NULL);
        return 10002;
    }

    cout << "Start Recording...........\n";
    cout << "Note: in 15 seconds the application will end by itself.\n";

    do
    {
    }

    while (waveInUnprepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

    SaveWaveFile();
    waveInStop(hWaveIn);
    cout << "Stop Recording.\n";
    waveInClose(hWaveIn);

    if (!waveIn)
        delete[] waveIn;

    return 0;
}

void SaveWaveFile()
{
    MMCKINFO ChunkInfo;
    MMCKINFO FormatChunkInfo;
    MMCKINFO DataChunkInfo;
    HMMIO handle = mmioOpen("test.wav", 0, MMIO_CREATE | MMIO_WRITE);

    if (!handle)
    {
        MessageBox(0, "Error creating file.", "Error Message", 0);
        return;
    }

    memset(&ChunkInfo, 0, sizeof(MMCKINFO));
    ChunkInfo.fccType = mmioStringToFOURCC("WAVE", 0);
    DWORD Res = mmioCreateChunk(handle, &ChunkInfo, MMIO_CREATERIFF);
    CheckMMIOError(Res);

    FormatChunkInfo.ckid = mmioStringToFOURCC("fmt ", 0);
    FormatChunkInfo.cksize = sizeof(WAVEFORMATEX);
    Res = mmioCreateChunk(handle, &FormatChunkInfo, 0);
    CheckMMIOError(Res);

    // Write the wave format data.
    mmioWrite(handle, (char *)&pFormat, sizeof(pFormat));
    Res = mmioAscend(handle, &FormatChunkInfo, 0);
    CheckMMIOError(Res);
    DataChunkInfo.ckid = mmioStringToFOURCC("data", 0);
    DWORD DataSize = WaveInHdr.dwBytesRecorded;
    DataChunkInfo.cksize = DataSize;
    Res = mmioCreateChunk(handle, &DataChunkInfo, 0);
    CheckMMIOError(Res);
    mmioWrite(handle, (char *)WaveInHdr.lpData, DataSize);

    // Ascend out of the data chunk.
    mmioAscend(handle, &DataChunkInfo, 0);

    // Ascend out of the RIFF chunk (the main chunk). Failure to do
    // this will result in a file that is unreadable by Windows95
    // Sound Recorder.
    mmioAscend(handle, &ChunkInfo, 0);
    mmioClose(handle, 0);
}

void CheckMMIOError(DWORD code)
{
    if (code == 0)
        return;
    char buff[256];
    wsprintf(buff,
             "MMIO Error. Error Code: %d", code);
    MessageBox(NULL, buff, "MMIO Error", 0);
}

// THE END.  YOU ARE WELCOME.