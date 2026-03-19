#define _CRT_SECURE_NO_WARNINGS
#include "dct_watermark.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * 宏定义与常量预设
 * M_PI: 圆周率，用于三角函数计算
 * SYNC_MARKER: 同步特征码，用于检测水印信号的存在性及起始位置
 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SYNC_MARKER 0xAA 

 /* 全局静态查找表：存储预计算的余弦系数，消除重复的三角函数运算开销 */
static double COS_TABLE[8][8];
static int table_initialized = 0;

/**
 * init_tables: 预计算 DCT 系数表
 * 依据 DCT-II 标准公式，提前计算 cos((2j+1)iπ / 16) 并缓存
 */
void init_tables() {
    int i, j;
    if (table_initialized) return;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            COS_TABLE[i][j] = cos((2 * j + 1) * i * M_PI / 16.0);
        }
    }
    table_initialized = 1;
}

/**
 * fdct_1d: 一维正向离散余弦变换 (Forward DCT)
 * 将空间域信号转换为频率域能量分布
 */
void fdct_1d(double in[8], double out[8]) {
    int i, j;
    for (i = 0; i < 8; i++) {
        double sum = 0;
        for (j = 0; j < 8; j++) {
            sum += in[j] * COS_TABLE[i][j];
        }
        // 应用正交化系数 Alpha
        double alpha = (i == 0) ? 0.35355339 : 0.5; // sqrt(1/8) 与 sqrt(2/8)
        out[i] = alpha * sum;
    }
}

/**
 * idct_1d: 一维逆向离散余弦变换 (Inverse DCT)
 * 将频率域系数还原回空间域像素值
 */
void idct_1d(double in[8], double out[8]) {
    int i, j;
    for (j = 0; j < 8; j++) {
        double sum = 0;
        for (i = 0; i < 8; i++) {
            double alpha = (i == 0) ? 0.35355339 : 0.5;
            sum += alpha * in[i] * COS_TABLE[i][j];
        }
        out[j] = sum;
    }
}

/**
 * fdct_8x8_fast: 二维快速 DCT 实现
 * 利用 DCT 的可分离特性 (Separability)，通过两次一维变换完成二维矩阵运算
 * 复杂度由 O(N^4) 降至 O(N^3)
 */
void fdct_8x8_fast(double in[8][8], double out[8][8]) {
    double temp[8][8];
    int i, j;
    // 步骤1：行变换
    for (i = 0; i < 8; i++) fdct_1d(in[i], temp[i]);
    // 步骤2：列变换
    for (j = 0; j < 8; j++) {
        double col_in[8], col_out[8];
        for (i = 0; i < 8; i++) col_in[i] = temp[i][j];
        fdct_1d(col_in, col_out);
        for (i = 0; i < 8; i++) out[i][j] = col_out[i];
    }
}

/**
 * idct_8x8_fast: 二维快速 IDCT 实现
 * 逆变换过程同样遵循可分离特性，执行逆向序列运算
 */
void idct_8x8_fast(double in[8][8], double out[8][8]) {
    double temp[8][8];
    int i, j;
    for (i = 0; i < 8; i++) idct_1d(in[i], temp[i]);
    for (j = 0; j < 8; j++) {
        double col_in[8], col_out[8];
        for (i = 0; i < 8; i++) col_in[i] = temp[i][j];
        idct_1d(col_in, col_out);
        for (i = 0; i < 8; i++) out[i][j] = col_out[i];
    }
}

/**
 * process_text_watermark: 水印处理主逻辑
 * @param mode: 1 为嵌入 (Embed), 2 为提取 (Extract)
 * @param strength: 嵌入强度，直接影响水印的鲁棒性与隐蔽性的平衡
 */
