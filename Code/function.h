#ifndef __FUSION_FUNCTION_H__
#define __FUSION_FUNCTION_H__

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

// ============================================================================
// 一、基础数据类型定义
// ============================================================================

/**
 * @brief 三维向量（用于加速度、角速度、磁场等物理量）
 * 
 * 使用方式:
 *   FusionVector v = { .axis = { .x=1.0f, .y=2.0f, .z=3.0f } };
 *   或 v.array[0], v.array[1], v.array[2] 按分量访问
 */
typedef union {
    float array[3];          ///< 按数组访问
    struct {
        float x;             ///< X 轴分量
        float y;             ///< Y 轴分量
        float z;             ///< Z 轴分量
    } axis;                  ///< 按名称访问
} FusionVector;

/**
 * @brief 四元数（用于表示旋转/姿态）
 * 
 * 四元数格式: w + xi + yj + zk
 * 四个分量满足归一化约束: w² + x² + y² + z² = 1
 * 单位四元数表示没有旋转（w=1, x=y=z=0）
 */
typedef union {
    float array[4];          ///< 按数组 [w, x, y, z] 访问
    struct {
        float w;             ///< 实部（标量部分）
        float x;             ///< 虚部 i 分量
        float y;             ///< 虚部 j 分量
        float z;             ///< 虚部 k 分量
    } element;
} FusionQuaternion;

/**
 * @brief 3x3 矩阵 按行主序存储
 * 
 * 用于传感器标定（失准角矩阵、软铁矩阵等）
 */
typedef union {
    float array[3][3];       ///< 按二维数组访问
    struct {
        float xx, xy, xz;    ///< 第一行
        float yx, yy, yz;    ///< 第二行
        float zx, zy, zz;    ///< 第三行
    } element;
} FusionMatrix;

/**
 * @brief 欧拉角 单位: 度
 * 
 * Roll  = 绕 X 轴旋转（翻滚角）
 * Pitch = 绕 Y 轴旋转（俯仰角）
 * Yaw   = 绕 Z 轴旋转（偏航角）
 * 旋转顺序: Z-Y-X (先偏航、再俯仰、最后翻滚)
 */
typedef union {
    float array[3];
    struct {
        float roll;          ///< 翻滚角 -180° ~ 180°
        float pitch;         ///< 俯仰角 -90°  ~ 90°
        float yaw;           ///< 偏航角 -180° ~ 180°
    } angle;
} FusionEuler;

// ============================================================================
// 二、常量宏
// ============================================================================

/** 零向量 {0, 0, 0} */
#define FUSION_VECTOR_ZERO      ((FusionVector){ .array = {0.0f, 0.0f, 0.0f} })

/** 全1向量 {1, 1, 1} */
#define FUSION_VECTOR_ONES      ((FusionVector){ .array = {1.0f, 1.0f, 1.0f} })

/** 单位四元数 {w=1, x=0, y=0, z=0} 表示无旋转 */
#define FUSION_IDENTITY_QUATERNION  ((FusionQuaternion){ .array = {1.0f, 0.0f, 0.0f, 0.0f} })

/** 单位矩阵 3x3 对角线全1 */
#define FUSION_IDENTITY_MATRIX  ((FusionMatrix){ .array = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} })

/** 零欧拉角 {roll=0, pitch=0, yaw=0} */
#define FUSION_EULER_ZERO       ((FusionEuler){ .array = {0.0f, 0.0f, 0.0f} })

#ifndef M_PI
#define M_PI  (3.14159265358979323846f)
#endif

// ============================================================================
// 三、坐标系约定
// ============================================================================

/**
 * @brief 地球坐标系约定
 * 
 * NWU = North-West-Up   (北-西-天) 最常用, X=北, Y=西, Z=上
 * ENU = East-North-Up   (东-北-天)
 * NED = North-East-Down (北-东-地) 常见于飞控
 */
typedef enum {
    FusionConventionNwu,   ///< 北-西-天
    FusionConventionEnu,   ///< 东-北-天
    FusionConventionNed,   ///< 北-东-地
} FusionConvention;

// ============================================================================
// 四、传感器轴对齐枚举（24 种排列）
// ============================================================================

/**
 * @brief 传感器坐标轴与机体坐标轴的对齐方式
 * 
 * 命名规则: ±轴1 ±轴2 ±轴3
 * 例如 +Y-X+Z 表示:
 *   机体 X = 传感器 +Y
 *   机体 Y = 传感器 -X
 *   机体 Z = 传感器 +Z
 * 
 * 共 24 种排列（3轴 × 2方向 × 4种下一轴选择）
 */
