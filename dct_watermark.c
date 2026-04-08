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

/* --- Reed-Solomon编码器 (GF(256)) --- */
// 完整的RS实现，支持错误纠正
// 使用标准的RS编码和解码算法

// GF(256)运算
static uint8_t gf_exp[512];
static uint8_t gf_log[256];
static int gf_initialized = 0;

static void gf_init() {
    if (gf_initialized) return;
    uint16_t x = 1;
    for (int i = 0; i < 255; i++) {
        gf_exp[i] = (uint8_t)x;
        gf_log[x] = (uint8_t)i;
        x <<= 1;
        if (x & 0x100) x ^= 0x11D;  // 生成多项式: x^8 + x^4 + x^3 + x^2 + 1
    }
    for (int i = 255; i < 512; i++) gf_exp[i] = gf_exp[i - 255];
    gf_initialized = 1;
}

static uint8_t gf_mul(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) return 0;
    return gf_exp[(int)gf_log[a] + gf_log[b]];
}

static uint8_t gf_div(uint8_t a, uint8_t b) {
    if (a == 0) return 0;
    if (b == 0) return 0xFF;
    return gf_exp[((int)gf_log[a] - gf_log[b] + 255) % 255];
}

/**
 * @brief RS编码
 */
static void rs_encode(uint8_t* data, int data_len, uint8_t* encoded, int nsym) {
    gf_init();
    memcpy(encoded, data, data_len);
    
    // 初始化发生器多项式
    uint8_t gen[256];
    gen[0] = 1;
    for (int i = 0; i < nsym; i++) {
        gen[i + 1] = 1;
        for (int j = i; j > 0; j--) {
            gen[j] = gf_mul(gen[j], gf_exp[i]) ^ gen[j - 1];
        }
        gen[0] = gf_mul(gen[0], gf_exp[i]);
    }
    
    // 计算校验符号
    uint8_t remainder[256] = {0};
    for (int i = 0; i < data_len; i++) {
        uint8_t coef = data[i] ^ remainder[0];
        memmove(remainder, remainder + 1, nsym - 1);
        remainder[nsym - 1] = 0;
        for (int j = 0; j < nsym; j++) {
            remainder[j] ^= gf_mul(gen[j], coef);
        }
    }
    
    memcpy(encoded + data_len, remainder, nsym);
}

/**
 * @brief RS解码（带错误纠正）
 */
static int rs_decode(uint8_t* encoded, int total_len, int nsym, uint8_t* decoded) {
    gf_init();
    int data_len = total_len - nsym;
    if (data_len <= 0 || nsym <= 0) return -1;
    
    // 计算syndrome
    uint8_t synd[256];
    for (int i = 0; i < nsym; i++) {
        synd[i] = 0;
        for (int j = 0; j < total_len; j++) {
            synd[i] = gf_mul(synd[i], gf_exp[i]) ^ encoded[j];
        }
    }
    
    // 检查是否有错误
    int has_error = 0;
    for (int i = 0; i < nsym; i++) {
        if (synd[i] != 0) { has_error = 1; break; }
    }
    
    // 无错误，直接返回
    if (!has_error) {
        memcpy(decoded, encoded, data_len);
        return data_len;
    }
    
    // 使用Berlekamp-Massey算法查找错误定位多项式
    uint8_t err_loc[256] = {1};
    uint8_t old_loc[256] = {1};
    int err_count = 0;
    
    for (int i = 0; i < nsym; i++) {
        uint8_t delta = synd[i];
        for (int j = 1; j <= err_count; j++) {
            delta ^= gf_mul(err_loc[err_count - j], synd[i - j]);
        }
        
        memmove(old_loc + 1, old_loc, 255);
        old_loc[0] = 0;
        
        if (delta != 0) {
            if (err_count <= i) {
                for (int j = 0; j <= err_count; j++) {
                    old_loc[j] = gf_div(err_loc[j], delta);
                }
                err_count = i + 1 - err_count;
            }
        }
        
        if (err_count > nsym / 2) return -1;  // 错误太多
    }
    
    // 找到错误位置并纠正
    uint8_t errs[256];
    int err_pos = 0;
    for (int i = 0; i < total_len; i++) {
        uint8_t X = gf_exp[255 - i];
        uint8_t Xl = 1;
        for (int j = 0; j <= err_count; j++) {
            Xl = gf_mul(Xl, X);
        }
        
        uint8_t sum = 0;
        for (int j = 0; j <= err_count; j++) {
            sum ^= gf_mul(err_loc[j], Xl);
            Xl = gf_mul(Xl, X);
        }
        
        if (sum == 0) {
            if (err_pos < 256) errs[err_pos++] = i;
        }
    }
    
    if (err_pos > nsym / 2) return -1;
    
    // 纠正错误
    uint8_t corrected[512];
    memcpy(corrected, encoded, total_len);
    
    for (int e = 0; e < err_pos; e++) {
        uint8_t X = gf_exp[255 - errs[e]];
        uint8_t num = 0, den = 0;
        
        for (int i = 0; i < nsym; i++) {
            num ^= gf_mul(synd[i], gf_mul(X, gf_exp[i * (nsym - 1 - errs[e])]));
            den ^= gf_mul(synd[i], gf_exp[i * (nsym - 1 - errs[e])]);
        }
        
        if (den != 0) {
            uint8_t error_val = gf_div(num, den);
            corrected[errs[e]] ^= error_val;
        }
    }
    
    memcpy(decoded, corrected, data_len);
    return data_len;
}

