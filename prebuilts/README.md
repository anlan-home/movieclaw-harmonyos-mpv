# 预编译产物

| ABI | 文件 | 大小（约） | 用途 |
|---|---|---|---|
| arm64-v8a | `arm64-v8a/libmpv.so` | 29 MB | 鸿蒙真机 |
| arm64-v8a | `arm64-v8a/libmpvbridge.so` | 73 KB | NAPI 桥（真机） |
| x86_64 | `x86_64/libmpv.so` | 35 MB | x86_64 模拟器 |
| x86_64 | `x86_64/libmpvbridge.so` | 75 KB | NAPI 桥（模拟器） |

`include/mpv/` 是配套的 libmpv 头文件（client.h / render.h / render_gl.h / stream_cb.h）。

`libmpv.so` 由本仓库脚本 + `patches/` 构建、`llvm-strip --strip-debug` 后产物；
`libmpvbridge.so` 由 `bridge/` 源码按同一 ABI 编译。两者已移除调试信息，
但仍保留动态符号表（因此 `file` 可能显示 not stripped）。均未做 HAP 签名。
校验值见 `SHA256SUMS`。放入 App 的 `entry/libs/<abi>/` 即可被加载。

注意：预编译库内含 FFmpeg/mpv/libplacebo 等 LGPL 组件，再分发时请同时遵守
`THIRD_PARTY.md` 中的上游许可证与源码提供要求。
