#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "dct_watermark.h"

int main() {
    // 解决控制台乱码
    system("chcp 65001");

    const char* text = "SECURE_ID_99872";
    const char* input = "test.bmp";
    const char* output = "result.bmp";

    printf("--- 快速 DCT 水印系统 ---\n");

    // 1. 嵌入
    printf("正在优化嵌入文字: [%s]...\n", text);
    process_text_watermark(input, output, 1, text);
    printf("嵌入成功，请检查: %s\n", output);

    // 2. 提取
    printf("从图中提取出的内容: ");
    process_text_watermark(output, NULL, 2, text);
    printf("\n------------------------\n");

    system("pause");
    return 0;
}