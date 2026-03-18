#ifndef DCT_WATERMARK_H
#define DCT_WATERMARK_H

#include <stdint.h>

// 导出函数声明
void fdct_8x8(double input[8][8], double output[8][8]);
void idct_8x8(double input[8][8], double output[8][8]);
void embed_bit(double dct_block[8][8], int bit, double alpha);

#endif