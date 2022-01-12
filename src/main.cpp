#include <stdio.h>
#include <Windows.h>
#include <mmsystem.h> //导入声音头文件
#include "aacenc.h"
#include "srs_librtmp.h"
#include "pcm2wave.h"

using namespace std;

// #define SAVE2WAV
#define MAX_INQUEU 2
#define BUFSIZE 1024 * 2
#define RTMP_URL "rtmp://47.241.234.247:1935/live/aac"
// #define RTMP_URL "rtmp://127.0.0.1:1935/live/aac"

static HWAVEIN hwi;
static WAVEFORMATEX waveformat;
static WAVEHDR *pwhi, whis[MAX_INQUEU];
static char waveBufferRecord[MAX_INQUEU][BUFSIZE];

static srs_rtmp_t rtmp;
static u_int32_t timestamp = 0;
static bool isrecord = 0;
static int wave_size = 0;
static BYTE *wave_buff = NULL;

int init_rtmp(void)
{
    printf("Recording audio and push to sever use rtmp\n");
    printf("Use fdk-aac encoding library.\n");
    printf("SRS(ossrs) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());
    printf("You can use VLC to play the rtmp audio url is:\n\t%s\n", RTMP_URL);

    rtmp = srs_rtmp_create(RTMP_URL);

    if (srs_rtmp_handshake(rtmp) != 0)
    {
        srs_human_trace("simple handshake failed.");
        return -1;
    }
    srs_human_trace("simple handshake success");

    if (srs_rtmp_connect_app(rtmp) != 0)
    {
        srs_human_trace("connect vhost/app failed.");
        return -1;
    }
    srs_human_trace("connect vhost/app success");

    if (srs_rtmp_publish_stream(rtmp) != 0)
    {
        srs_human_trace("publish stream failed.");
        return -1;
    }
    srs_human_trace("publish stream success");

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
    if (uMsg == MM_WIM_DATA && isrecord)
    {
        BYTE aac_data[BUFSIZE];
        int output_buf_len = aacenc_pcm2aac((uint8_t *)pwh->lpData, aac_data, pwh->dwBytesRecorded);

#ifdef SAVE2WAV
        wave_size += output_buf_len;
        wave_buff = (BYTE *)realloc(wave_buff, wave_size * sizeof(BYTE));

        if (wave_buff)
        {
            memcpy(wave_buff + wave_size - output_buf_len, aac_data, output_buf_len);
        }
        else
        {
            printf("realloc error...\n");
        }
#endif

        // 调用推流函数
        char sound_format = 10;
        char sound_rate = 3;
        char sound_size = 1;
        char sound_type = 0;
        timestamp += (int)(pwh->dwBytesRecorded * 1000 / (waveformat.nSamplesPerSec * waveformat.nBlockAlign));
        // timestamp += 23;
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
            // srs_rtmp_destroy(rtmp);
        }

        srs_human_trace("sent packet: type=%s, time=%d, raw_size=%d, aac_size=%d, codec=%d, rate=%d, sample=%d, channel=%d",
                        srs_human_flv_tag_type2string(SRS_RTMP_TYPE_AUDIO),
                        timestamp,
                        pwh->dwBytesRecorded,
                        output_buf_len,
                        sound_format,
                        sound_rate,
                        sound_size,
                        sound_type);

        // waveInPrepareHeader(hwi, pwh, sizeof(WAVEHDR));
        waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));
    }
}

int start_record(void)
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

    printf("10s to record and push stream");
    for (int i = 0; i < 10; i++)
    {
        Sleep(1000);
        printf(".");
    }
    printf("\n");

    if (waveInStart(hwi) != MMSYSERR_NOERROR)
    {
        printf("waveInStart error...\n");
        return -1;
    }
    isrecord = true;

    return 0;
}

void stop_record(void)
{
    // waveInReset(hwi);
    isrecord = false;
    waveInStop(hwi);

#ifdef SAVE2WAV
    pcm2wave((char *)wave_buff, wave_size, "record.wav");
#endif
}

int main(int argc, char *argv[])
{
    printf("start...\n");
    wave_buff = (BYTE *)malloc(1);

    if (init_rtmp() < 0)
        goto rtmp_destroy;

    if (aacenc_init() < 0)
        goto aacenc_destroy;

    if (start_record() < 0)
    {
        goto record_destroy;
    }

    // 阻塞函数
    getchar();

record_destroy:
    stop_record();
aacenc_destroy:
    aacenc_close();
rtmp_destroy:
    srs_rtmp_destroy(rtmp);
    free(wave_buff);
    printf("exit...\n");

    return 0;
}
