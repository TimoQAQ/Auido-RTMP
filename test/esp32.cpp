#include <unistd.h>
// #include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include "srs_librtmp.h"
#include "AACEncoderFDK.h"
#include "I2SMEMSSampler.h"

#define SSID "<<YOUR_WIFI_SSID>>"
#define PASSWORD "<<YOUR_WIFI_PASSWORD>>"

/**
 * @brief
 * 1. lwip移植到srs-librtmp
 * 2. idf+i2s+aac库
 * 方案一：UDP+PCM
 * 方案二：RTMP+AAC
 *
 */

using namespace aac_fdk;

void dataCallback(uint8_t *aac_data, size_t len);

// i2s config for reading from left channel of I2S
i2s_config_t i2sMemsConfigLeftChannel = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// i2s pins
i2s_pin_config_t i2sPins = {
    .bck_io_num = GPIO_NUM_32,
    .ws_io_num = GPIO_NUM_25,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = GPIO_NUM_33};

// how many samples to read at once
const int SAMPLE_SIZE = 16384;

AudioInfo info;
srs_rtmp_t rtmp;
AACEncoderFDK aac(dataCallback);
I2SSampler *i2sSampler = NULL;

// Task to write samples to our server
void i2sMemsWriterTask(void *param)
{
    I2SSampler *sampler = (I2SSampler *)param;
    int16_t *samples = (int16_t *)malloc(sizeof(uint16_t) * SAMPLE_SIZE);
    if (!samples)
    {
        Serial.println("Failed to allocate memory for samples");
        return;
    }
    while (true)
    {
        int samples_read = sampler->read(samples, SAMPLE_SIZE);
        // encode to aac
        aac.write((uint8_t *)samples, SAMPLE_SIZE);
    }
}

void dataCallback(uint8_t *aac_data, size_t len)
{
    // push to server use rtmp
    // 0 = Linear PCM, platform endian
    // 1 = ADPCM
    // 2 = MP3
    // 7 = G.711 A-law logarithmic PCM
    // 8 = G.711 mu-law logarithmic PCM
    // 10 = AAC
    // 11 = Speex
    char sound_format = 10;
    // 2 = 22 kHz
    char sound_rate = 2;
    // 1 = 16-bit samples
    char sound_size = 1;
    // 1 = Stereo sound
    char sound_type = 1;

    u_int32_t timestamp = 0;

    int ret = 0;
    if ((ret = srs_audio_write_raw_frame(rtmp,
                                         sound_format, sound_rate, sound_size, sound_type,
                                         (char *)aac_data, len, timestamp)) != 0)
    {
        srs_human_trace("send audio raw data failed. ret=%d", ret);
        srs_rtmp_destroy(rtmp);
    }

    srs_human_trace("sent packet: type=%s, time=%d, size=%d, codec=%d, rate=%d, sample=%d, channel=%d",
                    srs_human_flv_tag_type2string(SRS_RTMP_TYPE_AUDIO), timestamp, len, sound_format, sound_rate,
                    sound_size,
                    sound_type);

    // @remark, when use encode device, it not need to sleep.
    // usleep(1000 * time_delta);
}

void setup()
{
    Serial.begin(115200);

    Serial.printf("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("");
    Serial.println("WiFi Connected");
    Serial.println("Started up");

    printf("Example for srs-librtmp\n");
    printf("SRS(ossrs) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());

    rtmp = srs_rtmp_create("rtmp://ossrs.net/live/livestream");

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

    info.channels = 1;
    info.sample_rate = 44100;
    aac.begin(info);

    i2sSampler = new I2SMEMSSampler(I2S_NUM_0, i2sPins, i2sMemsConfigLeftChannel, false);
    i2sSampler->start();

    Serial.println("init over..");

rtmp_destroy:
    srs_rtmp_destroy(rtmp);
}

void loop()
{
    i2sMemsWriterTask(i2sSampler);
}
