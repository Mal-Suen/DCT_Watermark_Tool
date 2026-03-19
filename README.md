# DCT Discrete Cosine Transform Watermarking Algorithm README | DCT离散余弦变换水印算法

<div align="center">

**Language / 语言选择:**
[English](#english) | [中文](#中文)

---

</div>
<a id="english"></a>

## Core Concepts: Mapping from Spatial Domain to Frequency Domain

In digital imaging, the **Spatial Domain** refers to the arrangement of pixel intensities on a two-dimensional coordinate system. Conversely, the **DCT (Discrete Cosine Transform)** is an orthogonal linear transform that decomposes the color components of an image block into a combination of cosine waves with different frequencies.

### Physical Significance of Frequency Components

When an $8 \times 8$ pixel matrix undergoes a DCT transform, a 64th-order coefficient matrix is generated.

- **Low Frequency**: Located at the top-left corner of the matrix, representing the base color and average brightness (DC component) of the image.
- **Mid Frequency**: Located in the middle of the matrix, representing structural contours and primary textures.
- **High Frequency**: Located at the bottom-right corner of the matrix, representing edge sharpness, noise, and fine textures.

### Purpose of Energy Conversion

Converting spatial signals into frequency distributions is done to achieve **Energy Concentration**. In most natural images, the vast majority of energy (information) is concentrated in the low-frequency portion. The goal of a watermarking algorithm is to find a frequency band that is **both resistant to compression interference and visually imperceptible**, namely the "Mid-frequency band."

------

## Full Algorithm Execution Process

### Image Partitioning and Color Channel Selection

The algorithm first divides the image into non-overlapping $8 \times 8$ pixel blocks. Typically, the **Y (Luminance) component** of the **YCbCr** color space or the **Blue channel** in the **RGB** space is selected for processing. This is because the Human Visual System (HVS) has lower sensitivity to subtle changes in the blue component.

### Forward DCT Transform

Using the **Separable Transform** property, mathematical operations are performed on each pixel block. At this stage, the color intensities originally distributed across spatial coordinates are transformed into 64 frequency coefficients.

### Encoding and Embedding of the Watermark Signal

This is the logical core of the algorithm. We utilize the **Differential Energy Modification** method, carrying binary information (0 or 1) by adjusting the relative magnitude of two specific mid-frequency coefficients (e.g., $DCT_{4,3}$ and $DCT_{3,4}$):

- **Embedding Logic**:
  - If embedding $Bit = 1$: Apply mathematical compensation to make $DCT_{4,3}$ significantly greater than $DCT_{3,4}$.
  - If embedding $Bit = 0$: Apply mathematical compensation to make $DCT_{3,4}$ significantly greater than $DCT_{4,3}$.
- **Strength Control (Strength)**: A step-size parameter $S$ is introduced. The difference between the modified coefficients must satisfy $|A - B| > S$. A higher $S$ increases the survival rate of the watermark against attacks like JPEG compression and scaling but may cause "blocking artifacts" in the image.

### Inverse DCT Transform

Once the modification is complete, an IDCT operation is performed. This step recombines the frequency coefficients into spatial domain pixel values. Since modifications only occur in the mid-frequency band, the deviation between the restored pixels and the original pixels is minimal (usually between $\pm 1$ to $\pm 3$), achieving visual invisibility.

------

## Robustness and Stability Mechanisms

### Blind Extraction Protocol

This algorithm is a "Blind Watermark," meaning the extraction side does not require the original image for reference. The extractor only needs to perform an $8 \times 8$ block DCT on the target image again and observe the direction of the difference between the predetermined coordinate coefficients. If $DCT_{4,3} > DCT_{3,4}$, it is determined to be 1; otherwise, it is 0.

### Cyclic Redundancy Embedding

To counter **Cropping** attacks, the algorithm embeds the watermark string (Payload) cyclically across all blocks of the entire image. As long as the first extracted byte matches a preset **Sync Marker**, the starting position of the signal can be confirmed. This implies that as long as the image retains sufficient local integrity, the watermark can be recovered.

### Anti-compression Principle

JPEG compression erases high-frequency coefficients via quantization tables to save space. DCT watermarking records the signal in the mid-frequency coefficients that are largely preserved, ensuring that the relative relationship between coefficients remains stable even after severe data discarding (resampling, lossy compression), thus guaranteeing information persistence.

------

## Core Logic Implementation

### Original Algorithm

To deeply understand the **DCT (Discrete Cosine Transform) watermarking algorithm**, we can implement a core demonstration version in C. This algorithm directly implements the mathematical definition of 2D-DCT:

$$F(u, v) = \frac{1}{4} C(u) C(v) \sum_{x=0}^{7} \sum_{y=0}^{7} f(x, y) \cos \left[ \frac{(2x+1)u\pi}{16} \right] \cos \left[ \frac{(2y+1)v\pi}{16} \right]$$

代码段

```
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Original 2D-DCT Transform (Standard four-layer loop implementation)
 * @param f Input 8x8 spatial domain pixel matrix
 * @param F Output 8x8 frequency domain coefficient matrix
 */
void raw_dct_2d(double f[8][8], double F[8][8]) {
    for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
            double sum = 0.0;
            
            // Core: Full double accumulation of spatial coordinates (x, y)
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    sum += f[x][y] * cos((2 * x + 1) * u * M_PI / 16.0) * cos((2 * y + 1) * v * M_PI / 16.0);
                }
            }
            
            // Calculate orthogonal normalization coefficient alpha
            double au = (u == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
            double av = (v == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
            
            F[u][v] = au * av * sum;
        }
    }
}

/**
 * Original 2D-IDCT Inverse Transform (Standard four-layer loop implementation)
 * Used to restore spatial signal f from frequency distribution F
 */
void raw_idct_2d(double F[8][8], double f[8][8]) {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            double sum = 0.0;
            
            for (int u = 0; u < 8; u++) {
                for (int v = 0; v < 8; v++) {
                    double au = (u == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
                    double av = (v == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
                    
                    sum += au * av * F[u][v] * cos((2 * x + 1) * u * M_PI / 16.0) * cos((2 * y + 1) * v * M_PI / 16.0);
                }
            }
            f[x][y] = sum;
        }
    }
}
```

In the original algorithm, to obtain 64 $F(u, v)$ coefficients, each coefficient requires $8 \times 8 = 64$ multiply-accumulate operations. The total computational volume is $64 \times 64 = 4096$ core operations. Its complexity is $O(N^4)$ (where $N=8$). For a $1080p$ image, there are approximately $32,400$ blocks of $8 \times 8$, meaning the total computation nears **130 million floating-point operations**.

------

### Optimization Schemes

#### Lookup Table Method

The calculation of the `cos` function is expensive. In an $8 \times 8$ DCT, there are only 64 fixed combinations of input values for `cos`. Pre-calculating these 64 values and storing them in a static array can increase calculation speed several times over. The optimized code is shown below:

代码段

```
/* Global static lookup table: stores pre-calculated cosine coefficients to eliminate redundant trigonometric calculation overhead */
static double COS_TABLE[8][8];
static int table_initialized = 0;

/**
 * init_tables: Pre-calculates the DCT coefficient table
 * Based on the DCT-II standard formula, pre-calculates cos((2j+1)iπ / 16) and caches it
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
```

#### Separable Transform

A 2D DCT can be decomposed into two 1D DCTs. First, perform a 1D-DCT on each row, then perform a 1D-DCT on each column of the result. By processing rows and columns separately via `dct_1d`, the complexity can be reduced from $O(N^4)$ to $O(N^3)$. The optimized code is shown below:

C

```
/**
 * fdct_1d: 1D Forward Discrete Cosine Transform (Forward DCT)
 * Converts spatial domain signals into frequency domain energy distributions
 */
void fdct_1d(double in[8], double out[8]) {
    int i, j;
    for (i = 0; i < 8; i++) {
        double sum = 0;
        for (j = 0; j < 8; j++) {
            sum += in[j] * COS_TABLE[i][j];
        }
        // Apply orthogonalization coefficient Alpha
        double alpha = (i == 0) ? 0.35355339 : 0.5; // sqrt(1/8) and sqrt(2/8)
        out[i] = alpha * sum;
    }
}

/**
 * idct_1d: 1D Inverse Discrete Cosine Transform (Inverse DCT)
 * Restores frequency domain coefficients to spatial domain pixel values
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
 * fdct_8x8_fast: 2D Fast DCT Implementation
 * Utilizes the Separability of DCT to complete 2D matrix operations via two 1D transforms
 * Complexity reduced from O(N^4) to O(N^3)
 */
void fdct_8x8_fast(double in[8][8], double out[8][8]) {
    double temp[8][8];
    int i, j;
    // Step 1: Row transform
    for (i = 0; i < 8; i++) fdct_1d(in[i], temp[i]);
    // Step 2: Column transform
    for (j = 0; j < 8; j++) {
        double col_in[8], col_out[8];
        for (i = 0; i < 8; i++) col_in[i] = temp[i][j];
        fdct_1d(col_in, col_out);
        for (i = 0; i < 8; i++) out[i][j] = col_out[i];
    }
}
```

#### Strength Balance

The `Strength` value acts as a balance bar. `28.0` is a typical equilibrium point. The higher the value, the stronger the compression resistance, but the image may display "blocking artifacts."

C

```
double strength = 28.0;
```

#### Sync Features

Adding a specific bit pattern (e.g., `0xAA`) at the start of the data stream helps the extractor quickly locate the starting position and filter out images without watermarks.

C

```
#define SYNC_MARKER 0xAA 
```

<a id="中文"></a>

## 核心概念：从空间域到频率域的映射

在数字图像中，**空间域（Spatial Domain）**是指像素在二维坐标系上的亮度排列。而 **DCT（Discrete Cosine Transform）** 是一种正交线性变换，它将图像块的颜色分量分解为一系列具有不同频率的余弦波组合。

### 频率分量的物理意义

当一个 $8 \times 8$ 的像素方阵经过 DCT 变换后，会生成一个 64 阶的系数矩阵。

- **低频（Low Frequency）**：位于矩阵左上角，代表图像的基色和平均亮度（DC 分量）。
- **中频（Mid Frequency）**：位于矩阵中部，代表图像的轮廓特征与主要纹理。
- **高频（High Frequency）**：位于矩阵右下角，代表图像的边缘锐度、噪点和细微纹理。

### 能量转换的目的

将空间信号转换为频率分布，是为了实现**能量集中**。在大多数自然图像中，绝大部分能量（信息量）集中在低频部分，而水印算法的目标是找到一个**既能抵抗压缩干扰、又不引起视觉察觉**的频段，即“中频带”。

## 算法执行全流程

### 图像分块与色彩通道选择

算法首先将图像划分为不重叠的 $8 \times 8$ 像素块。通常选择 **YCbCr** 色彩空间的 **Y（亮度）分量**，或者 **RGB** 空间中的**蓝色通道**进行操作。这是因为人类视觉系统（HVS）对蓝色分量的微小变化敏感度较低。

### 正向 DCT 变换

利用分离变换（Separable Transform）特性，对每个像素块执行数学运算。此时，原本在空间坐标上分布的颜色强度被转化为 64 个频率系数。

### 水印信号的编码与嵌入

这是算法的逻辑核心。我们采用**差分能量修改法**，通过调整两个特定中频位置系数（例如 $DCT_{4,3}$ 和 $DCT_{3,4}$）的相对大小来携带二进制信息（0 或 1）：

- **嵌入逻辑**：
  - 若嵌入 $Bit = 1$：通过数学补偿，使 $DCT_{4,3}$ 显著大于 $DCT_{3,4}$。
  - 若嵌入 $Bit = 0$：通过数学补偿，使 $DCT_{3,4}$ 显著大于 $DCT_{4,3}$。
- **强度控制 (Strength)**：引入一个步长参数 $S$。修改后的系数差异需满足 $|A - B| > S$。$S$ 越大，水印在面对 JPEG 压缩、缩放等攻击时的存活率越高，但会导致图像产生块效应（Blocky artifacts）。

### 逆向 DCT 变换

修改完成后，执行 IDCT 运算。这一步将频率系数重新组合成空间域的像素值。由于修改仅发生在中频段，还原后的像素值与原像素值的偏差极小（通常在 $\pm 1$ 到 $\pm 3$ 之间），从而实现了视觉上的隐形。而实现了视觉上的隐形。

## 鲁棒性与稳定性保障机制

### 盲提取协议

该算法属于“盲水印”，即提取端无需原始图像参考。提取器仅需对目标图像再次进行 $8 \times 8$ 分块 DCT，直接观测预定坐标系数的差值方向。若 $DCT_{4,3} > DCT_{3,4}$，则判定为 1，反之为 0。

### 循环冗余嵌入

为了对抗图像剪裁（Cropping）攻击，算法将水印字符串（Payload）在整张图像的所有分块中进行循环嵌入。只要提取出的第一个字节符合预设的**同步特征码（Sync Marker）**，即可确认信号起始位置。这意味着只要图像保留了足够的局部完整性，水印即可恢复。

### 抗压缩原理

JPEG 压缩通过量化表抹除高频系数以节省空间。DCT 水印将信号刻录在保留较完整的中频系数中，使其在经历剧烈的数据舍弃（重采样、有损压缩）后，系数间的相对比例关系依然能够维持稳定，从而确保了信息的持久性。

## 核心逻辑实现

### 原始算法

为了深入理解 **DCT（离散余弦变换）水印算法**，我们可以通过 C 实现一个核心演示版本。该算法直接实现 2D-DCT 的数学定义式：

$$F(u, v) = \frac{1}{4} C(u) C(v) \sum_{x=0}^{7} \sum_{y=0}^{7} f(x, y) \cos \left[ \frac{(2x+1)u\pi}{16} \right] \cos \left[ \frac{(2y+1)v\pi}{16} \right]$$

```C
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * 原始 2D-DCT 变换（标准四层循环实现）
 * @param f 输入的 8x8 空间域像素矩阵
 * @param F 输出的 8x8 频率域系数矩阵
 */
void raw_dct_2d(double f[8][8], double F[8][8]) {
    for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
            double sum = 0.0;
            
            // 核心：空间域坐标 (x, y) 的全量双重累加
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    sum += f[x][y] * cos((2 * x + 1) * u * M_PI / 16.0) * cos((2 * y + 1) * v * M_PI / 16.0);
                }
            }
            
            // 计算正交归一化系数 alpha
            double au = (u == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
            double av = (v == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
            
            F[u][v] = au * av * sum;
        }
    }
}

/**
 * 原始 2D-IDCT 逆变换（标准四层循环实现）
 * 用于从频率分布 F 还原回空间信号 f
 */
void raw_idct_2d(double F[8][8], double f[8][8]) {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            double sum = 0.0;
            
            for (int u = 0; u < 8; u++) {
                for (int v = 0; v < 8; v++) {
                    double au = (u == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
                    double av = (v == 0) ? sqrt(1.0/8.0) : sqrt(2.0/8.0);
                    
                    sum += au * av * F[u][v] * cos((2 * x + 1) * u * M_PI / 16.0) * cos((2 * y + 1) * v * M_PI / 16.0);
                }
            }
            f[x][y] = sum;
        }
    }
}
```

在原始算法中，为了得到 64 个 $F(u, v)$ 系数，每一个系数都需要进行 $8 \times 8 = 64$ 次乘法累加。总计算量为 $64 \times 64 = 4096$ 次核心运算。其复杂度为 $O(N^4)$（其中 $N=8$）。对于一张 $1080p$ 的图片，大约有 $32,400$ 个 $8 \times 8$ 块，这意味着总计算量接近 **1.3 亿次浮点运算**。

### 优化方案

经过优化后的完整代码参见：[DCT_Watermark](https://github.com/Mal-Suen/DCT_Watermark)

#### 查表法

`cos` 函数的计算非常昂贵。而在 $8 \times 8$ 的 DCT 中，`cos` 的输入值只有 64 种固定的组合。预先计算好这 64 个值，存入一个静态数组，可是计算速度增加数倍。优化后的代码如下所示：

```C
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
```

#### 可分离变换

二维 DCT 可以分解为两次一维 DCT。先对每一行做 1D-DCT，再对结果的每一列做 1D-DCT。通过 `dct_1d` 分两次处理行和列，可以将复杂度从 $O(N^4)$ 降至 $O(N^3)$ 。优化后的代码如下所示：

```c
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
```

#### 强度平衡

强度值（Strength）是一个平衡杆。`28.0` 是一个平衡点。数值越大，抗压缩越强，但图片可能出现色块（块效应）。

```c
double strength = 28.0;
```

#### 同步特征

在数据流开头加入特定的位模式（如 `0xAA`），可以帮助提取器快速定位起始位置并过滤无水印图片。

```c
#define SYNC_MARKER 0xAA 
```

