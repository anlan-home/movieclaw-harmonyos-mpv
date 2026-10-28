#include <napi/native_api.h>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <native_window/external_window.h>
#include <hilog/log.h>

#include <dlfcn.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

std::mutex g_mutex;
mpv_handle *g_mpv = nullptr;
// 联调：--ps hwdec 传非 no 时打开 mpv 内部日志转发，验证硬解初始化失败原因
static bool hwdecDebug = false;
std::string g_surface_id;
mpv_render_context *g_render = nullptr;
OHNativeWindow *g_window = nullptr;
EGLDisplay g_display = EGL_NO_DISPLAY;
EGLContext g_context = EGL_NO_CONTEXT;
EGLSurface g_surface = EGL_NO_SURFACE;
EGLConfig g_config = nullptr;
int g_width = 0;
int g_height = 0;

std::string argString(napi_env env, napi_value value)
{
    size_t len = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &len) != napi_ok || len == 0) {
        return std::string();
    }
    std::vector<char> buf(len + 1);
    size_t copied = 0;
    if (napi_get_value_string_utf8(env, value, buf.data(), buf.size(), &copied) != napi_ok) {
        return std::string();
    }
    return std::string(buf.data(), copied);
}

napi_value boolValue(napi_env env, bool value)
{
    napi_value out = nullptr;
    napi_get_boolean(env, value, &out);
    return out;
}

napi_value numberValue(napi_env env, double value)
{
    napi_value out = nullptr;
    napi_create_double(env, value, &out);
    return out;
}

napi_value stringValue(napi_env env, const std::string &value)
{
    napi_value out = nullptr;
    napi_create_string_utf8(env, value.c_str(), value.size(), &out);
    return out;
}

bool setOption(const char *name, const std::string &value)
{
    return g_mpv != nullptr && mpv_set_option_string(g_mpv, name, value.c_str()) >= 0;
}

void *eglProcAddress(void *, const char *name)
{
    void *proc = reinterpret_cast<void *>(eglGetProcAddress(name));
    if (proc == nullptr) {
        proc = dlsym(RTLD_DEFAULT, name);
    }
    return proc;
}

void cleanupEgl()
{
    if (g_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_surface != EGL_NO_SURFACE) {
            eglDestroySurface(g_display, g_surface);
        }
        if (g_context != EGL_NO_CONTEXT) {
            eglDestroyContext(g_display, g_context);
        }
        eglTerminate(g_display);
    }
    g_display = EGL_NO_DISPLAY;
    g_context = EGL_NO_CONTEXT;
    g_surface = EGL_NO_SURFACE;
    g_config = nullptr;
    if (g_window != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(g_window);
        g_window = nullptr;
    }
}

bool initEgl(const std::string &surfaceId)
{
    cleanupEgl();
    const uint64_t surface = std::strtoull(surfaceId.c_str(), nullptr, 10);
    if (surface == 0) {
        return false;
    }
    if (OH_NativeWindow_CreateNativeWindowFromSurfaceId(surface, &g_window) != 0 || g_window == nullptr) {
        g_window = nullptr;
        return false;
    }
    g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_display == EGL_NO_DISPLAY || !eglInitialize(g_display, nullptr, nullptr)) {
        cleanupEgl();
        return false;
    }
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLint numConfigs = 0;
    if (!eglChooseConfig(g_display, configAttribs, &g_config, 1, &numConfigs) || numConfigs <= 0) {
        cleanupEgl();
        return false;
    }
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    g_context = eglCreateContext(g_display, g_config, EGL_NO_CONTEXT, contextAttribs);
    if (g_context == EGL_NO_CONTEXT) {
        cleanupEgl();
        return false;
    }
    g_surface = eglCreateWindowSurface(g_display, g_config,
        reinterpret_cast<EGLNativeWindowType>(g_window), nullptr);
    if (g_surface == EGL_NO_SURFACE || !eglMakeCurrent(g_display, g_surface, g_surface, g_context)) {
        cleanupEgl();
        return false;
    }
    eglQuerySurface(g_display, g_surface, EGL_WIDTH, &g_width);
    eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &g_height);
    eglSwapInterval(g_display, 1);
    return true;
}

