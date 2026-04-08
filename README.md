# DCT Watermark Tool | DCT离散余弦变换水印工具

<div align="center">

**Language / 语言选择:**
[English](#english) | [中文](#中文)

---
</div>
<a id="english"></a>

# DCT Watermark Tool

A lightweight and efficient C library for embedding and extracting text watermarks in images of various formats. This project employs the **Discrete Cosine Transform** (DCT) algorithm to hide watermark information in the mid-frequency domain of an image. It offers excellent visual invisibility and strong robustness, capable of withstanding attacks such as cropping and tampering.

## Features

- **Multi-Format Support**: Powered by the powerful `stb_image` library, it supports loading and saving mainstream image formats including **PNG, JPEG, BMP, TGA**, etc.
- **High Robustness**: Utilizes full-image redundant embedding and intelligent offset-searching techniques. It can reliably extract watermark information even when the image is **cropped by half or subjected to tampering attacks**.
- **DCT Domain Watermarking**: Operates on the image's DCT coefficients, resulting in minimal visual impact and preserving the original image's high quality.
- **Efficient Implementation**: Achieves fast processing through pre-calculated lookup tables (LUTs) and optimized DCT/IDCT algorithms.
- **Command-Line Interface**: Provides a simple and easy-to-use command-line tool, convenient for batch processing.
- **Embed/Extract**: Supports embedding text information into images and losslessly extracting the original information from them.
- **Intelligent Saving**: Automatically selects the corresponding image format for saving based on the output file's extension.
- **Flexible Strength**: Users can customize the embedding strength to balance between watermark invisibility and interference resistance.

## Build Environment

- **Operating Systems**: Linux, macOS, Windows (MinGW/Cygwin)
- **Compiler**: GCC or Clang (requires C99 standard support)
- **Dependencies**: `libm` (Math library). The project includes `stb_image.h` and `stb_image_write.h`, no additional installation is required.

## Compilation

After cloning the project locally, navigate to the project directory and use the following command to compile a command-line tool with a debug interface:

```bash
gcc -DDEBUG_MODE -o dct_watermark_tool dct_watermark.c -lm
```

This will generate an executable named `dct_watermark_tool`.

## Usage

### 1. Embed Watermark (`encode`)

Hides text information within an image.

```bash
./dct_watermark_tool encode <input_image_path> <output_image_path> "<text_to_embed>" [strength]
```

- `encode`: Specifies the embedding mode.
- `<input_image_path>`: Path to the source image (supports PNG, JPG, BMP, TGA, etc.).
- `<output_image_path>`: Path for the generated watermarked image (the extension determines the output format).
- `"<text_to_embed>"`: The text you want to embed. Quotes are mandatory if the text contains spaces.
- `[strength]`: Optional parameter, defaults to `20.0`. A higher value results in a more stable but potentially more visible watermark.

**Example**:

```bash
# Embed "Secret Message" into image.jpg, generating output.png
./dct_watermark_tool encode image.jpg output.png "Secret Message"

# Specify a strength of 30.0
./dct_watermark_tool encode image.jpg output.png "Secret Message" 30.0
```

### 2. Extract Watermark (`decode`)

Recovers text from a watermarked image.

```bash
./dct_watermark_tool decode <watermarked_image_path>
```

- `decode`: Specifies the extraction mode.
- `<watermarked_image_path>`: Path to the image containing the watermark.

**Example**:

```bash
# Extract and print the watermark text from output.png
./dct_watermark_tool decode output.png
```

## Integration as a Library

If you wish to integrate this functionality into your own C project, you can do so as follows:

1. Copy the files `dct_watermark.h`, `dct_watermark.c`, `stb_image.h`, and `stb_image_write.h` into your project directory.
2. Include the header file in your source code: `#include "dct_watermark.h"`.
3. When compiling, compile `dct_watermark.c` along with your other source files and link the math library `-lm`. **Note**: Do not define the `DEBUG_MODE` macro. This prevents the compilation of the `main` function at the end of the file, making it exist purely as a library.

The core API is the `wm_process` function. Its detailed description can be found in the `dct_watermark.h` header file.

## Status Codes

The return value of the command-line tool represents the operation status:

- `0`: Success (WM_SUCCESS)
- `1`: File I/O error (WM_ERROR_FILE_IO)
- `2`: Invalid file format or image too small (WM_ERROR_INVALID_FORMAT)
- `3`: Insufficient memory (WM_ERROR_MEMORY)
- `4`: Insufficient image space to accommodate the watermark (WM_ERROR_INSUFFICIENT_SPACE)

## Algorithm Principle

This project is based on the concept of **Quantization Index Modulation** (QIM). Specifically, it redundantly embeds the watermark information (sync marker, text content, terminator) across the entire image into each 8x8 pixel block. It selects two mid-frequency coefficients from each DCT block (e.g., (3,4) and (4,3)) and encodes one bit of information by adjusting their relative magnitude relationship (which one is larger). During extraction, the program performs an 8x8 sliding offset search, scanning image blocks from different starting points to cope with potential cropping, until it finds the sync marker and recovers the complete original text.

<a id="中文"></a>

# DCT Watermark Tool

一个轻量级、高效的 C 语言库，用于在多种格式的图像中嵌入和提取文本水印。该项目采用**离散余弦变换**（DCT）算法，将水印信息隐藏在图像的中频域，不仅具备出色的视觉隐蔽性，还拥有强大的鲁棒性，能够抵抗裁剪、涂抹等多种攻击。 

## 特性

- **多格式支持**: 通过强大的 `stb_image` 库，支持加载和保存包括 **PNG, JPEG, BMP, TGA** 等在内的主流图像格式。
- **高鲁棒性**: 采用全图冗余嵌入和智能偏移搜索技术，即使图像被**裁剪一半或遭受涂抹攻击**，依然能可靠地提取出水印信息。
- **DCT 域水印**: 在图像的 DCT 系数中进行操作，对视觉影响极小，保持了原图的高质量。
- **高效实现**: 通过预计算查找表（LUTs）和优化的 DCT/IDCT 算法，实现了快速处理。
- **命令行界面**: 提供简洁易用的命令行工具，方便批量处理。
- **嵌入/提取**: 支持将文本信息嵌入图像，以及从图像中无损提取原始信息。
- **智能保存**: 自动根据输出文件的扩展名选择对应的图像格式进行保存。
- **灵活强度**: 用户可自定义嵌入强度，在水印的不可见性和抗干扰能力之间取得平衡。

## 编译环境

*   **操作系统**: Linux, macOS, Windows (MinGW/Cygwin)
*   **编译器**: GCC 或 Clang (需支持 C99 标准)
*   **依赖库**: `libm` (数学库)。项目已包含 `stb_image.h` 和 `stb_image_write.h`，无需额外安装。

## 编译

克隆项目到本地后，进入项目目录，使用以下命令编译一个带有调试界面的命令行工具：

```bash
gcc -DDEBUG_MODE -o dct_watermark_tool dct_watermark.c -lm
```

这将生成一个名为 `dct_watermark_tool` 的可执行文件。

## 用法

### 1. 嵌入水印 (`encode`)

将文本信息隐藏到 BMP 图像中。

```bash
./dct_watermark_tool encode <输入图像路径> <输出图像路径> "<要嵌入的文本>" [强度]
```

- `encode`: 指定嵌入模式。
- `<输入图像路径>`: 原始 BMP 图像的路径。
- `<输出图像路径>`: 生成的含水印 BMP 图像的路径。
- `"<要嵌入的文本>"`: 您要嵌入的文本，包含空格时必须加引号。
- `[强度]`: 可选参数，默认为 `20.0`。数值越高，水印越稳定但可能越明显。

**示例**:

```bash
# 将 "Secret Message" 嵌入到 image.bmp 中，生成 output.bmp
./dct_watermark_tool encode image.bmp output.bmp "Secret Message"

# 指定强度为 30.0
./dct_watermark_tool encode image.bmp output.bmp "Secret Message" 30.0
```

### 2. 提取水印 (`decode`)

从含水印的 BMP 图像中恢复文本。

```bash
./dct_watermark_tool decode <含水印的图像路径>
```

- `decode`: 指定提取模式。
- `<含水印的图像路径>`: 包含水印的 BMP 图像的路径。

**示例**:

```bash
# 从 output.bmp 中提取并打印水印文本
./dct_watermark_tool decode output.bmp
```

## 作为库集成

如果你想将此功能集成到自己的 C 项目中，可以这样做：

1. 将 `dct_watermark.h`, `dct_watermark.c`, `stb_image.h`, `stb_image_write.h` 文件复制到你的项目目录。
2. 在你的源代码中包含头文件：`#include "dct_watermark.h"`。
3. 编译时，将 `dct_watermark.c` 与您的其他源文件一起编译，并链接数学库 `-lm`。**注意**：不要定义 `DEBUG_MODE` 宏，这样就不会编译文件末尾的 `main` 函数，使其作为一个纯库存在。

核心 API 是 `wm_process` 函数，其详细说明可在 `dct_watermark.h` 头文件中找到。

## 状态码

命令行工具的返回值代表操作状态：

- `0`: 成功 (WM_SUCCESS)
- `1`: 文件 I/O 错误 (WM_ERROR_FILE_IO)
- `2`: 文件格式无效或图像太小 (WM_ERROR_INVALID_FORMAT)
- `3`: 内存不足 (WM_ERROR_MEMORY)
- `4`: 图像空间不足以容纳水印 (WM_ERROR_INSUFFICIENT_SPACE)

## 文件格式要求

- 仅支持 **24 位真彩色** (RGB) BMP 文件。
- 图像尺寸必须大于等于 8x8 像素。

## 算法原理

该项目采用**量化索引调制**（Quantization Index Modulation, QIM）的思想。具体来说，它将水印信息（同步标记、文本内容、结束符）全图冗余地嵌入到每个 8x8 像素块中。它选取每 DCT 块中的两个中频系数（如 (3,4) 和 (4,3)），通过调整这两个系数的相对大小关系（哪个更大）来编码一位信息。提取时，程序会进行 8x8 的滑动偏移搜索，从不同起始点扫描图像块，以应对可能的裁剪，直到找到同步标记并恢复出完整的原始文本。

