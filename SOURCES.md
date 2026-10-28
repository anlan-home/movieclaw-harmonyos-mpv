# 依赖来源清单

## 构建脚手架与内核

| 组件 | 仓库 | 版本/分支 | 当前参考 commit |
|---|---|---|---|
| 构建脚手架 | https://github.com/labulakalia/libmpv-ohos-build | main | `22812413a569652d1fc60b6219f070e1e80626ea` |
| mpv（OHOS fork） | https://github.com/dex2oat/mpv | 0.41.0；原 pin `feat-ohos-0.41.0`，现远端 `master` | `d974ee332d73e4f79bc39f179f6edf99f13af6e5`（master） |
| FFmpeg（OHOS fork） | https://github.com/labulakalia/ohos-FFmpeg | `feat-ohos-n8.0` | `7dddf74de0346fee1f17bd49563efc5ab9deaf98` |
| libplacebo | https://github.com/haasn/libplacebo | `v7.360.1` | 见 tag |
| libass | https://github.com/libass/libass | `0.17.4` | 见 tag |
| dav1d | https://github.com/videolan/dav1d | `1.5.1` | 见 tag |
| mbedTLS | https://github.com/Mbed-TLS/mbedtls | `3.6.4` | 见 release |
| libxml2 | https://github.com/gnome/libxml2 | `v2.15.1` | 见 tag |
| fribidi | https://github.com/fribidi/fribidi | `v1.0.16` | 见 tag |
| FreeType | https://github.com/freetype/freetype | `VER-2-13-3` | 见 tag |
| HarfBuzz | https://github.com/harfbuzz/harfbuzz | `11.4.5` | 见 tag |
| fontconfig | https://github.com/arthenica/fontconfig | `2.17.1` | 见 tag |
| lcms | https://github.com/mm2/Little-CMS | `lcms2.17` | 见 tag |
| shaderc | https://github.com/google/shaderc | `v2025.4` | 见 tag |
| dovi_tool | https://github.com/quietvoid/dovi_tool | `2.3.1` | 见 tag |
| Lua | https://github.com/lua/lua | `5.2.4` | 见 tag |

## SDK

- OpenHarmony SDK `6.0-Release`
- 下载地址（脚本使用）：
  `https://repo.huaweicloud.com/openharmony/os/6.0-Release/ohos-sdk-windows_linux-public.tar.gz`

## 说明

`mpv` 的 `feat-ohos-0.41.0` 分支已不在远端公开列表；本仓库记录的源码快照版本为 mpv
0.41.0 的 OHOS fork。重新构建时 `download/download-deps.sh` 若报分支不存在，需要把
`V_MPV` 改成该 fork 当前仍可用的分支（例如 `master`），或从构建机上的源码快照复制
`libmpv/mpv`。其余依赖仍按 `download/deps-version.sh` 的标签拉取。
