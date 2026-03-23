#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "dct_watermark.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* --- 内部静态查找表与常量 --- */
static double G_COS_TBL[8][8];
static double G_ALPHA[8];
static int G_TBL_INIT = 0;
static const int R1 = 3, C1 = 4, R2 = 4, C2 = 3;

/**
 * @brief 初始化 DCT 查找表
 * 此函数只在程序运行期间第一次被调用时执行。
 */
static void init_tables() {
    if (G_TBL_INIT) return;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) G_COS_TBL[i][j] = cos((2 * j + 1) * i * M_PI / 16.0);
        G_ALPHA[i] = (i == 0) ? 0.35355339 : 0.5;
    }
    G_TBL_INIT = 1;
}

/* --- DCT 变换核心 --- */
/**
 * @brief 8x8 块的快速正向 DCT 变换
 */
static void fdct_8x8(double in[8][8], double out[8][8]) {
    double tmp[8][8];
    for (int i = 0; i < 8; i++) {
        for (int u = 0; u < 8; u++) {
            double s = 0;
            for (int j = 0; j < 8; j++) s += in[i][j] * G_COS_TBL[u][j];
            tmp[i][u] = s * G_ALPHA[u];
        }
    }
    for (int j = 0; j < 8; j++) {
        for (int v = 0; v < 8; v++) {
            double s = 0;
            for (int i = 0; i < 8; i++) s += tmp[i][j] * G_COS_TBL[v][i];
            out[v][j] = s * G_ALPHA[v];
        }
    }
}

/**
 * @brief 8x8 块的快速逆向 DCT 变换
 */
static void idct_8x8(double in[8][8], double out[8][8]) {
    double tmp[8][8];
    for (int i = 0; i < 8; i++) {
        for (int x = 0; x < 8; x++) {
            double s = 0;
            for (int u = 0; u < 8; u++) s += G_ALPHA[u] * in[i][u] * G_COS_TBL[u][x];
            tmp[i][x] = s;
        }
    }
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            double s = 0;
            for (int i = 0; i < 8; i++) s += G_ALPHA[i] * tmp[i][x] * G_COS_TBL[i][y];
            out[y][x] = s;
        }
    }
}

/* --- 提取函数 --- */
/**
 * @brief 从图像的特定偏移处扫描水印
 * @param pix 指向像素数据的指针
 * @param w 图像宽度
 * @param h 图像高度
 * @param ch 图像通道数
 * @param ox x轴扫描偏移量
 * @param oy y轴扫描偏移量
 * @param marker 同步标记字节
 * @return 1 如果找到并成功提取了水印，否则返回 0
 */