typedef enum {
    FusionAxesAlignmentPXPYPZ,  ///< +X +Y +Z  (不移位)
    FusionAxesAlignmentPXNZPY,  ///< +X -Z +Y
    FusionAxesAlignmentPXNYNZ,  ///< +X -Y -Z
    FusionAxesAlignmentPXPZNY,  ///< +X +Z -Y
    FusionAxesAlignmentNXPYNZ,  ///< -X +Y -Z
    FusionAxesAlignmentNXPZPY,  ///< -X +Z +Y
    FusionAxesAlignmentNXNYPZ,  ///< -X -Y +Z
    FusionAxesAlignmentNXNZNY,  ///< -X -Z -Y
    FusionAxesAlignmentPYNXPZ,  ///< +Y -X +Z
    FusionAxesAlignmentPYNZNX,  ///< +Y -Z -X
    FusionAxesAlignmentPYPXNZ,  ///< +Y +X -Z
    FusionAxesAlignmentPYPZPX,  ///< +Y +Z +X
    FusionAxesAlignmentNYPXPZ,  ///< -Y +X +Z
    FusionAxesAlignmentNYNZPX,  ///< -Y -Z +X
    FusionAxesAlignmentNYNXNZ,  ///< -Y -X -Z
    FusionAxesAlignmentNYPZNX,  ///< -Y +Z -X
    FusionAxesAlignmentPZPYNX,  ///< +Z +Y -X
    FusionAxesAlignmentPZPXPY,  ///< +Z +X +Y
    FusionAxesAlignmentPZNYPX,  ///< +Z -Y +X
    FusionAxesAlignmentPZNXNY,  ///< +Z -X -Y
    FusionAxesAlignmentNZPYPX,  ///< -Z +Y +X
    FusionAxesAlignmentNZNXPY,  ///< -Z -X +Y
    FusionAxesAlignmentNZNYNX,  ///< -Z -Y -X
    FusionAxesAlignmentNZPXNY,  ///< -Z +X -Y
} FusionAxesAlignment;

// ============================================================================
// 五、AHRS 姿态解算器 数据结构
// ============================================================================

/**
 * @brief AHRS 算法参数设置
 * 
 * 用户可配置的融合算法参数:
 * - gain: 互补滤波器增益, 默认 0.5 (越大加速度计修正越快, 但噪声也越大)
 * - gyroscopeRange: 陀螺仪量程 超过此值会触发恢复机制
 * - accelerationRejection: 加速度异常拒斥角(度) 用于抗机动干扰
 * - magneticRejection: 磁场异常拒斥角(度) 用于抗磁干扰
 */
typedef struct {
    FusionConvention convention;         ///< 坐标系约定 (NWU/ENU/NED)
    float gain;                          ///< AHRS 增益 默认 0.5
    float gyroscopeRange;                ///< 陀螺仪量程 dps 0=不限
    float accelerationRejection;         ///< 加速度拒斥阈值 度 默认90
    float magneticRejection;             ///< 磁场拒斥阈值 度 默认90
    unsigned int recoveryTriggerPeriod;  ///< 异常恢复触发周期
} FusionAhrsSettings;

/**
 * @brief AHRS 算法内部状态
 * 
 * 内部变量, 用户不应直接修改
 */
typedef struct {
    FusionAhrsSettings settings;          ///< 算法参数
    FusionQuaternion quaternion;          ///< 当前姿态四元数
    FusionVector accelerometer;           ///< 上一次加速度计值
    bool initialising;                    ///< 正在初始化
    float rampedGain;                     ///< 斜坡增益 (初始化时从高到低渐变)
    float rampedGainStep;                 ///< 增益递减步长
    bool angularRateRecovery;             ///< 角速度恢复标志
    FusionVector halfAccelerometerFeedback; ///< 加速度计反馈量(×0.5)
    FusionVector halfMagnetometerFeedback;  ///< 磁力计反馈量(×0.5)
    bool accelerometerIgnored;            ///< 当前是否忽略加速度计
    int accelerationRecoveryTrigger;      ///< 加速度恢复触发计数
    int accelerationRecoveryTimeout;      ///< 加速度恢复超时
    bool magnetometerIgnored;             ///< 当前是否忽略磁力计
    int magneticRecoveryTrigger;          ///< 磁场恢复触发计数
    int magneticRecoveryTimeout;          ///< 磁场恢复超时
} FusionAhrs;

