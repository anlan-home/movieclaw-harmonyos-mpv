cmake_minimum_required(VERSION 3.5.0)
project(mpvbridge)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${NATIVERENDER_ROOT_PATH}
    ${NATIVERENDER_ROOT_PATH}/../prebuilts/include
)

# 预编译 libmpv 按 ABI 放在 ../prebuilts/<OHOS_ARCH>/libmpv.so
set(MPV_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../prebuilts/${OHOS_ARCH})
add_library(mpv SHARED IMPORTED)
set_target_properties(mpv PROPERTIES IMPORTED_LOCATION ${MPV_LIB_DIR}/libmpv.so)

add_library(mpvbridge SHARED mpv_bridge.cpp)
target_link_libraries(mpvbridge PUBLIC libace_napi.z.so mpv EGL GLESv3 native_window dl libhilog_ndk.z.so)
