# 第三方组件

本仓库的构建脚手架沿用上游 MIT 许可（`LICENSE`）。预编译 `libmpv.so` 内含/链接以下
上游组件，使用时请遵守各自许可证：

- mpv / libmpv：LGPL-2.1-or-later（本构建 `-Dgpl=false`）
  https://github.com/mpv-player/mpv
- FFmpeg：LGPL-2.1-or-later（取决于构建开关；OHOS fork 见 SOURCES.md）
  https://ffmpeg.org/legal.html
- libplacebo：LGPL-2.1-or-later
- libass：ISC
- dav1d：BSD-2-Clause
- FreeType：FTL / GPLv2（二选一）
- HarfBuzz：Old MIT
- FriBidi：LGPL-2.1-or-later
- fontconfig：MIT
- libxml2：MIT
- mbedTLS：Apache-2.0
- lcms2：MIT
- shaderc：Apache-2.0
- Lua：MIT
- dovi_tool：MIT

各项目的完整许可证与源码以其上游仓库为准；预编译产物不改变上游许可。