/**
 * @brief AHRS 内部诊断信息（错误量、忽略标志）
 */
typedef struct {
    float accelerationError;              ///< 加速度误差 度
    bool accelerometerIgnored;            ///< 加速度计是否被忽略
    float accelerationRecoveryTrigger;    ///< 加速度恢复触发比例 0~1
    float magneticError;                  ///< 磁场误差 度
    bool magnetometerIgnored;             ///< 磁场是否被忽略
    float magneticRecoveryTrigger;        ///< 磁场恢复触发比例 0~1
} FusionAhrsInternalStates;

/**
 * @brief AHRS 运行状态标志
 */
typedef struct {
    bool initialising;              ///< 是否处于初始化阶段
    bool angularRateRecovery;       ///< 是否处于角速度恢复
    bool accelerationRecovery;      ///< 是否处于加速度恢复
    bool magneticRecovery;          ///< 是否处于磁场恢复
} FusionAhrsFlags;

// ============================================================================
// 六、陀螺仪零偏在线校准 数据结构
// ============================================================================

/**
 * @brief 陀螺仪零偏在线校准器
 * 
 * 检测陀螺仪静止状态, 自动追踪零偏漂移（温漂）
 * 原理: 角速度小于3dps持续5秒 → 认为是静止 → 低通滤波更新零偏
 */
typedef struct {
    float filterCoefficient;       ///< 低通滤波系数 (截止频率 0.02Hz)
    unsigned int timeout;          ///< 静止超时周期数
    unsigned int timer;            ///< 当前静止计时器
    FusionVector gyroscopeOffset;  ///< 当前估计的零偏值
} FusionOffset;

// ============================================================================
// 七、数学基础函数（角度转换）
// ============================================================================

/**
 * @brief 度转弧度
 * @param degrees 角度值(度)
 * @return 弧度值
 * @note 角度控制常用度, 数学计算常用弧度, 此函数完成转换
 */
static inline float FusionDegreesToRadians(const float degrees) {
    return degrees * (M_PI / 180.0f);
}

/**
 * @brief 弧度转度
 * @param radians 弧度值
 * @return 度数
 */
static inline float FusionRadiansToDegrees(const float radians) {
    return radians * (180.0f / M_PI);
}

/**
 * @brief 安全反正弦 (防越界)
 * @param value 输入值
 * @return 反正弦结果 范围 [-π/2, π/2]
 */
static inline float FusionAsin(const float value) {
    if (value <= -1.0f) return M_PI / -2.0f;
    if (value >= 1.0f)  return M_PI / 2.0f;
    return asinf(value);
}

/**
 * @brief 快速平方根倒数算法 (Quake III 经典算法)
 * 
 * 用于归一化向量的快速近似计算
 * 如果定义了 FUSION_USE_NORMAL_SQRT 则使用标准 sqrt
 */
#ifndef FUSION_USE_NORMAL_SQRT
static inline float FusionFastInverseSqrt(const float x) {
    typedef union { float f; int32_t i; } Union32;
    Union32 u = {.f = x};
    u.i = 0x5F1F1412 - (u.i >> 1);
    return u.f * (1.69000231f - 0.714158168f * x * u.f * u.f);
}
#endif

// ============================================================================
// 八、向量运算（Vector Operations）
// ============================================================================

/** @brief 判断向量是否为零向量 */
static inline bool FusionVectorIsZero(const FusionVector vector) {
    return (vector.axis.x == 0.0f) && (vector.axis.y == 0.0f) && (vector.axis.z == 0.0f);
}

/** @brief 向量加法 C = A + B */
static inline FusionVector FusionVectorAdd(const FusionVector a, const FusionVector b) {
    return (FusionVector){ .axis = { .x = a.axis.x + b.axis.x,
                                     .y = a.axis.y + b.axis.y,
                                     .z = a.axis.z + b.axis.z }};
}

/** @brief 向量减法 C = A - B */
static inline FusionVector FusionVectorSubtract(const FusionVector a, const FusionVector b) {
    return (FusionVector){ .axis = { .x = a.axis.x - b.axis.x,
                                     .y = a.axis.y - b.axis.y,
                                     .z = a.axis.z - b.axis.z }};
}

