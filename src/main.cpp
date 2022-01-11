#include <stdio.h>
#include <Windows.h>
#include <mmsystem.h> //导入声音头文件
#include "aacenc.h"
#include "srs_librtmp.h"
#include "pcm2wave.h"

using namespace std;

#define MAX_INQUEU 2
#define BUFSIZE 2048

static srs_rtmp_t rtmp;
HWAVEIN hWaveIn;    //输入设备
static HWAVEIN hwi; // handle指向音频输入
static WAVEFORMATEX waveformat;
static WAVEHDR *pwhi, whis[MAX_INQUEU];
static char waveBufferRecord[MAX_INQUEU][BUFSIZE];
static int bufflag = 0; //标记读取哪个缓冲区
static u_int32_t timestamp = 0;
static BYTE *wave_buff = NULL;
static int wave_size = 0;

int init_rtmp(void)
{
    printf("Example for srs-librtmp\n");
    printf("SRS(ossrs) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());

    rtmp = srs_rtmp_create("rtmp://47.241.234.247:1935/live/test");
    // rtmp = srs_rtmp_create("rtmp://127.0.0.1:1935/live/test");

    if (srs_rtmp_handshake(rtmp) != 0)
    {
        srs_human_trace("simple handshake failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("simple handshake success");

    if (srs_rtmp_connect_app(rtmp) != 0)
    {
        srs_human_trace("connect vhost/app failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("connect vhost/app success");

    if (srs_rtmp_publish_stream(rtmp) != 0)
    {
        srs_human_trace("publish stream failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("publish stream success");

    return 0;

rtmp_destroy:
    srs_rtmp_destroy(rtmp);

    return 0;
}

void CALLBACK waveInProc(
    HWAVEIN hwi,
    UINT uMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2)
{
    LPWAVEHDR pwh = (LPWAVEHDR)dwParam1;
    if (uMsg == MM_WIM_DATA)
    {
        // waveInUnprepareHeader(hwi, pwh, sizeof(WAVEHDR));

        // static unsigned char prevBuf[BUFSIZE];
        // memcpy(prevBuf, pwh->lpData, pwh->dwBytesRecorded);
        // bufflag = (bufflag + 1) % MAX_INQUEU;


        if (1)
        {
            // 调用pcm2aac 返回数据
            // 数组大小设置，动态或者静态
            BYTE aac_data[20480];
            int output_buf_len = accenc_pcm2acc((uint8_t *)pwh->lpData, aac_data, BUFSIZE);

            wave_size += output_buf_len;
            wave_buff = (BYTE *)realloc(wave_buff, wave_size * sizeof(BYTE));

            if (wave_buff)
            {
                memcpy(wave_buff + wave_size - output_buf_len, aac_data, output_buf_len);
            }
            else
            {
                printf("error\n");
            }


            // 调用推流函数
            char sound_format = 10;
            char sound_rate = 3;
            char sound_size = 1;
            char sound_type = 0;
            // 时间戳如何获取
            // u_int32_t timestamp += (u_int32_t)clock();
            timestamp += 23;

            int ret = 0;
            if ((ret = srs_audio_write_raw_frame(rtmp,
                                                 sound_format,
                                                 sound_rate,
                                                 sound_size,
                                                 sound_type,
                                                 (char *)aac_data,
                                                 output_buf_len,
                                                 timestamp)) != 0)
            {
                srs_human_trace("send audio raw data failed. ret=%d", ret);
                srs_rtmp_destroy(rtmp);
            }

            srs_human_trace("sent packet: type=%s, time=%d, size=%d, codec=%d, rate=%d, sample=%d, channel=%d",
                            srs_human_flv_tag_type2string(SRS_RTMP_TYPE_AUDIO),
                            timestamp,
                            output_buf_len,
                            sound_format,
                            sound_rate,
                            sound_size,
                            sound_type);
        }

        // waveInPrepareHeader(hwi, pwh, sizeof(WAVEHDR));
        waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));
    }
}

void StartRecord(void)
{
    memset(&waveformat, 0, sizeof(WAVEFORMATEX));
    waveformat.wFormatTag = WAVE_FORMAT_PCM;
    waveformat.nChannels = 1;
    waveformat.wBitsPerSample = 16;
    waveformat.nSamplesPerSec = 44100L;
    waveformat.nBlockAlign = 2;
    waveformat.nAvgBytesPerSec = 88200L;
    waveformat.cbSize = 0;
    waveInOpen(&hwi, WAVE_MAPPER, &waveformat, (DWORD_PTR)waveInProc, (DWORD_PTR)NULL, CALLBACK_FUNCTION);
    for (int k = 0; k < MAX_INQUEU; k++)
    {
        pwhi = &whis[k];
        pwhi->dwFlags = 0;
        pwhi->dwUser = 0;
        pwhi->dwLoops = 1;
        pwhi->dwBytesRecorded = 0;
        pwhi->dwBufferLength = BUFSIZE;
        pwhi->lpData = waveBufferRecord[k];
        waveInPrepareHeader(hwi, pwhi, sizeof(WAVEHDR));
        waveInAddBuffer(hwi, pwhi, sizeof(WAVEHDR));
    }
    if (waveInStart(hwi) != MMSYSERR_NOERROR)
    {
        printf("waveInStart error");
    }
}

void StopRecord(void)
{
    waveInStop(hwi);
    pcm2wave((char *)wave_buff, wave_size, "record.wav");
}

int main(int argc, char *argv[])
{
    printf("start...\n");

    wave_buff = (BYTE *)malloc(1);

    init_rtmp();
    accenc_init();
    StartRecord();

    while (1)
    {
        char c = getchar();
        if (c == 0x61)
            break;
        else
            Sleep(1000);
    }

    StopRecord();

    return 0;
}
