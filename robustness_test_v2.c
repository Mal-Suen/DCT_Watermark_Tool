/*
 * 鲁棒性测试工具 v2 - 带水印内容验证
 * 测试后生成详细报告
 */

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_WATERMARK_LEN 1024
#define ORIG_WATERMARK "Robustness Test Message: Hello World!"

/* 攻击类型定义 */
typedef struct {
    char name[64];
    char file[256];
    int passed;
    char decoded[MAX_WATERMARK_LEN];
    int similarity; // 相似度百分比
} attack_test_t;

/* 计算字符串相似度（简单版本）*/
int calc_similarity(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0;
    
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    if (len1 == 0 || len2 == 0) return 0;
    
    // 简单字符匹配计数
    int matches = 0;
    int min_len = len1 < len2 ? len1 : len2;
    
    for (int i = 0; i < min_len; i++) {
        if (s1[i] == s2[i]) matches++;
    }
    
    return (int)((float)matches / len2 * 100.0f);
}

/* 各种攻击函数 */
void test_crop(const char* input, const char* output, float percent) {
    int w, h, ch;
    uint8_t* pix = stbi_load(input, &w, &h, &ch, 0);
    if (!pix) return;
    
    int new_w = (int)(w * sqrt(percent / 100.0f));
    int new_h = (int)(h * sqrt(percent / 100.0f));
    int ox = (w - new_w) / 2, oy = (h - new_h) / 2;
    
    uint8_t* new_pix = (uint8_t*)malloc(new_w * new_h * ch);
    for (int y = 0; y < new_h; y++)
        memcpy(new_pix + y * new_w * ch, pix + ((y + oy) * w + ox) * ch, new_w * ch);
    
    stbi_write_bmp(output, new_w, new_h, ch, new_pix);
    free(new_pix);
    stbi_image_free(pix);
}

void test_noise(const char* input, const char* output, int level) {
    int w, h, ch;
    uint8_t* pix = stbi_load(input, &w, &h, &ch, 0);
    if (!pix) return;
    
    srand(42);
    for (int i = 0; i < w * h * ch; i++) {
        int noise = (rand() % (level * 2 + 1)) - level;
        int val = pix[i] + noise;
        pix[i] = (uint8_t)(val > 255 ? 255 : (val < 0 ? 0 : val));
    }
    
    stbi_write_bmp(output, w, h, ch, pix);
    stbi_image_free(pix);
}

void test_blur(const char* input, const char* output, int kernel) {
    int w, h, ch;
    uint8_t* pix = stbi_load(input, &w, &h, &ch, 0);
    if (!pix) return;
    
    uint8_t* new_pix = (uint8_t*)malloc(w * h * ch);
    int half = kernel / 2;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            for (int c = 0; c < ch; c++) {
                int sum = 0, count = 0;
                for (int ky = -half; ky <= half; ky++) {
                    for (int kx = -half; kx <= half; kx++) {
                        int ny = y + ky, nx = x + kx;
                        if (ny >= 0 && ny < h && nx >= 0 && nx < w) {
                            sum += pix[(ny * w + nx) * ch + c];
                            count++;
                        }
                    }
                }
                new_pix[(y * w + x) * ch + c] = (uint8_t)(sum / count);
            }
        }
    }
    
    stbi_write_bmp(output, w, h, ch, new_pix);
    free(new_pix);
    stbi_image_free(pix);
}

void test_brightness(const char* input, const char* output, float factor) {
    int w, h, ch;
    uint8_t* pix = stbi_load(input, &w, &h, &ch, 0);
    if (!pix) return;
    
    for (int i = 0; i < w * h * ch; i++) {
        int val = (int)(pix[i] * factor);
        pix[i] = (uint8_t)(val > 255 ? 255 : (val < 0 ? 0 : val));
    }
    
    stbi_write_bmp(output, w, h, ch, pix);
    stbi_image_free(pix);
}

void test_resize(const char* input, const char* output, float scale) {
    int w, h, ch;
    uint8_t* pix = stbi_load(input, &w, &h, &ch, 0);
    if (!pix) return;
    
    int new_w = (int)(w * scale), new_h = (int)(h * scale);
    uint8_t* new_pix = (uint8_t*)malloc(new_w * new_h * ch);
    
    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            int sx = (int)((float)x / new_w * w);
            int sy = (int)((float)y / new_h * h);
            for (int c = 0; c < ch; c++)
                new_pix[(y * new_w + x) * ch + c] = pix[(sy * w + sx) * ch + c];
        }
    }
    
    stbi_write_bmp(output, new_w, new_h, ch, new_pix);
    free(new_pix);
    stbi_image_free(pix);
}