bool initMpv(const std::string &hwdec, const std::string &ao, bool renderApi)
{
    if (g_mpv != nullptr) {
        return false;
    }
    g_mpv = mpv_create();
    if (g_mpv == nullptr) {
        return false;
    }
    if (renderApi) {
        setOption("vo", "libmpv");
        setOption("gpu-api", "opengl");
    }
    setOption("ao", ao.empty() ? "ohaudio" : ao);
    setOption("hwdec", hwdec.empty() ? "no" : hwdec);
    hwdecDebug = !hwdec.empty() && hwdec != "no";
    if (hwdecDebug) {
        mpv_request_log_messages(g_mpv, "info");
    }
    setOption("keep-open", "yes");
    setOption("idle", "yes");
    setOption("force-window", "yes");
    setOption("terminal", "no");
    setOption("input-default-bindings", "no");
    setOption("input-vo-keyboard", "no");
    setOption("osc", "no");
    setOption("osd-level", "0");
    // 端上蓝光剪辑清单（ffconcat）里的取流地址是 http(s)：concat demuxer 默认只允许本地协议，
    // 这里放开网络协议（引号包住含逗号的取值，mpv 的 keyval 解析支持）。
    setOption("demuxer-lavf-o", "safe=0,protocol_whitelist=\"file,http,https,tcp,tls\"");
    // 字幕仍由 mpv/libass 画（保留 ASS 样式），但缩小并上移，避免盖住底部控制条
    setOption("sub-visibility", "yes");
    setOption("sub-font-size", "34");
    setOption("sub-pos", "100");
    setOption("sub-margin-y", "18");
    setOption("config", "no");
    setOption("load-scripts", "no");
    setOption("ytdl", "no");
    setOption("audio-channels", "auto");
    setOption("cache", "yes");
    if (mpv_initialize(g_mpv) < 0) {
        mpv_terminate_destroy(g_mpv);
        g_mpv = nullptr;
        return false;
    }
    mpv_request_log_messages(g_mpv, "warn");
    return true;
}

void cleanupAll()
{
    if (g_render != nullptr) {
        mpv_render_context_free(g_render);
        g_render = nullptr;
    }
    if (g_mpv != nullptr) {
        mpv_terminate_destroy(g_mpv);
        g_mpv = nullptr;
    }
    cleanupEgl();
}

napi_value Create(napi_env env, napi_callback_info info)
{
    size_t argc = 5;
    napi_value args[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return boolValue(env, false);
    }
    const std::string surface = argString(env, args[0]);
    const std::string hwdec = argc >= 2 ? argString(env, args[1]) : std::string("no");
    const std::string vo = argc >= 3 ? argString(env, args[2]) : std::string("gpu");
    const std::string ao = argc >= 4 ? argString(env, args[3]) : std::string("ohaudio");
    const std::string gpuApi = argc >= 5 ? argString(env, args[4]) : std::string("opengl");
    if (surface.empty()) {
        return boolValue(env, false);
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    // 同一个 XComponent surface 直接复用已初始化的 mpv：换集/换画质只 loadfile replace，
    // 不再重新 mpv_initialize + 建 OHOS VO（这条路径原来会卡 UI 并反复泄漏资源）
    if (g_mpv != nullptr && g_surface_id == surface) {
        return boolValue(env, true);
    }
    if (g_mpv != nullptr) {
        mpv_terminate_destroy(g_mpv);
        g_mpv = nullptr;
    }
    mpv_handle *handle = mpv_create();
    if (handle == nullptr) {
        return boolValue(env, false);
    }
    g_mpv = handle;
    g_surface_id = surface;

    setOption("vo", vo);
    setOption("gpu-context", "ohos");
    setOption("gpu-api", gpuApi);
    setOption("wid", surface);
    setOption("ao", ao);
    setOption("hwdec", hwdec.empty() ? "no" : hwdec);
    setOption("keep-open", "yes");
    setOption("idle", "yes");
    setOption("force-window", "yes");
    setOption("terminal", "no");
    setOption("input-default-bindings", "no");
    setOption("input-vo-keyboard", "no");
    setOption("osc", "no");
    setOption("osd-level", "0");
    // 端上蓝光剪辑清单（ffconcat）里的取流地址是 http(s)：concat demuxer 默认只允许本地协议，
    // 这里放开网络协议（引号包住含逗号的取值，mpv 的 keyval 解析支持）。
    setOption("demuxer-lavf-o", "safe=0,protocol_whitelist=\"file,http,https,tcp,tls\"");
    // 字幕仍由 mpv/libass 画（保留 ASS 样式），但缩小并上移，避免盖住底部控制条
    setOption("sub-visibility", "yes");
    setOption("sub-font-size", "34");
    setOption("sub-pos", "100");
    setOption("sub-margin-y", "18");
    setOption("config", "no");
    setOption("load-scripts", "no");
    setOption("ytdl", "no");
    setOption("audio-channels", "auto");
    setOption("cache", "yes");
    /*
     * 杜比视界（尤其 Profile 5 IPTPQc2）：vo_gpu 的 RPU 映射只对 disable_residual 的
     * RPU 生效，其余靠 guess_csp 退回 BT.2020 —— 缺省画质链按错误系数转 RGB 会整体偏绿。
     * 显式给 tone-map/gamut 配置让 HDR→SDR 走正常流程；支持 HDR 的屏幕由
     * target-colorspace-hint 把 PQ 原样交给系统合成。
     */
    setOption("tone-mapping", "bt.2446a");
    setOption("tone-mapping-mode", "hybrid");
    setOption("gamut-mapping-mode", "perceptual");
    setOption("target-colorspace-hint", "yes");

    if (mpv_initialize(g_mpv) < 0) {
        mpv_terminate_destroy(g_mpv);
        g_mpv = nullptr;
        return boolValue(env, false);
    }
    mpv_request_log_messages(g_mpv, "warn");
    return boolValue(env, true);
}

napi_value Destroy(napi_env env, napi_callback_info)
{
    mpv_handle *old = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        old = g_mpv;
        g_mpv = nullptr;
        g_surface_id.clear();
        if (g_render != nullptr) {
            mpv_render_context_free(g_render);
            g_render = nullptr;
        }
    }
    // mpv_terminate_destroy 会等解码/网络线程退出，不能再压 UI 线程；
    // 句柄已经摘走，后台销毁期间可以安全地创建下一个实例
    if (old != nullptr) {
        std::thread([old]() { mpv_terminate_destroy(old); }).detach();
    }
    return boolValue(env, true);
}