/** @brief 向量元素求和 x+y+z */
static inline float FusionVectorSum(const FusionVector vector) {
    return vector.axis.x + vector.axis.y + vector.axis.z;
}

/** @brief 向量数乘 result = v × scalar */
static inline FusionVector FusionVectorMultiplyScalar(const FusionVector v, const float s) {
    return (FusionVector){ .axis = { .x = v.axis.x * s,
                                     .y = v.axis.y * s,
                                     .z = v.axis.z * s }};
}

/**
 * @brief 哈达玛积 逐元素相乘
 * 
 * result = { ax×bx, ay×by, az×bz }
 * 用于标定中的灵敏度修正
 */
static inline FusionVector FusionVectorHadamardProduct(const FusionVector a, const FusionVector b) {
    return (FusionVector){ .axis = { .x = a.axis.x * b.axis.x,
                                     .y = a.axis.y * b.axis.y,
                                     .z = a.axis.z * b.axis.z }};
}

/**
 * @brief 向量叉积 C = A × B
 * 
 * 结果向量垂直于 A 和 B 所在的平面
 * 用途: 计算修正方向、构造坐标系
 */
static inline FusionVector FusionVectorCrossProduct(const FusionVector a, const FusionVector b) {
    return (FusionVector){ .axis = { .x = a.axis.y * b.axis.z - a.axis.z * b.axis.y,
                                     .y = a.axis.z * b.axis.x - a.axis.x * b.axis.z,
                                     .z = a.axis.x * b.axis.y - a.axis.y * b.axis.x }};
}

/**
 * @brief 向量点积 A·B = ax×bx + ay×by + az×bz
 * 
 * 结果反映两向量夹角: >0同向, =0垂直, <0反向
 */
static inline float FusionVectorDotProduct(const FusionVector a, const FusionVector b) {
    return FusionVectorSum(FusionVectorHadamardProduct(a, b));
}

/** @brief 向量模长的平方 |v|² */
static inline float FusionVectorMagnitudeSquared(const FusionVector v) {
    return FusionVectorSum(FusionVectorHadamardProduct(v, v));
}

/** @brief 向量模长 |v| */
static inline float FusionVectorMagnitude(const FusionVector v) {
    return sqrtf(FusionVectorMagnitudeSquared(v));
}

/**
 * @brief 向量归一化 方向不变, 长度变为1
 * 
 * 用途: 仅保留方向信息, 用于角度/姿态计算
 */
static inline FusionVector FusionVectorNormalise(const FusionVector v) {
#ifdef FUSION_USE_NORMAL_SQRT
    const float inv = 1.0f / sqrtf(FusionVectorMagnitudeSquared(v));
#else
    const float inv = FusionFastInverseSqrt(FusionVectorMagnitudeSquared(v));
#endif
    return FusionVectorMultiplyScalar(v, inv);
}

// ============================================================================
// 九、四元数运算（Quaternion Operations）
// ============================================================================

/** @brief 四元数加法 C = A + B */
static inline FusionQuaternion FusionQuaternionAdd(const FusionQuaternion a, const FusionQuaternion b) {
    return (FusionQuaternion){ .element = { .w = a.element.w + b.element.w,
                                            .x = a.element.x + b.element.x,
                                            .y = a.element.y + b.element.y,
                                            .z = a.element.z + b.element.z }};
}

/**
 * @brief 四元数乘法 C = A × B
 * 
 * 四元数乘法表示旋转的组合: 先做B旋转, 再做A旋转
 * 注意: 四元数乘法不满足交换律
 */
static inline FusionQuaternion FusionQuaternionMultiply(const FusionQuaternion a, const FusionQuaternion b) {
    return (FusionQuaternion){ .element = {
        .w = a.element.w * b.element.w - a.element.x * b.element.x - a.element.y * b.element.y - a.element.z * b.element.z,
        .x = a.element.w * b.element.x + a.element.x * b.element.w + a.element.y * b.element.z - a.element.z * b.element.y,
        .y = a.element.w * b.element.y - a.element.x * b.element.z + a.element.y * b.element.w + a.element.z * b.element.x,
        .z = a.element.w * b.element.z + a.element.x * b.element.y - a.element.y * b.element.x + a.element.z * b.element.w
    }};
}

/**
 * @brief 四元数与向量相乘 将向量视为 w=0 的四元数
 * 
 * 用途: 用四元数旋转一个向量
 */
