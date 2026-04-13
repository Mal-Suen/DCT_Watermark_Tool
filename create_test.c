#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdlib.h>
#include <stdio.h>

int main() {
    int width = 512, height = 512, channels = 3;
    size_t pixel_size = (size_t)width * height * channels;
    unsigned char* pixels = (unsigned char*)malloc(pixel_size);
    
    if (!pixels) {
        fprintf(stderr, "错误: 内存分配失败\n");
        return 1;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * channels;
            pixels[idx + 0] = (unsigned char)((x * 255 / width) % 256);
            pixels[idx + 1] = (unsigned char)((y * 255 / height) % 256);
            pixels[idx + 2] = (unsigned char)(((x + y) * 128 / width) % 256);
        }
    }

    int success = stbi_write_bmp("test_image.bmp", width, height, channels, pixels);
    free(pixels);
    
    if (!success) {
        fprintf(stderr, "错误: 写入文件失败\n");
        return 1;
    }
    
    return 0;
}