/**
 * 创建 render API 播放器：mpv 只管解码/音频，视频帧由我们的 EGL/GLES 画到
 * XComponent 的 OHNativeWindow 上。这样绕开 mpv OHOS VO 在 x86_64 模拟器上的崩溃。
 */
napi_value CreateRender(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3] = {nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return boolValue(env, false);
    }
    const std::string surface = argString(env, args[0]);
    const std::string hwdec = argc >= 2 ? argString(env, args[1]) : std::string("no");
    const std::string ao = argc >= 3 ? argString(env, args[2]) : std::string("ohaudio");
    if (surface.empty()) {
        return boolValue(env, false);
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    cleanupAll();
    if (!initEgl(surface)) {
        cleanupAll();
        return boolValue(env, false);
    }
    if (!initMpv(hwdec, ao, true)) {
        cleanupAll();
        return boolValue(env, false);
    }
    mpv_opengl_init_params glInit = {
        .get_proc_address = eglProcAddress,
        .get_proc_address_ctx = nullptr,
    };
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    if (mpv_render_context_create(&g_render, g_mpv, params) < 0) {
        cleanupAll();
        return boolValue(env, false);
    }
    return boolValue(env, true);
}

napi_value RenderFrame(napi_env env, napi_callback_info)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_render == nullptr || g_display == EGL_NO_DISPLAY || g_surface == EGL_NO_SURFACE) {
        return boolValue(env, false);
    }
    int width = g_width;
    int height = g_height;
    eglQuerySurface(g_display, g_surface, EGL_WIDTH, &width);
    eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &height);
    if (width <= 0 || height <= 0) {
        return boolValue(env, false);
    }
    g_width = width;
    g_height = height;
    glViewport(0, 0, width, height);
    mpv_opengl_fbo fbo = {
        .fbo = 0,
        .w = width,
        .h = height,
        .internal_format = 0,
    };
    int flipY = 1;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    if (mpv_render_context_update(g_render) & MPV_RENDER_UPDATE_FRAME) {
        mpv_render_context_render(g_render, params);
        eglSwapBuffers(g_display, g_surface);
        mpv_render_context_report_swap(g_render);
    }
    return boolValue(env, true);
}