static inline FusionQuaternion FusionQuaternionMultiplyVector(const FusionQuaternion q, const FusionVector v) {
    return (FusionQuaternion){ .element = {
        .w = -q.element.x * v.axis.x - q.element.y * v.axis.y - q.element.z * v.axis.z,
        .x =  q.element.w * v.axis.x + q.element.y * v.axis.z - q.element.z * v.axis.y,
        .y =  q.element.w * v.axis.y - q.element.x * v.axis.z + q.element.z * v.axis.x,
        .z =  q.element.w * v.axis.z + q.element.x * v.axis.y - q.element.y * v.axis.x
    }};
}

/**
 * @brief 四元数归一化 使模长=1
 * 
 * 四元数经多次运算后模长会偏离1, 需要定期归一化
 */
static inline FusionQuaternion FusionQuaternionNormalise(const FusionQuaternion q) {
    const float a = q.element.w, b = q.element.x, c = q.element.y, d = q.element.z;
#ifdef FUSION_USE_NORMAL_SQRT
    const float inv = 1.0f / sqrtf(a*a + b*b + c*c + d*d);
#else
    const float inv = FusionFastInverseSqrt(a*a + b*b + c*c + d*d);
#endif
    return (FusionQuaternion){ .element = {
        .w = a * inv, .x = b * inv, .y = c * inv, .z = d * inv }};
}

// ============================================================================
// 十、矩阵运算
// ============================================================================

/**
 * @brief 矩阵与向量相乘 result = M × v
 * 
 * 用于传感器标定: 失准角矩阵 × 原始数据 = 修正后数据
 */
static inline FusionVector FusionMatrixMultiplyVector(const FusionMatrix m, const FusionVector v) {
    return (FusionVector){ .axis = {
        .x = m.element.xx * v.axis.x + m.element.xy * v.axis.y + m.element.xz * v.axis.z,
        .y = m.element.yx * v.axis.x + m.element.yy * v.axis.y + m.element.yz * v.axis.z,
        .z = m.element.zx * v.axis.x + m.element.zy * v.axis.y + m.element.zz * v.axis.z
    }};
}

// ============================================================================
// 十一、四元数与欧拉角转换
// ============================================================================

/**
 * @brief 四元数 → 旋转矩阵
 * @param quaternion 单位四元数
 * @return 3x3 旋转矩阵
 */
static inline FusionMatrix FusionQuaternionToMatrix(const FusionQuaternion q) {
    float qwqw = q.element.w * q.element.w;
    float qwqx = q.element.w * q.element.x;
    float qwqy = q.element.w * q.element.y;
    float qwqz = q.element.w * q.element.z;
    float qxqy = q.element.x * q.element.y;
    float qxqz = q.element.x * q.element.z;
    float qyqz = q.element.y * q.element.z;
    return (FusionMatrix){ .element = {
        .xx = 2.0f * (qwqw - 0.5f + q.element.x * q.element.x),
        .xy = 2.0f * (qxqy - qwqz),
        .xz = 2.0f * (qxqz + qwqy),
        .yx = 2.0f * (qxqy + qwqz),
        .yy = 2.0f * (qwqw - 0.5f + q.element.y * q.element.y),
        .yz = 2.0f * (qyqz - qwqx),
        .zx = 2.0f * (qxqz - qwqy),
        .zy = 2.0f * (qyqz + qwqx),
        .zz = 2.0f * (qwqw - 0.5f + q.element.z * q.element.z),
    }};
}

/**
 * @brief 四元数 → ZYX 欧拉角（度）
 * 
 * 旋转顺序: 先绕 Z(Yaw), 再绕 Y(Pitch), 最后绕 X(Roll)
 * 这是最常用的欧拉角转换方式
 * @param quaternion 四元数
 * @return 欧拉角 roll, pitch, yaw 单位度
 */
static inline FusionEuler FusionQuaternionToEuler(const FusionQuaternion q) {
    const float qw = q.element.w, qx = q.element.x;
    const float qy = q.element.y, qz = q.element.z;
    const float halfMinusQySq = 0.5f - qy * qy;
    return (FusionEuler){ .angle = {
        .roll  = FusionRadiansToDegrees(atan2f(qw * qx + qy * qz, halfMinusQySq - qx * qx)),
        .pitch = FusionRadiansToDegrees(FusionAsin(2.0f * (qw * qy - qz * qx))),
        .yaw   = FusionRadiansToDegrees(atan2f(qw * qz + qx * qy, halfMinusQySq - qz * qz)),
    }};
}

