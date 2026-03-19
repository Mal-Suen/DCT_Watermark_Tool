#ifndef DCT_WATERMARK_H
#define DCT_WATERMARK_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1, reserved2;
    uint32_t offset;
} BMPHeader;

typedef struct {
    uint32_t size;
    int32_t width, height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t isize;
    int32_t x_res, y_res;
    uint32_t colors, important_colors;
} BMPInfo;
#pragma pack(pop)

// mode: 1-嵌入, 2-提取
void process_text_watermark(const char* in_file, const char* out_file, int mode, const char* text);

#endif