napi_value Load(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3] = {nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return boolValue(env, false);
    }
    const std::string url = argString(env, args[0]);
    std::string headers = argc >= 2 ? argString(env, args[1]) : std::string();
    double startSeconds = 0;
    if (argc >= 3) {
        napi_get_value_double(env, args[2], &startSeconds);
    }
    if (url.empty()) {
        return boolValue(env, false);
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_mpv == nullptr) {
        return boolValue(env, false);
    }
    if (!headers.empty()) {
        // http-header-fields is a string-list option. Our callers only pass simple
        // "Header: value" lines, so a comma separator is safe enough here.
        std::string joined;
        size_t start = 0;
        while (start < headers.size()) {
            size_t end = headers.find('\n', start);
            if (end == std::string::npos) {
                end = headers.size();
            }
            if (!joined.empty()) {
                joined += ",";
            }
            joined += headers.substr(start, end - start);
            start = end + 1;
        }
        mpv_set_option_string(g_mpv, "http-header-fields", joined.c_str());
    }
    char startOpt[64];
    // loadfile 的 start 选项让新流直接从续播点起，避免先播 0 秒再 seek（HLS 转码流会把过早 seek 丢掉）
    if (std::isfinite(startSeconds) && startSeconds > 0.1) {
        std::snprintf(startOpt, sizeof(startOpt), "start=%.3f", startSeconds);
        const char *cmd[] = {"loadfile", url.c_str(), "replace", "-1", startOpt, nullptr};
        return boolValue(env, mpv_command_async(g_mpv, 0, cmd) >= 0);
    }
    const char *cmd[] = {"loadfile", url.c_str(), "replace", nullptr};
    // loadfile 会等网络/探测，必须异步：入队后立刻返回，由 playback-restart / end-file 事件推进状态
    return boolValue(env, mpv_command_async(g_mpv, 0, cmd) >= 0);
}

napi_value Command(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return boolValue(env, false);
    }
    const std::string action = argString(env, args[0]);
    const std::string value = argc >= 2 ? argString(env, args[1]) : std::string();

    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_mpv == nullptr) {
        return boolValue(env, false);
    }
    bool ok = false;
    if (action == "pause") {
        ok = mpv_set_property_string(g_mpv, "pause", value == "true" ? "yes" : "no") >= 0;
    } else if (action == "seek") {
        char *end = nullptr;
        const double seconds = std::strtod(value.c_str(), &end);
        if (end != value.c_str() && std::isfinite(seconds)) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.3f", seconds);
            const char *cmd[] = {"seek", buf, "absolute+exact", nullptr};
            ok = mpv_command_async(g_mpv, 0, cmd) >= 0;
        }
    } else if (action == "speed") {
        ok = mpv_set_property_string(g_mpv, "speed", value.c_str()) >= 0;
    } else if (action == "volume") {
        ok = mpv_set_property_string(g_mpv, "volume", value.c_str()) >= 0;
    } else if (action == "audio") {
        ok = mpv_set_property_string(g_mpv, "aid", value.c_str()) >= 0;
    } else if (action == "hwdec") {
        ok = mpv_set_property_string(g_mpv, "hwdec", value.c_str()) >= 0;
    } else if (action == "subtitle") {
        ok = mpv_set_property_string(g_mpv, "sid", value.c_str()) >= 0;
    } else if (action == "subadd") {
        // 外挂字幕 URL 可能被服务端拖到几秒才返回；同步 sub-add 曾把 ArkUI 主线程按住 9 秒
        const char *cmd[] = {"sub-add", value.c_str(), "select", nullptr};
        ok = mpv_command_async(g_mpv, 0, cmd) >= 0;
    } else if (action == "subpos") {
        ok = mpv_set_property_string(g_mpv, "sub-pos", value.c_str()) >= 0;
    } else if (action == "subfontsize") {
        ok = mpv_set_property_string(g_mpv, "sub-font-size", value.c_str()) >= 0;
    } else if (action == "subdelay") {
        ok = mpv_set_property_string(g_mpv, "sub-delay", value.c_str()) >= 0;
    } else if (action == "subborderstyle") {
        ok = mpv_set_property_string(g_mpv, "sub-border-style", value.c_str()) >= 0;
    } else if (action == "panscan") {
        ok = mpv_set_property_string(g_mpv, "panscan", value.c_str()) >= 0;
    } else if (action == "keepaspect") {
        ok = mpv_set_property_string(g_mpv, "keepaspect", value.c_str()) >= 0;
    } else if (action == "stop") {
        const char *cmd[] = {"stop", nullptr};
        ok = mpv_command_async(g_mpv, 0, cmd) >= 0;
    }
    return boolValue(env, ok);
}