// ============================================================================
// 十二、传感器轴对齐
// ============================================================================

/**
 * @brief 传感器坐标轴交换 将传感器轴映射到机体轴
 * @param sensor 原始传感器数据
 * @param alignment 对齐方式 (见 FusionAxesAlignment 枚举)
 * @return 对齐后的数据
 * 
 * 用途: 芯片安装方向与机体方向不一致时, 通过轴对齐统一坐标系
 * 例: 芯片竖着放 → 用 +Y-X+Z 将传感器Y轴映射到机体X轴
 */
static inline FusionVector FusionAxesSwap(const FusionVector s, const FusionAxesAlignment align) {
    FusionVector r;
    switch (align) {
        case FusionAxesAlignmentPXPYPZ:  return s;                                    // +X +Y +Z
        case FusionAxesAlignmentPXNZPY:  r = (FusionVector){.axis={+s.axis.x,-s.axis.z,+s.axis.y}}; return r;  // +X -Z +Y
        case FusionAxesAlignmentPXNYNZ:  r = (FusionVector){.axis={+s.axis.x,-s.axis.y,-s.axis.z}}; return r;  // +X -Y -Z
        case FusionAxesAlignmentPXPZNY:  r = (FusionVector){.axis={+s.axis.x,+s.axis.z,-s.axis.y}}; return r;  // +X +Z -Y
        case FusionAxesAlignmentNXPYNZ:  r = (FusionVector){.axis={-s.axis.x,+s.axis.y,-s.axis.z}}; return r;  // -X +Y -Z
        case FusionAxesAlignmentNXPZPY:  r = (FusionVector){.axis={-s.axis.x,+s.axis.z,+s.axis.y}}; return r;  // -X +Z +Y
        case FusionAxesAlignmentNXNYPZ:  r = (FusionVector){.axis={-s.axis.x,-s.axis.y,+s.axis.z}}; return r;  // -X -Y +Z
        case FusionAxesAlignmentNXNZNY:  r = (FusionVector){.axis={-s.axis.x,-s.axis.z,-s.axis.y}}; return r;  // -X -Z -Y
        case FusionAxesAlignmentPYNXPZ:  r = (FusionVector){.axis={+s.axis.y,-s.axis.x,+s.axis.z}}; return r;  // +Y -X +Z
        case FusionAxesAlignmentPYNZNX:  r = (FusionVector){.axis={+s.axis.y,-s.axis.z,-s.axis.x}}; return r;  // +Y -Z -X
        case FusionAxesAlignmentPYPXNZ:  r = (FusionVector){.axis={+s.axis.y,+s.axis.x,-s.axis.z}}; return r;  // +Y +X -Z
        case FusionAxesAlignmentPYPZPX:  r = (FusionVector){.axis={+s.axis.y,+s.axis.z,+s.axis.x}}; return r;  // +Y +Z +X
        case FusionAxesAlignmentNYPXPZ:  r = (FusionVector){.axis={-s.axis.y,+s.axis.x,+s.axis.z}}; return r;  // -Y +X +Z
        case FusionAxesAlignmentNYNZPX:  r = (FusionVector){.axis={-s.axis.y,-s.axis.z,+s.axis.x}}; return r;  // -Y -Z +X
        case FusionAxesAlignmentNYNXNZ:  r = (FusionVector){.axis={-s.axis.y,-s.axis.x,-s.axis.z}}; return r;  // -Y -X -Z
        case FusionAxesAlignmentNYPZNX:  r = (FusionVector){.axis={-s.axis.y,+s.axis.z,-s.axis.x}}; return r;  // -Y +Z -X
        case FusionAxesAlignmentPZPYNX:  r = (FusionVector){.axis={+s.axis.z,+s.axis.y,-s.axis.x}}; return r;  // +Z +Y -X
        case FusionAxesAlignmentPZPXPY:  r = (FusionVector){.axis={+s.axis.z,+s.axis.x,+s.axis.y}}; return r;  // +Z +X +Y
        case FusionAxesAlignmentPZNYPX:  r = (FusionVector){.axis={+s.axis.z,-s.axis.y,+s.axis.x}}; return r;  // +Z -Y +X
        case FusionAxesAlignmentPZNXNY:  r = (FusionVector){.axis={+s.axis.z,-s.axis.x,-s.axis.y}}; return r;  // +Z -X -Y
        case FusionAxesAlignmentNZPYPX:  r = (FusionVector){.axis={-s.axis.z,+s.axis.y,+s.axis.x}}; return r;  // -Z +Y +X
        case FusionAxesAlignmentNZNXPY:  r = (FusionVector){.axis={-s.axis.z,-s.axis.x,+s.axis.y}}; return r;  // -Z -X +Y
        case FusionAxesAlignmentNZNYNX:  r = (FusionVector){.axis={-s.axis.z,-s.axis.y,-s.axis.x}}; return r;  // -Z -Y -X
        case FusionAxesAlignmentNZPXNY:  r = (FusionVector){.axis={-s.axis.z,+s.axis.x,-s.axis.y}}; return r;  // -Z +X -Y
    }
    return s;
}

