# GKI 内核 ABI 符号对齐技术文档

## 概述

本文档详细解释如何通过修补 Google 官方 GKI (Generic Kernel Image) 内核的 ABI 符号白名单，使编译出的内核模块(.ko)能够在所有符合 GKI 标准的设备上通用加载。

---

## 核心原理

### 1. 什么是 GKI ABI？

GKI (Generic Kernel Image) 是 Google 从 Android 11 开始推行的通用内核镜像方案：

```
┌─────────────────────────────────────────────────────┐
│                    Android 设备                      │
├─────────────────────────────────────────────────────┤
│  Vendor Modules (.ko)  │  GKI Modules (.ko)         │
├─────────────────────────────────────────────────────┤
│              GKI Kernel (通用内核镜像)               │
├─────────────────────────────────────────────────────┤
│           ABI 符号表 (Kernel Module Interface)       │
└─────────────────────────────────────────────────────┘
```

- **ABI (Application Binary Interface)**: 内核导出给模块使用的二进制接口
- **符号表**: 内核导出的函数和变量列表，模块通过这些符号与内核交互
- **CRC 校验**: 每个符号都有 CRC 值，用于验证 ABI 兼容性

### 2. 为什么需要符号对齐？

当你编译一个内核模块时：

```
模块编译时 → 记录依赖的符号 + CRC 值
     ↓
模块加载时 → 内核检查符号是否存在 + CRC 是否匹配
     ↓
不匹配 → 加载失败: "disagrees about version of symbol xxx"
```

**GKI 的解决方案**：Google 维护一个官方的 ABI 符号白名单，所有 GKI 认证设备必须导出相同的符号和 CRC。

---

## 关键文件解析

### 1. ABI 符号白名单位置

| Android 版本 | 内核版本 | 符号表文件路径 |
|-------------|---------|---------------|
| Android 12 | 5.10 | `common/android/abi_gki_aarch64` |
| Android 13 | 5.15 | `common/android/abi_gki_aarch64` |
| Android 14 | 6.1 | `common/android/abi_gki_aarch64` |
| Android 15 | 6.6 | `common/android/abi_gki_aarch64` |
| Android 16 | 6.12 | `common/android/abi_gki_aarch64` |

### 2. 符号表文件格式

```
# common/android/abi_gki_aarch64 示例内容
[abi_symbol_list]
  alloc_pages
  copy_from_user
  copy_to_user
  kmalloc
  kfree
  register_kprobe      # ← kprobe 相关符号
  unregister_kprobe    # ← kprobe 相关符号
  ...
```

### 3. 模块列表文件

```
# common/android/gki_aarch64_modules (Android ≤13)
# 或 common/modules.bzl (Android ≥14)

drivers/kerneldriver/mydriver.ko
```

---

## 工作流程中的符号修补步骤

### 步骤 1: 检查缺失符号

```bash
cd android-kernel

# 检查 kprobe 符号是否在白名单中
grep "register_kprobe" common/android/abi_gki_aarch64
grep "unregister_kprobe" common/android/abi_gki_aarch64
```

### 步骤 2: 添加缺失符号到白名单

```bash
# 如果符号不存在，添加到 ABI 白名单
for symbol in register_kprobe unregister_kprobe; do
  grep -q "$symbol" common/android/abi_gki_aarch64 || \
  echo "$symbol" >> common/android/abi_gki_aarch64
done
```

### 步骤 3: 确保内核导出符号

```bash
# 修改内核源码，确保符号被 EXPORT_SYMBOL 导出
# common/kernel/kprobes.c

# 原始代码可能只有:
EXPORT_SYMBOL(unregister_kprobe);

# 需要添加:
EXPORT_SYMBOL(register_kprobe);
```

### 步骤 4: 编译验证

```bash
# 编译后检查符号是否正确导出
cat out/*/dist/abi_symbollist.txt | grep kprobe
```

---

## 为什么这样做能适配所有 GKI 设备？

### 原理图解

```
┌────────────────────────────────────────────────────────────┐
│                    Google GKI 标准                          │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  官方 ABI 符号表 (abi_gki_aarch64)                    │  │
│  │  - 所有 GKI 认证设备必须导出这些符号                   │  │
│  │  - 符号 CRC 值必须一致                                │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│  你的编译流程                                               │
│  1. 从 Google 官方拉取内核源码                              │
│  2. 使用官方 ABI 符号表                                     │
│  3. 补充缺失的符号 (如 kprobe)                              │
│  4. 编译模块 → 模块的符号依赖 = 官方 ABI                    │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│  任意 GKI 认证设备                                          │
│  - 小米/OPPO/vivo/三星/一加...                              │
│  - 内核符号表 = Google 官方 ABI                             │
│  - CRC 校验通过 ✓                                          │
│  - 模块加载成功 ✓                                          │
└────────────────────────────────────────────────────────────┘
```

### 关键点