static int scan_watermark(uint8_t* pix, int w, int h, int ch, int ox, int oy, uint8_t marker) {
    uint8_t shift = 0;
    int state = 0, bit_idx = 0, found_any = 0;
    int blocks_checked = 0;

    for (int i = oy; i <= h - 8; i += 8) {
        for (int j = ox; j <= w - 8; j += 8) {
            blocks_checked++;
            if (state == 0 && blocks_checked > 300) return 0;   // 早期退出：如果扫描了300个块仍未找到同步标记，就放弃当前偏移

            double b[8][8], d[8][8];
            for (int r = 0; r < 8; r++) {
                uint8_t* p = pix + ((i + r) * w + j) * ch;
                for (int c = 0; c < 8; c++) b[r][c] = (double)p[c * ch + 2]; // 始终处理 B 通道
            }

            fdct_8x8(b, d);
            shift = (shift << 1) | (d[R1][C1] > d[R2][C2]);

            if (state == 0) {
                if (shift == marker) { state = 1; bit_idx = 0; shift = 0; }
            }
            else {
                if (++bit_idx == 8) {
                    if (shift == 0) return 1;
                    putchar(shift);
                    bit_idx = 0; shift = 0;
                    found_any = 1;
                }
            }
        }
    }
    return found_any;
}

 /* --- 辅助函数：格式保存选择 --- */
 /**
  * @brief 根据输出文件的扩展名自动选择合适的格式进行保存
  * @param path 输出文件路径
  * @param w 图像宽度
  * @param h 图像高度
  * @param ch 图像通道数
  * @param pix 指向像素数据的指针
  */
 static void save_image_by_extension(const char* path, int w, int h, int ch, uint8_t* pix) {
    char ext[16] = { 0 };
    const char* dot = strrchr(path, '.');
    if (!dot) { stbi_write_png(path, w, h, ch, pix, w * ch); return; }  // 如果没有找到扩展名，默认保存为 PNG

    for (int i = 0; i < 15 && dot[i + 1]; i++) ext[i] = tolower((unsigned char)dot[i + 1]);

    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
        stbi_write_jpg(path, w, h, ch, pix, 100); // 质量设为100以减少DCT损失
    }
    else if (strcmp(ext, "bmp") == 0) {
        stbi_write_bmp(path, w, h, ch, pix);
    }
    else if (strcmp(ext, "tga") == 0) {
        stbi_write_tga(path, w, h, ch, pix);
    }
    else {
        stbi_write_png(path, w, h, ch, pix, w * ch); // 默认保存为PNG
    }
}

 /* --- API 入口 --- */
 /**
  * @brief 核心的 DCT 域水印处理函数
  * @param in_path 输入图像路径
  * @param out_path 输出图像路径 (嵌入模式时有效)
  * @param mode 1为嵌入，2为提取
  * @param text 要嵌入的文本 (嵌入模式时有效)
  * @param strength 嵌入强度
  * @param marker 同步标记字节
  * @return 操作状态
  */
 wm_status_t wm_process(const char* in_path, const char* out_path, int mode,
    const char* text, double strength, uint8_t marker) {
    init_tables();
    int w, h, ch;
    uint8_t* pix = stbi_load(in_path, &w, &h, &ch, 0);
    if (!pix) return WM_ERROR_FILE_IO;

    if (mode == 1) { // 嵌入模式
        if (!out_path || !text) { stbi_image_free(pix); return WM_ERROR_INVALID_FORMAT; }

        size_t p_len = strlen(text) + 2;
        uint8_t* buf = calloc(p_len, 1);
        buf[0] = marker; memcpy(buf + 1, text, strlen(text));
        int total_bits = (int)p_len * 8, b_idx = 0;

        for (int i = 0; i <= h - 8; i += 8) {
            for (int j = 0; j <= w - 8; j += 8, b_idx++) {
                double b[8][8], d[8][8];
                for (int r = 0; r < 8; r++) {
                    uint8_t* p = pix + ((i + r) * w + j) * ch;
                    for (int c = 0; c < 8; c++) b[r][c] = (double)p[c * ch + 2];
                }
                fdct_8x8(b, d);
                int bit = (buf[(b_idx % total_bits) / 8] >> (7 - (b_idx % total_bits) % 8)) & 1;
                if (bit) { if (d[R1][C1] <= d[R2][C2]) { d[R1][C1] = d[R2][C2] + strength; d[R2][C2] -= strength; } }
                else { if (d[R1][C1] >= d[R2][C2]) { d[R1][C1] = d[R2][C2] - strength; d[R2][C2] += strength; } }
                idct_8x8(d, b);
                for (int r = 0; r < 8; r++) {
                    uint8_t* p = pix + ((i + r) * w + j) * ch;
                    for (int c = 0; c < 8; c++) {
                        double v = b[r][c] + 0.5;
                        p[c * ch + 2] = (uint8_t)(v > 255 ? 255 : (v < 0 ? 0 : v));
                    }
                }
            }
        }
        save_image_by_extension(out_path, w, h, ch, pix);
        free(buf);
    }
    else { // 提取模式
        printf("Decoding %dx%d image...\n", w, h);
        int ok = 0;
        for (int oy = 0; oy < 8 && !ok; oy++) {
            for (int ox = 0; ox < 8 && !ok; ox++) {
                if (scan_watermark(pix, w, h, ch, ox, oy, marker)) {
                    printf("\n[Success at offset %d,%d]\n", ox, oy);
                    ok = 1;
                }
            }
        }
        if (!ok) printf("No watermark detected.\n");
    }

    stbi_image_free(pix);
    return WM_SUCCESS;
}


 // 仅当定义了 DEBUG_MODE 宏时，才编译以下 main 函数，使其成为一个命令行工具
#ifdef DEBUG_MODE
#include <ctype.h>

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 2) {
        goto print_usage;
    }

    const char* command = argv[1];
    uint8_t sync_marker = 0xAA;

    if (strcmp(command, "encode") == 0) {
        if (argc < 5) goto print_usage;

        const char* in_file = argv[2];
        const char* out_file = argv[3];
        const char* text_to_embed = argv[4];
        double strength = (argc >= 6) ? atof(argv[5]) : 20.0;
        if (strength <= 0) strength = 20.0;

        printf("\n--- DCT Watermark Encoder ---\n");
        printf("Mode:          ENCODE\n");
        printf("Input:         %s\n", in_file);
        printf("Output:        %s\n", out_file);
        printf("Strength:      %.2f\n", strength);

        wm_status_t status = wm_process(in_file, out_file, 1, text_to_embed, strength, sync_marker);
        if (status == WM_SUCCESS) printf("\n>>> [SUCCESS] Watermark embedded.\n");
        else fprintf(stderr, "\n>>> [ERROR] Code: %d\n", status);

    }
    else if (strcmp(command, "decode") == 0) {
        if (argc < 3) goto print_usage;

        const char* in_file = argv[2];
        printf("\n--- DCT Watermark Decoder ---\n");
        printf("Mode:          DECODE\n");
        printf("Input:         %s\n", in_file);
        printf("Result:        ");

        wm_status_t status = wm_process(in_file, NULL, 2, NULL, 0.0, sync_marker);
        if (status != WM_SUCCESS) fprintf(stderr, "\n>>> [ERROR] Code: %d\n", status);

    }
    else {
        goto print_usage;
    }

    return 0;

print_usage:
    fprintf(stderr, "\nDCT Watermark Tool v1.0\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s encode <in.bmp> <out.bmp> \"<text>\" [strength]\n", argv[0]);
    fprintf(stderr, "  %s decode <in.bmp>\n", argv[0]);
    return 1;
}
#endif