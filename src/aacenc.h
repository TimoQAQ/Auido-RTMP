#ifndef _AACENC_H_
#include <stdint.h>

int aacenc_init(void);
int aacenc_close(void);
int aacenc_pcm2aac(uint8_t *input_buf, uint8_t *output_buf, int input_size);

#define _AACENC_H_
#endif