// ============================================================================
// 十三、传感器标定模型
// ============================================================================

/**
 * @brief 陀螺仪/加速度计标定模型
 * 
 * 修正公式: calibrated = Misalignment × ((raw - offset) × sensitivity)
 * 
 * @param uncalibrated  未标定的原始数据
 * @param misalignment  3x3 失准角矩阵（传感器三轴不正交的修正）
 * @param sensitivity   灵敏度修正 (1.0=完美, >1=欠灵敏)
 * @param offset        零偏修正
 * @return 标定后的数据
 * 
 * 三个修正分步说明:
 *   raw - offset:           剔除静态零偏
 *   × sensitivity:          修正各轴灵敏度不一致
 *   × misalignment:         修正三轴不正交（安装角度偏差）
 */
static inline FusionVector FusionCalibrationInertial(
    const FusionVector uncalibrated,
    const FusionMatrix misalignment,
    const FusionVector sensitivity,
    const FusionVector offset)
{
    return FusionMatrixMultiplyVector(
        misalignment,
        FusionVectorHadamardProduct(
            FusionVectorSubtract(uncalibrated, offset),
            sensitivity));
}

/**
 * @brief 磁力计标定模型
 * 
 * 修正公式: calibrated = SoftIron × (raw - HardIron)
 * 
 * HardIron(硬铁): 传感器周围永磁体产生的固定偏移
 * SoftIron(软铁): 传感器周围铁磁材料扭曲磁场
 */
static inline FusionVector FusionCalibrationMagnetic(
    const FusionVector uncalibrated,
    const FusionMatrix softIronMatrix,
    const FusionVector hardIronOffset)
{
    return FusionMatrixMultiplyVector(
        softIronMatrix,
        FusionVectorSubtract(uncalibrated, hardIronOffset));
}

// ============================================================================
// 十四、AHRS 姿态解算 函数声明
// ============================================================================

/**
 * @brief 初始化 AHRS 解算器
 * @param ahrs AHRS 结构体指针
 * @note 调用后设为 NWU 坐标系, gain=0.5, 进入3秒初始化斜坡期
 */
void FusionAhrsInitialise(FusionAhrs *const ahrs);

/**
 * @brief 重置 AHRS 保持当前参数, 重新开始融合
 * @param ahrs AHRS 结构体指针
 * @note 姿态四元数重置为单位四元数, 初始化标志置true
 */
void FusionAhrsReset(FusionAhrs *const ahrs);

/**
 * @brief 设置 AHRS 算法参数
 * @param ahrs AHRS 结构体指针
 * @param settings 参数结构体指针
 */
void FusionAhrsSetSettings(FusionAhrs *const ahrs, const FusionAhrsSettings *const settings);

/**
 * @brief 更新 AHRS 使用陀螺仪+加速度计+磁力计 三传感器融合
 * @param ahrs          AHRS 结构体指针
 * @param gyroscope      陀螺仪数据 单位: 度/秒
 * @param accelerometer  加速度计数据 单位: g
 * @param magnetometer   磁力计数据 任意校准后的单位
 * @param deltaTime      采样间隔 单位: 秒 (如52Hz = 1/52 ≈ 0.01923)
 */
void FusionAhrsUpdate(FusionAhrs *const ahrs,
    const FusionVector gyroscope, const FusionVector accelerometer,
    const FusionVector magnetometer, const float deltaTime);