napi_value GetNumber(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return numberValue(env, NAN);
    }
    const std::string name = argString(env, args[0]);
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_mpv == nullptr) {
        return numberValue(env, NAN);
    }
    double value = NAN;
    if (mpv_get_property(g_mpv, name.c_str(), MPV_FORMAT_DOUBLE, &value) < 0) {
        return numberValue(env, NAN);
    }
    return numberValue(env, value);
}

napi_value GetBool(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return boolValue(env, false);
    }
    const std::string name = argString(env, args[0]);
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_mpv == nullptr) {
        return boolValue(env, false);
    }
    int flag = 0;
    if (mpv_get_property(g_mpv, name.c_str(), MPV_FORMAT_FLAG, &flag) < 0) {
        return boolValue(env, false);
    }
    return boolValue(env, flag != 0);
}

napi_value GetString(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return stringValue(env, "");
    }
    const std::string name = argString(env, args[0]);
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_mpv == nullptr) {
        return stringValue(env, "");
    }
    char *value = mpv_get_property_string(g_mpv, name.c_str());
    if (value == nullptr) {
        return stringValue(env, "");
    }
    std::string out(value);
    mpv_free(value);
    return stringValue(env, out);
}

/**
 * 窗口尺寸变了（横竖屏切换）：把新的物理像素尺寸告诉 mpv。
 * OHOS 的 ra_ctx 没有窗口 resize 事件（control 返回 VO_NOTIMPL），只靠
 * GET_BUFFER_GEOMETRY 在启动时读一次；不主动喂尺寸，转屏后画面会卡在旧大小。
 * 与 mpv-android 的 surfaceChanged 一样走 `android-surface-size` 的 OHOS 对应项。
 */
napi_value SetSurfaceSize(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 2) {
        return boolValue(env, false);
    }
    int32_t width = 0;
    int32_t height = 0;
    if (napi_get_value_int32(env, args[0], &width) != napi_ok ||
        napi_get_value_int32(env, args[1], &height) != napi_ok ||
        width <= 0 || height <= 0) {
        return boolValue(env, false);
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_mpv == nullptr) {
        return boolValue(env, false);
    }
    char size[32];
    std::snprintf(size, sizeof(size), "%dx%d", width, height);
    return boolValue(env, mpv_set_property_string(g_mpv, "ohos-surface-size", size) >= 0);
}

napi_value DrainEvents(napi_env env, napi_callback_info)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    napi_value array = nullptr;
    napi_create_array(env, &array);
    if (g_mpv == nullptr) {
        return array;
    }
    uint32_t index = 0;
    while (true) {
        mpv_event *event = mpv_wait_event(g_mpv, 0);
        if (event == nullptr || event->event_id == MPV_EVENT_NONE) {
            break;
        }
        std::string name(mpv_event_name(event->event_id));
        if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
            // Do not return log text: URLs/tokens can appear in decoder logs.
            // 联调hwdec时转发到hilog（token 泄露面与本地调试流一致，生产 hwdec=no 不受影响）
            mpv_event_log_message *msg = (mpv_event_log_message *)event->data;
            if (msg != nullptr && msg->text != nullptr && hwdecDebug) {
                std::string text(msg->text);
                while (!text.empty() && (text.back() == 0x0A || text.back() == 0x0D)) {
                    text.pop_back();
                }
                if (!text.empty()) {
                    OH_LOG_Print(LOG_APP, LOG_INFO, 0x0000, "MpvLog", "%{public}s", text.c_str());
                }
            }
            continue;
        }
        napi_value item = nullptr;
        napi_create_string_utf8(env, name.c_str(), name.size(), &item);
        napi_set_element(env, array, index++, item);
    }
    return array;
}

napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"create", nullptr, Create, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createRender", nullptr, CreateRender, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"renderFrame", nullptr, RenderFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"destroy", nullptr, Destroy, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"load", nullptr, Load, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"command", nullptr, Command, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getNumber", nullptr, GetNumber, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getBool", nullptr, GetBool, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getString", nullptr, GetString, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setSurfaceSize", nullptr, SetSurfaceSize, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"drainEvents", nullptr, DrainEvents, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}

napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "mpvbridge",
    .nm_priv = nullptr,
    .reserved = {0},
};

}  // namespace

extern "C" __attribute__((constructor)) void RegisterMpvBridgeModule(void)
{
    napi_module_register(&g_module);
}
