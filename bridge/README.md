# libmpvbridge（HarmonyOS NAPI 桥）

这是 MovieClaw 播放器使用的薄 NAPI 层：负责 mpv 生命周期、属性读写、事件队列与
OHOS Surface 绑定；渲染仍由 libmpv 自己的 OHOS VO 完成。

构建（在 DevEco/HarmonyOS 工程中）：

- 把 `mpv_bridge.cpp` 加入 `entry/src/main/cpp`；
- 参考 `CMakeLists.txt` 链接 `prebuilts/include/mpv` 与
  `prebuilts/<abi>/libmpv.so`、`libace_napi.z.so`、EGL、GLESv3、native_window、hilog；
- `OHOS_ARCH` 使用 `arm64-v8a` 或 `x86_64`。

`prebuilts/<abi>/libmpvbridge.so` 是 Stripped 的预编译产物，供直接联调使用。