/**
 * @brief 更新 AHRS 仅用陀螺仪+加速度计 (无磁力计)
 * 
 * 核心融合逻辑:
 *   1. 陀螺仪积分 → 预测姿态
 *   2. 加速度计测量重力方向 → 修正 roll/pitch
 *   3. 加速度异常检测与拒斥 (如车辆急加速时暂时忽略加速度计)
 *   4. 初始化阶段 yaw 强制置零 (因为没有磁力计, 偏航会漂移)
 * 
 * @param ahrs          AHRS 结构体指针
 * @param gyroscope      陀螺仪数据 单位: 度/秒
 * @param accelerometer  加速度计数据 单位: g
 * @param deltaTime      采样间隔 单位: 秒
 */
void FusionAhrsUpdateNoMagnetometer(FusionAhrs *const ahrs,
    const FusionVector gyroscope, const FusionVector accelerometer,
    const float deltaTime);

/**
 * @brief 用外部航向源更新 AHRS (如GPS航向)
 * @param ahrs          AHRS 结构体指针
 * @param gyroscope      陀螺仪数据
 * @param accelerometer  加速度计数据
 * @param heading        外部航向 单位: 度
 * @param deltaTime      采样间隔
 */
void FusionAhrsUpdateExternalHeading(FusionAhrs *const ahrs,
    const FusionVector gyroscope, const FusionVector accelerometer,
    const float heading, const float deltaTime);

/** @brief 获取当前姿态四元数 */
FusionQuaternion FusionAhrsGetQuaternion(const FusionAhrs *const ahrs);

/** @brief 设置姿态四元数 (复位姿态) */
void FusionAhrsSetQuaternion(FusionAhrs *const ahrs, FusionQuaternion quaternion);

/**
 * @brief 获取重力方向向量 在传感器坐标系中
 * @return 重力方向向量 (z分量 ≈ 1.0 表示水平)
 */
FusionVector FusionAhrsGetGravity(const FusionAhrs *const ahrs);

/**
 * @brief 获取线性加速度 = 加速度计 - 重力
 * @return 除去重力后的真实运动加速度
 */
FusionVector FusionAhrsGetLinearAcceleration(const FusionAhrs *const ahrs);

/**
 * @brief 获取地球坐标系中的加速度
 * @return 转换到地球坐标系并除去重力后的加速度
 */
FusionVector FusionAhrsGetEarthAcceleration(const FusionAhrs *const ahrs);

/** @brief 获取 AHRS 内部诊断状态 */
FusionAhrsInternalStates FusionAhrsGetInternalStates(const FusionAhrs *const ahrs);

/** @brief 获取 AHRS 运行标志 */
FusionAhrsFlags FusionAhrsGetFlags(const FusionAhrs *const ahrs);

/**
 * @brief 设置偏航角 用于重置漂移
 * @param ahrs    AHRS 结构体
 * @param heading 偏航角 单位: 度
 * @note 无磁力计时, yaw会随时间漂移; 此函数可手动归零
 */
void FusionAhrsSetHeading(FusionAhrs *const ahrs, const float heading);

// ============================================================================
// 十五、陀螺仪零偏在线校准 函数声明
// ============================================================================

/**
 * @brief 初始化零偏校准器
 * @param offset     校准器结构体指针
 * @param sampleRate 采样频率 Hz (如52)
 */
void FusionOffsetInitialise(FusionOffset *const offset, unsigned int sampleRate);

/**
 * @brief 更新零偏校准 返回修正后的陀螺仪数据
 * 
 * 工作流程:
 *   raw_gyro = raw_gyro - current_offset      // 先减去零偏
 *   检测 |raw_gyro| < 3dps?                    // 是否静止
 *   是 → timer++ → timer>5秒 → 低通更新 offset
 *   否 → timer=0 (运动时不更新)
 * 
 * @param offset     校准器结构体指针
 * @param gyroscope  原始陀螺仪数据 单位: dps
 * @return           修正后的陀螺仪数据
 */
FusionVector FusionOffsetUpdate(FusionOffset *const offset, FusionVector gyroscope);

// ============================================================================
// 十六、电子罗盘 函数声明
// ============================================================================

/**
 * @brief 倾角补偿磁航向
 * 
 * 原理: 加速度计测重力 → 补偿倾斜 → 磁力计算航向
 * 输入: 加速度计(g单位) + 磁力计(任意校准单位)
 * 输出: 航向角(度, 0=北, 90=东, 180=南, 270=西)
 */
float FusionCompassCalculateHeading(FusionConvention convention,
    FusionVector accelerometer, FusionVector magnetometer);

#endif /* __FUSION_FUNCTION_H__ */