1. **统一的符号来源**: 使用 Google 官方内核源码编译，符号 CRC 与所有 GKI 设备一致
2. **白名单机制**: 只要符号在 `abi_gki_aarch64` 中，所有 GKI 设备都必须导出
3. **版本对应**: Android 13 + 5.15 内核的设备，ABI 符号表完全相同

---

## Android 6.12 内核 (Android 16) 的适配

### 获取源码

```bash
mkdir android-kernel && cd android-kernel
repo init -u https://android.googlesource.com/kernel/manifest \
  -b common-android16-6.12
repo sync -j$(nproc)
```

### 符号修补流程

```bash
# 1. 查看当前 ABI 符号表
cat common/android/abi_gki_aarch64

# 2. 检查你的模块需要哪些符号
# 假设你的模块使用了 kprobe
nm -u your_module.ko | grep " U "

# 3. 对比并补充缺失符号
REQUIRED_SYMBOLS=(
  "register_kprobe"
  "unregister_kprobe"
  "register_kretprobe"
  "unregister_kretprobe"
)

for sym in "${REQUIRED_SYMBOLS[@]}"; do
  if ! grep -q "^  $sym$" common/android/abi_gki_aarch64; then
    echo "  $sym" >> common/android/abi_gki_aarch64
    echo "Added: $sym"
  fi
done

# 4. 确保内核源码导出这些符号
# 检查 EXPORT_SYMBOL 声明
grep -r "EXPORT_SYMBOL.*kprobe" common/kernel/
```

### Bazel 构建系统 (Android 14+)

```bash
# Android 14+ 使用 Bazel 构建
tools/bazel run //common:kernel_aarch64_dist

# 模块列表在 modules.bzl 中管理
# common/modules.bzl
_COMMON_GKI_MODULES_LIST = [
    "drivers/kerneldriver/mydriver.ko",
    ...
]
```

---

## 常见问题排查

### 问题 1: 模块加载失败 - 符号版本不匹配

```
insmod: ERROR: could not insert module xxx.ko: Invalid module format
dmesg: xxx: disagrees about version of symbol register_kprobe
```

**解决方案**:
```bash
# 检查模块编译时的符号 CRC
modinfo xxx.ko | grep vermagic

# 检查目标设备的内核符号 CRC
cat /proc/kallsyms | grep register_kprobe

# 确保使用相同版本的内核源码编译
```

### 问题 2: 符号未导出

```
ERROR: "register_kprobe" undefined!
```

**解决方案**:
```bash
# 1. 确认符号在 ABI 白名单中
grep register_kprobe common/android/abi_gki_aarch64

# 2. 确认内核源码有 EXPORT_SYMBOL
grep -r "EXPORT_SYMBOL.*register_kprobe" common/

# 3. 如果没有，手动添加
sed -i '/^int register_kprobe/a EXPORT_SYMBOL(register_kprobe);' \
  common/kernel/kprobes.c
```

### 问题 3: ABI 检查失败

```
ERROR: ABI check failed
```

**解决方案**:
```bash
# 跳过 ABI 检查 (仅用于测试)
BUILD_CONFIG=common/build.config.gki.aarch64 \
  SKIP_ABI_CHECKS=1 \
  build/build.sh

# 或更新 ABI 参考文件
BUILD_CONFIG=common/build.config.gki.aarch64 \
  UPDATE_ABI=1 \
  build/build.sh
```

---

## 版本兼容性矩阵

| Android | 内核版本 | 构建系统 | ABI 文件 | 模块列表 |
|---------|---------|---------|---------|---------|
| 12 | 5.10 | build.sh | abi_gki_aarch64 | gki_aarch64_modules |
| 13 | 5.15 | build.sh | abi_gki_aarch64 | gki_aarch64_modules |
| 14 | 6.1 | Bazel | abi_gki_aarch64 | modules.bzl |
| 15 | 6.6 | Bazel | abi_gki_aarch64 | modules.bzl |
| 16 | 6.12 | Bazel | abi_gki_aarch64 | modules.bzl |

---

## 总结

你的理解是正确的：

1. **从 Google 官方获取内核源码** → 确保 ABI 符号表与所有 GKI 设备一致
2. **修补缺失符号 (如 kprobe)** → 确保模块依赖的符号被导出
3. **编译模块** → 模块的符号 CRC 与官方 ABI 一致
4. **通用加载** → 任何相同 Android 版本 + 内核版本的 GKI 设备都能加载

这就是为什么你编译的 Android 13 + 5.15 内核模块可以在所有 5.15 GKI 设备上运行的原因。

---

## 参考资源

- [Google GKI 官方文档](https://source.android.com/docs/core/architecture/kernel/generic-kernel-image)
- [Android 内核 ABI 监控](https://source.android.com/docs/core/architecture/kernel/abi-monitor)
- [GKI 模块开发指南](https://source.android.com/docs/core/architecture/kernel/loadable-kernel-modules)
