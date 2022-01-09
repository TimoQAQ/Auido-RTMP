

int ShowMenu()
{
    system("cls");
    cout << "******录音管理系统******" << endl;
    cout << " 1. 开始录音" << endl;
    cout << " 2. 停止录音" << endl;
    cout << " 3. 保存录音" << endl;
    cout << " 0. 退出" << endl;
    cout << "************************" << endl;
    int iChoose = 0;
    while (1)
    {
        cin.clear();
        cin.sync(); // fflush(stdin)

        cout << "请选择: ";
        cin >> iChoose;
        if (!cin.good())
            continue;
        if (iChoose < 0 || iChoose > 3)
            continue;
        break;
    }
    return iChoose;
}

void SaveRecord()
{
    if (dwRecord == true)
    {
        StopRecord();
    }

    DWORD NumToWrite = 0;
    DWORD dwNumber = 0;
    CHAR FilePathName[MAX_PATH] = {NULL};
    strcpy_s(FilePathName, "a.wav");
    HANDLE FileHandle =
        CreateFile((LPCWSTR)FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // memset(m_pWaveHdr.lpData, 0, datasize);

    // dwNumber = FCC("RIFF");
    WriteFile(FileHandle, "RIFF", 4, &NumToWrite, NULL);
    dwNumber = dwDataLength + 18 + 20;
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    // dwNumber = FCC("WAVE");

    WriteFile(FileHandle, "WAVE", 4, &NumToWrite, NULL);
    // dwNumber = FCC("fmt ");
    WriteFile(FileHandle, "fmt ", 4, &NumToWrite, NULL);
    dwNumber = 16;

    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);

    WriteFile(FileHandle, &waveformat.wFormatTag, sizeof(waveformat.wFormatTag), &NumToWrite, NULL);
    WriteFile(FileHandle, &waveformat.nChannels, sizeof(waveformat.nChannels), &NumToWrite, NULL);
    WriteFile(FileHandle, &waveformat.nSamplesPerSec, sizeof(waveformat.nSamplesPerSec), &NumToWrite, NULL);
    WriteFile(FileHandle, &waveformat.nAvgBytesPerSec, sizeof(waveformat.nAvgBytesPerSec), &NumToWrite, NULL);
    WriteFile(FileHandle, &waveformat.nBlockAlign, sizeof(waveformat.nBlockAlign), &NumToWrite, NULL);
    WriteFile(FileHandle, &waveformat.wBitsPerSample, sizeof(waveformat.wBitsPerSample), &NumToWrite, NULL);
    // dwNumber = FCC("data");
    WriteFile(FileHandle, "data", 4, &NumToWrite, NULL);
    dwNumber = dwDataLength;
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    WriteFile(FileHandle, pSaveBuffer, dwDataLength, &NumToWrite, NULL);
    SetEndOfFile(FileHandle);
    CloseHandle(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE; // 收尾关闭句柄

    // waveInReset(hwi); //清空内存块
    // waveInClose(hwi); //关闭录音设备
}
