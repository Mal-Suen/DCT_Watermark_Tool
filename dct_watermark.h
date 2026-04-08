// dct_watermark.h
// 头文件保护宏，防止头文件被重复包含，避免重定义错误
#ifndef DCT_WATERMARK_H
#define DCT_WATERMARK_H

// 引入标准整数类型库，确保 int32_t, uint16_t 等类型的跨平台一致性
#include <stdint.h>

// --- C++ 兼容性支持 ---
// 如果在 C++ 环境下编译，extern "C" 告诉编译器以 C 语言的方式处理函数名，
// 避免 C++ 的名称修饰（name mangling）导致链接错误
#ifdef __cplusplus
extern "C" {
#endif

    /* --- BMP 文件格式结构体定义 --- */

    // 使用 pragma pack 指令，强制编译器以 1 字节对齐方式存储结构体成员
    // 这是处理二进制文件格式的关键，防止编译器自动填充字节导致结构体大小与文件格式不符
#pragma pack(push, 1)

// BMP 文件头结构体 (BMPHeader)，紧跟在文件开头，固定 14 字节
    typedef struct {
        uint16_t bfType;       // 文件标识符，必须为 'BM' (0x4D42)
        uint32_t bfSize;       // 整个文件的大小（字节）
        uint16_t bfReserved1;  // 保留字段，通常为 0
        uint16_t bfReserved2;  // 保留字段，通常为 0
        uint32_t bfOffBits;    // 从文件头到实际像素数据的偏移量（字节）
    } BMPHeader;

    // BMP 信息头结构体 (BMPInfo)，紧跟在 BMPHeader 之后，固定 40 字节 (biSize = 40)
    typedef struct {
        uint32_t biSize;          // 本结构体的大小，必须为 40
        int32_t  biWidth;         // 图像宽度（像素）
        int32_t  biHeight;        // 图像高度（像素），负值表示数据从顶到底存储
        uint16_t biPlanes;        // 平面数，必须为 1
        uint16_t biBitCount;      // 每个像素的位数，24 表示 RGB 三通道
        uint32_t biCompression;   // 压缩方式，0 表示无压缩
        uint32_t biSizeImage;     // 图像大小（字节），对于无压缩图像可以为 0
        int32_t  biXPelsPerMeter; // 水平分辨率（像素/米）
        int32_t  biYPelsPerMeter; // 垂直分辨率（像素/米）
        uint32_t biClrUsed;       // 调色板颜色数，对于 24 位图通常为 0
        uint32_t biClrImportant;  // 重要颜色数，通常为 0
    } BMPInfo;

    // 恢复编译器默认的对齐方式
#pragma pack(pop)

/* --- 返回状态码定义 --- */

// 定义函数返回状态码的枚举类型，便于调用者判断函数执行结果
    typedef enum {
        WM_SUCCESS = 0,              // 操作成功
        WM_ERROR_FILE_IO,            // 文件读写操作失败 (如文件不存在、权限不足)
        WM_ERROR_INVALID_FORMAT,     // 输入文件格式不符合要求 (如不是 BMP 或不是 24 位)
        WM_ERROR_MEMORY,             // 内存分配失败
        WM_ERROR_INSUFFICIENT_SPACE, // 图像空间不足以容纳水印数据
        WM_ERROR_NULL_PARAMETER,     // 参数为NULL
        WM_ERROR_DECODE_FAILED       // 解码失败
    } wm_status_t;

    /* --- 核心接口声明 --- */

    /**
     * @brief 核心的 DCT 域水印嵌入与提取函数
     *
     * 该函数在一个 24 位 BMP 图像的 DCT 域中，通过调整一对中频系数的相对大小来嵌入或提取文本水印。
     *
     * @param in_path  输入的 BMP 图像文件路径
     * @param out_path 输出的含水印 BMP 图像文件路径 (提取模式下可为 NULL)
     * @param mode     操作模式: 1 为嵌入模式, 2 为提取模式
     * @param text     要嵌入的文本字符串 (嵌入模式下使用，提取模式下忽略)
     * @param strength 嵌入强度，数值越大，水印越稳定但也可能越明显
     * @param marker   用于标识水印起始位置的同步标记字节，嵌入和提取时必须相同
     * @return wm_status_t 返回操作状态码
     */
    wm_status_t wm_process(const char* in_path, const char* out_path, int mode,
        const char* text, double strength, uint8_t marker);

    // --- C++ 兼容性支持结束 ---
#ifdef __cplusplus
}
#endif

#endif // DCT_WATERMARK_H