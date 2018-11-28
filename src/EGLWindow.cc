// Copyright 2017 Keri Oleg

#include <EGL/egl.h>
#include <linux/input.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include "EGLWindow.hh"
#include <atomic>
#include <cstring>
#include <stdexcept>

#include <xdg-shell-unstable-v6-client-protocol.h>

class EGLWindow::Impl {
    wl_display* display_;
    wl_compositor* compositor_;
    wl_surface* surface_;
    wl_registry* registry_;
    wl_egl_window* window_;
    wl_cursor_theme* cursor_theme_;
    wl_cursor* default_cursor_;
    wl_surface* cursor_surface_;
    wl_keyboard* keyboard_;
    wl_seat* seat_;

    zxdg_shell_v6* shell_;
    zxdg_surface_v6* xdg_surface_;
    zxdg_toplevel_v6* toplevel_;

    EGLSurface egl_surface_;
    EGLDisplay dpy_;
    EGLContext ctx_;
    EGLConfig conf_;

    RenderFn render_;
    std::atomic_bool stop_;
    bool configured_;

    bool pressed_;
    int x_;
    int y_;

  public:
    Impl(int width, int height, RenderFn render) :
        keyboard_(nullptr),
        render_(render),
        stop_(false) {
        static const wl_keyboard_listener keyboard_listener = {
            [](void*, wl_keyboard*, uint32_t, int, uint32_t) {},
            [](void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {},
            [](void*, wl_keyboard*, uint32_t, wl_surface*) {},
            [](void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key,
                uint32_t) {
                if (key == KEY_ESC || key == KEY_Q) {
                    reinterpret_cast<Impl*>(data)->stop();
                }
            },
            [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t,
                uint32_t) {},
            nullptr};

        static const wl_seat_listener seat_listener = {
            [](void* data, wl_seat* seat, unsigned caps) {
                Impl* window = reinterpret_cast<Impl*>(data);

                if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) &&
                    !window->keyboard_) {
                    window->keyboard_ = wl_seat_get_keyboard(seat);
                    wl_keyboard_add_listener(
                        window->keyboard_, &keyboard_listener, window);
                } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) &&
                           window->keyboard_) {
                    wl_keyboard_destroy(window->keyboard_);
                    window->keyboard_ = nullptr;
                }
            },
            nullptr};

        static const zxdg_shell_v6_listener xdg_shell_listener = {
            [](void*, zxdg_shell_v6* shell, uint32_t serial) {
                zxdg_shell_v6_pong(shell, serial);
            }};

        static const wl_registry_listener registry_listener = {
            [](void* data, wl_registry* registry, uint32_t id,
                const char* interface, uint32_t) {
                Impl* window = reinterpret_cast<Impl*>(data);
                if (strcmp(interface, wl_compositor_interface.name) == 0) {
                    window->compositor_ =
                        reinterpret_cast<wl_compositor*>(wl_registry_bind(
                            registry, id, &wl_compositor_interface, 1));
                } else if (strcmp(interface, zxdg_shell_v6_interface.name) ==
                           0) {
                    window->shell_ =
                        reinterpret_cast<zxdg_shell_v6*>(wl_registry_bind(
                            registry, id, &zxdg_shell_v6_interface, 1));
                    zxdg_shell_v6_add_listener(
                        window->shell_, &xdg_shell_listener, data);

                } else if (strcmp(interface, wl_seat_interface.name) == 0) {
                    window->seat_ = reinterpret_cast<wl_seat*>(
                        wl_registry_bind(registry, id, &wl_seat_interface, 1));
                    wl_seat_add_listener(window->seat_, &seat_listener, window);
                } else if (strcmp(interface, wl_shm_interface.name) == 0) {
                    wl_shm* shm = reinterpret_cast<wl_shm*>(
                        wl_registry_bind(registry, id, &wl_shm_interface, 1));
                    window->cursor_theme_ = wl_cursor_theme_load(NULL, 32, shm);
                    window->default_cursor_ = wl_cursor_theme_get_cursor(
                        window->cursor_theme_, "left_ptr");
                }
            },
            nullptr};

        static const zxdg_surface_v6_listener xdg_surface_listener = {
            [](void* data, zxdg_surface_v6* surface, uint32_t serial) {
                Impl* window = reinterpret_cast<Impl*>(data);
                zxdg_surface_v6_ack_configure(surface, serial);
                window->configured_ = true;
            }};

        // create display
        display_ = wl_display_connect(NULL);
        if (display_ == nullptr) {
            throw std::runtime_error("Can't connect to display");
        }

        registry_ = wl_display_get_registry(display_);
        wl_registry_add_listener(registry_, &registry_listener, this);
        wl_display_dispatch(display_);
        wl_display_roundtrip(display_);

        if (!compositor_ || !shell_) {
            throw std::runtime_error("Can't init wayland window");
        }
        // init surface and toplevel
        surface_ = wl_compositor_create_surface(compositor_);
        xdg_surface_ = zxdg_shell_v6_get_xdg_surface(shell_, surface_);
        toplevel_ = zxdg_surface_v6_get_toplevel(xdg_surface_);

        zxdg_surface_v6_add_listener(xdg_surface_, &xdg_surface_listener, this);
        zxdg_toplevel_v6_set_title(toplevel_, "SecureVision");
        cursor_surface_ = wl_compositor_create_surface(compositor_);
        configured_ = false;

        wl_surface_commit(surface_);

        // init egl
        static const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

        EGLint config_attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1,
            EGL_ALPHA_SIZE, 1, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE};

        dpy_ = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_));
        eglInitialize(dpy_, NULL, NULL);
        eglBindAPI(EGL_OPENGL_ES_API);

        EGLint n;
        eglChooseConfig(dpy_, config_attribs, &conf_, 1, &n);
        ctx_ = eglCreateContext(dpy_, conf_, EGL_NO_CONTEXT, context_attribs);
        window_ = wl_egl_window_create(surface_, width, height);
        egl_surface_ = eglCreateWindowSurface(dpy_, conf_,
            reinterpret_cast<EGLNativeWindowType>(window_), nullptr);

        wl_display_roundtrip(display_);
        wl_surface_commit(surface_);

        eglMakeCurrent(dpy_, egl_surface_, egl_surface_, ctx_);
    }

    ~Impl() {
        wl_egl_window_destroy(window_);

        zxdg_toplevel_v6_destroy(toplevel_);
        zxdg_surface_v6_destroy(xdg_surface_);
        wl_surface_destroy(surface_);

        eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(dpy_);
        eglReleaseThread();

        wl_surface_destroy(cursor_surface_);
        if (cursor_theme_) {
            wl_cursor_theme_destroy(cursor_theme_);
        }

        if (compositor_) {
            wl_compositor_destroy(compositor_);
        }

        wl_registry_destroy(registry_);
        wl_display_flush(display_);
        wl_display_disconnect(display_);
    }

    int loop() {
        int ret;
        while (!stop_ && ret != -1) {
            if (configured_) {
                ret = wl_display_dispatch_pending(display_);
                render_();
                wl_surface_frame(surface_);
                eglSwapBuffers(dpy_, egl_surface_);
            } else {
                ret = wl_display_dispatch(display_);
            }
        }
        return 0;
    }

    void stop() {
        stop_ = true;
    }
};

EGLWindow::EGLWindow(int width, int height, RenderFn render) :
    pImpl_(std::make_unique<EGLWindow::Impl>(width, height, render)) {
}

EGLWindow::~EGLWindow() {
}

int EGLWindow::loop() {
    return pImpl_->loop();
}

void EGLWindow::stop() {
    pImpl_->stop();
}
