#ifndef _AACENC_H_
#include <stdint.h>

int accenc_init(void);
int accenc_pcm2acc(uint8_t *input_buf, uint8_t *output_buf, int input_size);

#define _AACENC_H_
#endif