/* --- 常量定义 --- */
#define WM_DEFAULT_STRENGTH     50.0        // 默认嵌入强度（平衡点：质量vs鲁棒性）
#define WM_EARLY_EXIT_BLOCKS    300         // 早期退出：扫描块数阈值
#define WM_JPEG_QUALITY         100         // JPEG保存质量(0-100)
#define WM_MAX_EXT_LEN          32          // 最大文件扩展名长度
#define WM_SYNC_MARKER_DEFAULT  0xAA        // 默认同步标记字节
#define WM_BLOCK_SIZE           8           // DCT块大小
#define WM_COEFF_PAIRS          6           // 使用的系数对数量（增加到6）
#define WM_BIT_REPEAT           13          // 固定重复次数（适应长文本）

// 多系数对定义 - 覆盖低中高频，最大化鲁棒性
static const int COEFF_PAIRS[WM_COEFF_PAIRS][4] = {
    {1, 2, 2, 1},  // 系数对1: (1,2) vs (2,1) - 最低频
    {1, 3, 3, 1},  // 系数对2: (1,3) vs (3,1) - 低频
    {2, 3, 3, 2},  // 系数对3: (2,3) vs (3,2) - 中低频
    {1, 4, 4, 1},  // 系数对4: (1,4) vs (4,1) - 低频
    {2, 4, 4, 2},  // 系数对5: (2,4) vs (4,2) - 中频
    {3, 4, 4, 3},  // 系数对6: (3,4) vs (4,3) - 中频（原始系数对）
};

/* --- 内部静态查找表与常量 --- */
static double G_COS_TBL[8][8];
static double G_ALPHA[8];
static int G_TBL_INIT = 0;

/**
 * @brief 初始化 DCT 查找表
 * 此函数只在程序运行期间第一次被调用时执行。
 * 注意：此函数不是线程安全的，如需多线程支持请使用原子操作或互斥锁
 */
static void init_tables(void) {
    if (G_TBL_INIT) return;
    for (int i = 0; i < WM_BLOCK_SIZE; i++) {
        for (int j = 0; j < WM_BLOCK_SIZE; j++) 
            G_COS_TBL[i][j] = cos((2 * j + 1) * i * M_PI / 16.0);
        G_ALPHA[i] = (i == 0) ? 0.35355339 : 0.5;
    }
    G_TBL_INIT = 1;
}

/* --- DCT 变换核心 --- */
/**
 * @brief 8x8 块的快速正向 DCT 变换
 * @param in 输入8x8像素块
 * @param out 输出DCT系数块
 */
static void fdct_8x8(double in[WM_BLOCK_SIZE][WM_BLOCK_SIZE], double out[WM_BLOCK_SIZE][WM_BLOCK_SIZE]) {
    double tmp[WM_BLOCK_SIZE][WM_BLOCK_SIZE];
    for (int i = 0; i < WM_BLOCK_SIZE; i++) {
        for (int u = 0; u < WM_BLOCK_SIZE; u++) {
            double s = 0;
            for (int j = 0; j < WM_BLOCK_SIZE; j++) s += in[i][j] * G_COS_TBL[u][j];
            tmp[i][u] = s * G_ALPHA[u];
        }
    }
    for (int j = 0; j < WM_BLOCK_SIZE; j++) {
        for (int v = 0; v < WM_BLOCK_SIZE; v++) {
            double s = 0;
            for (int i = 0; i < WM_BLOCK_SIZE; i++) s += tmp[i][j] * G_COS_TBL[v][i];
            out[v][j] = s * G_ALPHA[v];
        }
    }
}