void test_jpeg(const char* input, const char* output, int quality) {
    int w, h, ch;
    uint8_t* pix = stbi_load(input, &w, &h, &ch, 0);
    if (!pix) return;
    
    stbi_write_jpg(output, w, h, ch, pix, quality);
    stbi_image_free(pix);
}

int run_decode(const char* input, char* output, int out_size) {
    const char* tmp = "temp_decode.txt";
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "dct_watermark_tool.exe decode \"%s\" > \"%s\" 2>&1", input, tmp);
    system(cmd);
    
    FILE* fp = fopen(tmp, "r");
    int success = 0;
    output[0] = '\0';
    
    if (fp) {
        char line[1024], prev[1024] = {0};
        while (fgets(line, sizeof(line), fp)) {
            size_t len = strlen(line);
            if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
            
            if (strstr(line, "[Success at offset")) {
                if (strlen(prev) > 0) {
                    strncpy(output, prev, out_size - 1);
                    output[out_size - 1] = '\0';
                }
                success = 1;
                break;
            }
            strcpy(prev, line);
        }
        fclose(fp);
    }
    
    remove(tmp);
    return success;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    system("chcp 65001 >nul 2>&1");
#endif
    
    if (argc < 2) {
        printf("用法: %s <含水印图像>\n", argv[0]);
        return 1;
    }
    
    system("if not exist robustness_tests mkdir robustness_tests");
    
    printf("\n========================================\n");
    printf("  DCT 水印鲁棒性测试工具 v2\n");
    printf("========================================\n");
    printf("原始水印: %s\n", ORIG_WATERMARK);
    printf("测试图像: %s\n", argv[1]);
    printf("========================================\n\n");
    
    attack_test_t tests[30];
    int test_count = 0;
    int pass_count = 0;
    
    /* 定义测试用例 */
    struct {
        char name[64];
        int type; // 0=crop, 1=noise, 2=blur, 3=brightness, 4=resize, 5=jpeg
        float param;
    } test_defs[] = {
        {"基准测试-原始图像", -1, 0},
        {"裁剪攻击-75%%", 0, 75},
        {"裁剪攻击-50%%", 0, 50},
        {"裁剪攻击-25%%", 0, 25},
        {"噪声攻击-级别10", 1, 10},
        {"噪声攻击-级别20", 1, 20},
        {"噪声攻击-级别30", 1, 30},
        {"噪声攻击-级别50", 1, 50},
        {"模糊攻击-核3", 2, 3},
        {"模糊攻击-核5", 2, 5},
        {"模糊攻击-核7", 2, 7},
        {"亮度调整-0.5", 3, 0.5},
        {"亮度调整-0.7", 3, 0.7},
        {"亮度调整-1.5", 3, 1.5},
        {"缩放-50%%", 4, 0.5},
        {"缩放-75%%", 4, 0.75},
        {"缩放-150%%", 4, 1.5},
        {"JPEG-质量95", 5, 95},
        {"JPEG-质量90", 5, 90},
        {"JPEG-质量80", 5, 80},
        {"JPEG-质量70", 5, 70},
    };
    
    int num_tests = sizeof(test_defs) / sizeof(test_defs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        char outfile[256];
        test_count++;
        
        printf("[测试 %d/%d] %s... ", test_count, num_tests, test_defs[i].name);
        fflush(stdout);
        
        // 生成攻击后的图像
        switch (test_defs[i].type) {
            case -1: // 基准测试
                snprintf(outfile, sizeof(outfile), "%s/baseline.bmp", "robustness_tests");
                break;
            case 0: // 裁剪
                snprintf(outfile, sizeof(outfile), "%s/crop_%.0f.bmp", "robustness_tests", test_defs[i].param);
                test_crop(argv[1], outfile, test_defs[i].param);
                break;
            case 1: // 噪声
                snprintf(outfile, sizeof(outfile), "%s/noise_%.0f.bmp", "robustness_tests", test_defs[i].param);
                test_noise(argv[1], outfile, (int)test_defs[i].param);
                break;
            case 2: // 模糊
                snprintf(outfile, sizeof(outfile), "%s/blur_%.0f.bmp", "robustness_tests", test_defs[i].param);
                test_blur(argv[1], outfile, (int)test_defs[i].param);
                break;
            case 3: // 亮度
                snprintf(outfile, sizeof(outfile), "%s/bright_%.1f.bmp", "robustness_tests", test_defs[i].param);
                test_brightness(argv[1], outfile, test_defs[i].param);
                break;
            case 4: // 缩放
                snprintf(outfile, sizeof(outfile), "%s/resize_%.2f.bmp", "robustness_tests", test_defs[i].param);
                test_resize(argv[1], outfile, test_defs[i].param);
                break;
            case 5: // JPEG
                snprintf(outfile, sizeof(outfile), "%s/jpeg_q%.0f.jpg", "robustness_tests", test_defs[i].param);
                test_jpeg(argv[1], outfile, (int)test_defs[i].param);
                break;
        }
        
        // 运行解码
        strncpy(tests[i].name, test_defs[i].name, sizeof(tests[i].name) - 1);
        strncpy(tests[i].file, outfile, sizeof(tests[i].file) - 1);
        tests[i].passed = run_decode(outfile, tests[i].decoded, sizeof(tests[i].decoded));
        tests[i].similarity = calc_similarity(tests[i].decoded, ORIG_WATERMARK);
        
        if (tests[i].passed) {
            pass_count++;
            printf("通过 (相似度: %d%%)\n", tests[i].similarity);
        } else {
            printf("失败\n");
        }
    }
    
    /* 打印详细报告 */
    printf("\n\n========================================\n");
    printf("  详细测试报告\n");
    printf("========================================\n\n");
    
    printf("%-25s %-8s %-10s %s\n", "测试项", "状态", "相似度", "解码内容");
    printf("%-25s %-8s %-10s %s\n", "-------------------------", "--------", "----------", "--------------------");
    
    for (int i = 0; i < test_count; i++) {
        printf("%-25s %-8s %-10d%% ", 
               tests[i].name,
               tests[i].passed ? "通过" : "失败",
               tests[i].similarity);
        
        if (tests[i].passed) {
            // 只显示前30个字符
            char short_text[35];
            strncpy(short_text, tests[i].decoded, 30);
            short_text[30] = '\0';
            if (strlen(tests[i].decoded) > 30) strcat(short_text, "...");
            printf("%s", short_text);
        } else {
            printf("[无法解码]");
        }
        printf("\n");
    }
    
    /* 汇总 */
    printf("\n========================================\n");
    printf("  测试总结\n");
    printf("========================================\n");
    printf("总测试数: %d\n", test_count);
    printf("通过数量: %d\n", pass_count);
    printf("失败数量: %d\n", test_count - pass_count);
    printf("通过率: %.1f%%\n", (float)pass_count / test_count * 100.0f);
    
    /* 分类统计 */
    int crop_pass = 0, noise_pass = 0, blur_pass = 0, bright_pass = 0, resize_pass = 0, jpeg_pass = 0;
    for (int i = 1; i < test_count; i++) { // 跳过基准测试
        if (strstr(tests[i].name, "裁剪") && tests[i].passed) crop_pass++;
        if (strstr(tests[i].name, "噪声") && tests[i].passed) noise_pass++;
        if (strstr(tests[i].name, "模糊") && tests[i].passed) blur_pass++;
        if (strstr(tests[i].name, "亮度") && tests[i].passed) bright_pass++;
        if (strstr(tests[i].name, "缩放") && tests[i].passed) resize_pass++;
        if (strstr(tests[i].name, "JPEG") && tests[i].passed) jpeg_pass++;
    }
    
    printf("\n分类通过率:\n");
    printf("  裁剪攻击: %d/3 (%.0f%%)\n", crop_pass, (float)crop_pass / 3 * 100);
    printf("  噪声攻击: %d/4 (%.0f%%)\n", noise_pass, (float)noise_pass / 4 * 100);
    printf("  模糊攻击: %d/3 (%.0f%%)\n", blur_pass, (float)blur_pass / 3 * 100);
    printf("  亮度调整: %d/3 (%.0f%%)\n", bright_pass, (float)bright_pass / 3 * 100);
    printf("  缩放攻击: %d/3 (%.0f%%)\n", resize_pass, (float)resize_pass / 3 * 100);
    printf("  JPEG压缩: %d/4 (%.0f%%)\n", jpeg_pass, (float)jpeg_pass / 4 * 100);
    
    printf("\n========================================\n");
    if (pass_count == test_count) {
        printf("[优秀] 水印具有极强的鲁棒性！\n");
    } else if (pass_count >= test_count * 0.8) {
        printf("[良好] 水印具有良好的鲁棒性。\n");
    } else if (pass_count >= test_count * 0.6) {
        printf("[中等] 建议提高水印强度。\n");
    } else {
        printf("[较差] 需要显著提高水印强度。\n");
    }
    printf("========================================\n");
    
    return 0;
}
