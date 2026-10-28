# MovieClaw HarmonyOS libmpv kernel

MovieClaw 鸿蒙端播放器的 **libmpv 内核构建仓库**：构建脚本、OHCodec/P010 补丁，以及
arm64-v8a / x86_64 预编译 `libmpv.so`。这里不包含 HarmonyOS App 业务代码。

## 上游引用（重要）

本仓库是下面这份社区构建工程的分支，并在此之上增加了 ohcodec 崩溃修复、libplacebo
外部 YUV/P010 支持和 x86_64 构建：

| 组件 | 上游仓库 | 版本 / 分支 | 用在哪 |
|---|---|---|---|
| 构建脚手架（基底） | [labulakalia/libmpv-ohos-build](https://github.com/labulakalia/libmpv-ohos-build) | `main` @ `2281241` | `build.sh` / `download.sh` / `scripts/` / `crossfiles/` 的来源 |
| mpv | [dex2oat/mpv](https://github.com/dex2oat/mpv) | 0.41.0 OHOS fork；下载脚本原本 pin `feat-ohos-0.41.0`，该分支现已不在远端，`master` 保留 OHOS 代码 | 播放器/解复用/字幕/VO |
| FFmpeg | [labulakalia/ohos-FFmpeg](https://github.com/labulakalia/ohos-FFmpeg) | `feat-ohos-n8.0` @ `7dddf74` | 解复用、软/硬解码、P010 输出 |
| libplacebo | [haasn/libplacebo](https://github.com/haasn/libplacebo) | `v7.360.1` + 本仓库补丁 | gpu-next、HDR/DV、GLES 外部 YUV |
| libass | [libass/libass](https://github.com/libass/libass) | `0.17.4` | ASS/SSA 字幕 |
| dav1d | [videolan/dav1d](https://github.com/videolan/dav1d) | `1.5.1` | AV1 软解 |
| FreeType | [freetype/freetype](https://github.com/freetype/freetype) | `VER-2-13-3` | 字体 |
| HarfBuzz | [harfbuzz/harfbuzz](https://github.com/harfbuzz/harfbuzz) | `11.4.5` | 文字整形 |
| FriBidi | [fribidi/fribidi](https://github.com/fribidi/fribidi) | `v1.0.16` | 双向文字 |
| fontconfig | [arthenica/fontconfig](https://github.com/arthenica/fontconfig) | `2.17.1` | 字体配置 |
| libxml2 | [gnome/libxml2](https://github.com/gnome/libxml2) | `v2.15.1` | fontconfig 依赖 |
| mbedTLS | [Mbed-TLS/mbedtls](https://github.com/Mbed-TLS/mbedtls) | `3.6.4` | FFmpeg TLS |
| lcms2 | [mm2/Little-CMS](https://github.com/mm2/Little-CMS) | `lcms2.17` | 色彩管理 |
| shaderc | [google/shaderc](https://github.com/google/shaderc) | `v2025.4` | libplacebo 着色器编译 |
| Lua | [lua/lua](https://github.com/lua/lua) | `5.2.4` | mpv 脚本 |
| dovi_tool | [quietvoid/dovi_tool](https://github.com/quietvoid/dovi_tool) | `2.3.1` | Dolby Vision 元数据 |
| OpenHarmony SDK | 华为 OpenHarmony 6.0-Release 公共 SDK | `ohos-sdk-windows_linux-public` | 交叉编译 |

各依赖的精确分支/标签见 `download/deps-version.sh`、`SOURCES.md` 和 `patches/`。

## 本仓库相对上游的改动

1. **`patches/mpv/0001-ohcodec-uninit-null-guard.patch`**
   - 修复 `ra_hwdec_load_driver()` 在 ohcodec 初始化失败后仍调用 `uninit()`，
     而 `uninit()` 无条件解引用 `p->interop` 导致的 vo 线程 SIGSEGV（空指针）。
   - 同时避免 init 失败路径重复 `owner_uninit()` 的双重释放。
   - 这是在 10-bit HDR 硬解、以及没有 `GL_EXT_YUV_target` 的设备上闪退的根因。
2. **`patches/libplacebo/support-external-yuv-zero-copy.patch`**
   - 为 libplacebo GLES 增加 `__samplerExternal2DY2YEXT`（`PL_SAMPLER_EXTERNAL_YUV`）
     外部 YUV 采样，支撑 OHCodec Surface → OES/P010 的零拷贝路径。
   - 另有 `remove-unnecessary-atomic-test.patch`，去掉构建机上的原子性探测。
3. **x86_64 构建**：`crossfiles/amd64-crossfile-linux.ini`、`ARCH=amd64`，用于 x86_64 模拟器；
   arm64-v8a 用于真机。
4. **预编译产物**：两个 ABI 的 `libmpv.so`、`libmpvbridge.so`（NAPI 桥）与
   `prebuilts/include/mpv/` 头文件；校验见 `prebuilts/SHA256SUMS`。

## 仓库结构

```text
build.sh / bundle.sh / download.sh / patch.sh   构建入口
download/ / scripts/ / crossfiles/              依赖下载与交叉编译
patches/mpv/                                     ohcodec 空指针修复
patches/libplacebo/                              GLES 外部 YUV/P010
bridge/                                          NAPI 桥源码 + CMake
prebuilts/<abi>/                                 libmpv.so / libmpvbridge.so
prebuilts/include/mpv/                           libmpv 头文件
SOURCES.md / THIRD_PARTY.md                      上游来源与许可证
```

## 构建

环境：Linux 或 WSL（Windows 原生不支持）；需要 git/make/python3/meson/gperf/pkg-config/nasm。

```bash
# 1. 准备 OHOS SDK。脚本默认 /sdk/linux，也可改 env.sh 的 OHOS_NDK_HOME
#    对应华为 OpenHarmony 6.0-Release SDK 的 linux/native 目录
./download.sh          # 下载 SDK、Rust 工具链、各依赖源码
./patch.sh             # 应用 patches/（libplacebo + mpv）

# 2. 同时构建 x86_64 + arm64（输出到 libmpv/<arch>-build/libmpv.so）
./bundle.sh

# 只构建一个 ABI：
ARCH=arm64 ./build.sh
```

构建产物放到 HarmonyOS 工程：

```text
entry/libs/arm64-v8a/libmpv.so
entry/libs/x86_64/libmpv.so
```

## 播放器侧使用约定

- 默认 `hwdec=ohcodec`；初始化失败自动回落软件解码。硬解起播 10 秒无帧会再降级一次。
- `vo=gpu-next` 用于 HDR/杜比视界映射；真机 GLES/HDR 输出失败时 App 侧可降级 `vo=gpu`。
- ISO/BDMV 由服务端只给剪辑清单，mpv 在端上解复用 m2ts 并拼接，服务端不起 ffmpeg。
- 本仓库只提供内核，不包含 App 的播放 UI、会话协商或服务端代码。

## 许可

- 本仓库的构建脚手架来源为 MIT（见 `LICENSE`，版权归上游 `ErBW_s` / `libmpv-ohos-build`）。
- mpv 为 LGPL-2.1-or-later（本构建使用 `-Dgpl=false`）；FFmpeg、libplacebo、libass 等
  各自遵循其上游许可证；预编译库的使用/再分发需同时遵守对应上游许可证与源码获取要求。
- 具体声明见 `THIRD_PARTY.md`。