/**
 * @brief 8x8 块的快速逆向 DCT 变换
 * @param in 输入DCT系数块
 * @param out 输出8x8像素块
 */
static void idct_8x8(double in[WM_BLOCK_SIZE][WM_BLOCK_SIZE], double out[WM_BLOCK_SIZE][WM_BLOCK_SIZE]) {
    double tmp[WM_BLOCK_SIZE][WM_BLOCK_SIZE];
    for (int i = 0; i < WM_BLOCK_SIZE; i++) {
        for (int x = 0; x < WM_BLOCK_SIZE; x++) {
            double s = 0;
            for (int u = 0; u < WM_BLOCK_SIZE; u++) s += G_ALPHA[u] * in[i][u] * G_COS_TBL[u][x];
            tmp[i][x] = s;
        }
    }
    for (int x = 0; x < WM_BLOCK_SIZE; x++) {
        for (int y = 0; y < WM_BLOCK_SIZE; y++) {
            double s = 0;
            for (int i = 0; i < WM_BLOCK_SIZE; i++) s += G_ALPHA[i] * tmp[i][x] * G_COS_TBL[i][y];
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
    if (!pix) return 0;

    uint8_t shift = 0;
    int state = 0, bit_idx = 0, found_any = 0;
    int repeat_count = 0;
    int repeat_votes_1 = 0;
    int blocks_checked = 0;

    for (int i = oy; i <= h - WM_BLOCK_SIZE; i += WM_BLOCK_SIZE) {
        for (int j = ox; j <= w - WM_BLOCK_SIZE; j += WM_BLOCK_SIZE) {
            blocks_checked++;
            if (state == 0 && blocks_checked > 4096 * 4)
                return 0;

            double b[WM_BLOCK_SIZE][WM_BLOCK_SIZE], d[WM_BLOCK_SIZE][WM_BLOCK_SIZE];
            for (int r = 0; r < WM_BLOCK_SIZE; r++) {
                uint8_t* p = pix + ((i + r) * w + j) * ch;
                for (int c = 0; c < WM_BLOCK_SIZE; c++)
                    b[r][c] = (double)p[c * ch + 2];
            }

            fdct_8x8(b, d);

            int vote_1 = 0;
            for (int k = 0; k < WM_COEFF_PAIRS; k++) {
                int r1 = COEFF_PAIRS[k][0];
                int c1 = COEFF_PAIRS[k][1];
                int r2 = COEFF_PAIRS[k][2];
                int c2 = COEFF_PAIRS[k][3];
                if (d[r1][c1] > d[r2][c2]) vote_1++;
            }

            if (vote_1 > WM_COEFF_PAIRS / 2) repeat_votes_1++;
            repeat_count++;

            if (repeat_count == WM_BIT_REPEAT) {
                int bit = (repeat_votes_1 > WM_BIT_REPEAT / 2) ? 1 : 0;
                shift = (shift << 1) | bit;
                repeat_count = 0;
                repeat_votes_1 = 0;

                if (state == 0) {
                    if (shift == marker) {
                        state = 1;
                        bit_idx = 0;
                        shift = 0;
                    }
                }
                else {
                    if (++bit_idx == 8) {
                        if (shift == 0) return 1;
                        putchar(shift);
                        bit_idx = 0;
                        shift = 0;
                        found_any = 1;
                    }
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
    if (!path) return;
    
    char ext[WM_MAX_EXT_LEN] = { 0 };
    const char* dot = strrchr(path, '.');
    if (!dot) { 
        stbi_write_png(path, w, h, ch, pix, w * ch); 
        return;  // 如果没有找到扩展名，默认保存为 PNG
    }

    // 安全地复制扩展名
    int i = 0;
    dot++; // 跳过 '.'
    while (dot[i] && i < WM_MAX_EXT_LEN - 1) {
        ext[i] = (char)tolower((unsigned char)dot[i]);
        i++;
    }

    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
        stbi_write_jpg(path, w, h, ch, pix, WM_JPEG_QUALITY); // 质量设为100以减少DCT损失
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
    // 输入参数验证
    if (!in_path) return WM_ERROR_NULL_PARAMETER;
    if (mode == 1 && (!out_path || !text)) return WM_ERROR_NULL_PARAMETER;
    if (mode != 1 && mode != 2) return WM_ERROR_INVALID_FORMAT;
    if (strength < 0) strength = WM_DEFAULT_STRENGTH;
    if (marker == 0) marker = WM_SYNC_MARKER_DEFAULT;

    init_tables();
    int w, h, ch;
    uint8_t* pix = stbi_load(in_path, &w, &h, &ch, 0);
    if (!pix) return WM_ERROR_FILE_IO;

    // 检查图像尺寸
    if (w < WM_BLOCK_SIZE || h < WM_BLOCK_SIZE) {
        stbi_image_free(pix);
        return WM_ERROR_INVALID_FORMAT;
    }

    if (mode == 1) { // 嵌入模式
        size_t p_len = strlen(text) + 2;  // +1 for marker, +1 for terminator
        uint8_t* buf = (uint8_t*)calloc(p_len, 1);
        if (!buf) {
            stbi_image_free(pix);
            return WM_ERROR_MEMORY;
        }

        buf[0] = marker;
        memcpy(buf + 1, text, strlen(text));
        int total_bits = (int)p_len * 8;
        int b_idx = 0;

        // 使用固定重复次数，编码解码一致
        int repeat_count = WM_BIT_REPEAT;

        for (int i = 0; i <= h - WM_BLOCK_SIZE; i += WM_BLOCK_SIZE) {
            for (int j = 0; j <= w - WM_BLOCK_SIZE; j += WM_BLOCK_SIZE) {
                int current_bit_idx = b_idx / repeat_count;
                if (current_bit_idx >= total_bits) break;
                int bit = (buf[(current_bit_idx % total_bits) / 8] >> (7 - (current_bit_idx % total_bits) % 8)) & 1;
                b_idx++;

                double b[WM_BLOCK_SIZE][WM_BLOCK_SIZE], d[WM_BLOCK_SIZE][WM_BLOCK_SIZE];
                for (int r = 0; r < WM_BLOCK_SIZE; r++) {
                    uint8_t* p = pix + ((i + r) * w + j) * ch;
                    for (int c = 0; c < WM_BLOCK_SIZE; c++)
                        b[r][c] = (double)p[c * ch + 2];
                }
                fdct_8x8(b, d);

                // 使用多个系数对嵌入同一bit（提高鲁棒性）
                for (int k = 0; k < WM_COEFF_PAIRS; k++) {
                    int r1 = COEFF_PAIRS[k][0];
                    int c1 = COEFF_PAIRS[k][1];
                    int r2 = COEFF_PAIRS[k][2];
                    int c2 = COEFF_PAIRS[k][3];

                    if (bit) { // 嵌入1: 让 d[r1][c1] > d[r2][c2]
                        if (d[r1][c1] <= d[r2][c2]) {
                            d[r1][c1] = d[r2][c2] + strength;
                            d[r2][c2] -= strength;
                        }
                    }
                    else { // 嵌入0: 让 d[r1][c1] < d[r2][c2]
                        if (d[r1][c1] >= d[r2][c2]) {
                            d[r1][c1] = d[r2][c2] - strength;
                            d[r2][c2] += strength;
                        }
                    }
                }

                idct_8x8(d, b);
                for (int r = 0; r < WM_BLOCK_SIZE; r++) {
                    uint8_t* p = pix + ((i + r) * w + j) * ch;
                    for (int c = 0; c < WM_BLOCK_SIZE; c++) {
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
        for (int oy = 0; oy < WM_BLOCK_SIZE && !ok; oy++) {
            for (int ox = 0; ox < WM_BLOCK_SIZE && !ok; ox++) {
                if (scan_watermark(pix, w, h, ch, ox, oy, marker)) {
                    printf("\n[Success at offset %d,%d]\n", ox, oy);
                    ok = 1;
                }
            }
        }
        if (!ok) {
            printf("No watermark detected.\n");
            stbi_image_free(pix);
            return WM_ERROR_DECODE_FAILED;
        }
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