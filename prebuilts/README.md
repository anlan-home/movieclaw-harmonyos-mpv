# 预编译库

| ABI | 文件 | 大小（约） | 用途 |
|---|---|---|---|
| arm64-v8a | `arm64-v8a/libmpv.so` | 29 MB | 鸿蒙真机 |
| x86_64 | `x86_64/libmpv.so` | 35 MB | x86_64 模拟器 |

均由本仓库脚本 + `patches/` 构建、`llvm-strip --strip-debug` 后产物；未做 HAP/APK 签名。
校验值见 `SHA256SUMS`。放入 App 的 `entry/libs/<abi>/libmpv.so` 即可被
`libmpvbridge.so` 动态加载。

注意：预编译库内含 FFmpeg/mpv/libplacebo 等 LGPL 组件，再分发时请同时遵守
`THIRD_PARTY.md` 中的上游许可证与源码提供要求。