void process_text_watermark(const char* in_file, const char* out_file, int mode, const char* text) {
    BMPHeader header;
    BMPInfo info;
    FILE* f;
    uint8_t* pixels, * payload;
    int width, height, line_size, text_len, payload_len, total_bits;
    int i, j, r, c, block_idx = 0;
    double block[8][8], dct[8][8];
    double strength = 28.0;
    uint8_t current_char = 0;

    init_tables(); // 初始化查找表

    /* 1. 文件流处理与内存映射 */
    f = fopen(in_file, "rb");
    if (!f) return;
    fread(&header, sizeof(BMPHeader), 1, f);
    fread(&info, sizeof(BMPInfo), 1, f);
    width = info.width;
    height = abs(info.height);
    line_size = (width * 3 + 3) & ~3; // BMP 行字节对齐处理
    pixels = (uint8_t*)malloc(line_size * height);
    if (!pixels) { fclose(f); return; }
    fread(pixels, line_size * height, 1, f);
    fclose(f);

    /* 2. 数据载荷 (Payload) 构建 */
    text_len = (int)strlen(text);
    payload_len = text_len + 1; // 包含 1 字节同步头
    payload = (uint8_t*)malloc(payload_len);
    if (!payload) { free(pixels); return; }
    payload[0] = SYNC_MARKER;
    memcpy(payload + 1, text, text_len);
    total_bits = payload_len * 8;

    /* 3. 图像分块处理 (8x8 Blocks) */
    for (i = 0; i <= height - 8; i += 8) {
        for (j = 0; j <= width - 8; j += 8) {
            // 计算当前处理块对应的比特位（支持全图循环嵌入以增强冗余度）
            int current_bit_pos = block_idx % total_bits;
            int bit = (payload[current_bit_pos / 8] >> (7 - (current_bit_pos % 8))) & 1;

            // 提取蓝色通道进行 DCT 变换（人眼对蓝色分量的敏感度最低）
            for (r = 0; r < 8; r++)
                for (c = 0; c < 8; c++)
                    block[r][c] = (double)pixels[(i + r) * line_size + (j + c) * 3 + 2];

            fdct_8x8_fast(block, dct);

            if (mode == 1) { // 嵌入流程
                /* 基于中频系数比较的差分嵌入逻辑 */
                if (bit == 1) {
                    // 若嵌入比特为1，确保系数 (4,3) 显著大于 (3,4)
                    if (dct[4][3] <= dct[3][4]) dct[4][3] = dct[3][4] + strength;
                }
                else {
                    // 若嵌入比特为0，确保系数 (3,4) 显著大于 (4,3)
                    if (dct[4][3] >= dct[3][4]) dct[3][4] = dct[4][3] + strength;
                }

                idct_8x8_fast(dct, block); // 逆变换回空间域

                /* 像素重构与动态范围限幅 (Clipping) */
                for (r = 0; r < 8; r++) {
                    for (c = 0; c < 8; c++) {
                        double p = block[r][c];
                        pixels[(i + r) * line_size + (j + c) * 3 + 2] = (uint8_t)(p > 255 ? 255 : (p < 0 ? 0 : p));
                    }
                }
            }
            else if (mode == 2 && block_idx < total_bits) { // 提取流程
                /* 检测中频系数的相对大小以恢复比特信息 */
                int e_bit = (dct[4][3] > dct[3][4]) ? 1 : 0;
                current_char |= (e_bit << (7 - (block_idx % 8)));

                if ((block_idx + 1) % 8 == 0) {
                    if (block_idx / 8 == 0) { // 同步头校验
                        if (current_char != SYNC_MARKER) {
                            printf("\n[Fatal] Sync Header Mismatch. Terminating.\n");
                            free(pixels); free(payload); return;
                        }
                    }
                    else {
                        printf("%c", current_char); // 输出还原字符
                    }
                    current_char = 0;
                }
            }
            block_idx++;
        }
    }

    /* 4. 数据回写 */
    if (mode == 1) {
        FILE* out = fopen(out_file, "wb");
        if (out) {
            fwrite(&header, sizeof(BMPHeader), 1, out);
            fwrite(&info, sizeof(BMPInfo), 1, out);
            fwrite(pixels, line_size * height, 1, out);
            fclose(out);
        }
    }
    free(pixels); free(payload);
}