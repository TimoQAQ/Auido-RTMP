#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "aacenc_lib.h"

int channels = 1;
HANDLE_AACENCODER handle;
AACENC_InfoStruct info = {0};

int accenc_init(void)
{

    int aot = 2;
    int vbr = 0;
    int eld_sbr = 0;
    int afterburner = 1;

    CHANNEL_MODE mode = MODE_1;
    int sample_rate = 44100;
    int bits_per_sample = 16;
    int bitrate = 128000;

    if (aacEncOpen(&handle, 0, channels) != AACENC_OK)
    {
        fprintf(stderr, "Unable to open encoder\n");
        return 1;
    }
    if (aacEncoder_SetParam(handle, AACENC_AOT, aot) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the AOT\n");
        return 1;
    }
    if (aot == 39 && eld_sbr)
    {
        if (aacEncoder_SetParam(handle, AACENC_SBR_MODE, 1) != AACENC_OK)
        {
            fprintf(stderr, "Unable to set SBR mode for ELD\n");
            return 1;
        }
    }
    if (aacEncoder_SetParam(handle, AACENC_SAMPLERATE, sample_rate) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the sample rate\n");
        return 1;
    }
    if (aacEncoder_SetParam(handle, AACENC_CHANNELMODE, mode) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the channel mode\n");
        return 1;
    }
    if (aacEncoder_SetParam(handle, AACENC_CHANNELORDER, 1) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the wav channel order\n");
        return 1;
    }
    if (vbr)
    {
        if (aacEncoder_SetParam(handle, AACENC_BITRATEMODE, vbr) != AACENC_OK)
        {
            fprintf(stderr, "Unable to set the VBR bitrate mode\n");
            return 1;
        }
    }
    else
    {
        if (aacEncoder_SetParam(handle, AACENC_BITRATE, bitrate) != AACENC_OK)
        {
            fprintf(stderr, "Unable to set the bitrate\n");
            return 1;
        }
    }
    if (aacEncoder_SetParam(handle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the ADTS transmux\n");
        return 1;
    }
    if (aacEncoder_SetParam(handle, AACENC_AFTERBURNER, afterburner) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the afterburner mode\n");
        return 1;
    }
    if (aacEncEncode(handle, NULL, NULL, NULL, NULL) != AACENC_OK)
    {
        fprintf(stderr, "Unable to initialize the encoder\n");
        return 1;
    }
    if (aacEncInfo(handle, &info) != AACENC_OK)
    {
        fprintf(stderr, "Unable to get the encoder info\n");
        return 1;
    }

    return 0;
}

int accenc_pcm2acc(uint8_t *input_buf, uint8_t *output_buf, int input_size)
{
    AACENC_ERROR err;
    AACENC_BufDesc in_buf = {0}, out_buf = {0};
    AACENC_InArgs in_args = {0};
    AACENC_OutArgs out_args = {0};
    int in_identifier = IN_AUDIO_DATA;
    int out_identifier = OUT_BITSTREAM_DATA;
    int in_size, in_elem_size;
    int out_size, out_elem_size;
    void *in_ptr, *out_ptr;
    uint8_t outbuf[20480];

    in_ptr = input_buf;
    in_size = input_size;
    in_elem_size = 2;
    in_args.numInSamples = input_size <= 0 ? -1 : input_size / 2;
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;

    out_ptr = outbuf;
    out_size = sizeof(outbuf);
    out_elem_size = 1;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_size;
    out_buf.bufElSizes = &out_elem_size;

    if ((err = aacEncEncode(handle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK)
    {
        if (err == AACENC_ENCODE_EOF)
        {
            fprintf(stderr, "end of buffer: 0x%x\n", err);
            return 0;
        }

        fprintf(stderr, "Encoding failed: 0x%x\n", err);
        return -1;
    }

    memcpy(output_buf, outbuf, out_args.numOutBytes);
    return out_args.numOutBytes;
}