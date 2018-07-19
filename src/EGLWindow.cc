// Copyright 2017 Keri Oleg

#include <EGL/egl.h>
#include <linux/input.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#include <cstring>
#include <stdexcept>
#include "EGLWindow.hh"

class EGLWindow::Impl {
    wl_display *display_;
    wl_compositor *compositor_;
    wl_surface *surface_;
    wl_shell *shell_;
    wl_shell_surface *shell_surface_;
    wl_registry *registry_;
    wl_egl_window *window_;
    wl_callback *callback_;
    wl_cursor_theme *cursor_theme_;
    wl_cursor *default_cursor_;
    wl_surface *cursor_surface_;
    wl_seat *seat_;
    wl_keyboard *keyboard_;
    EGLSurface egl_surface_;
    EGLDisplay dpy_;
    EGLContext ctx_;
    EGLConfig conf_;

    RenderFn render_;
    bool stop_;

    const wl_callback_listener frame_listener_ = {
        redraw
    };

  public:

    Impl(int width, int height, RenderFn render) :
            render_(render), stop_(false), keyboard_(NULL) {
        static const wl_registry_listener registry_listener = {
            global_registry_handler
        };

        display_ = wl_display_connect(NULL);
        if (display_ == nullptr) {
            throw std::runtime_error("Can't connect to display");
        }
        registry_ = wl_display_get_registry(display_);
        wl_registry_add_listener(registry_, &registry_listener, this);
        wl_display_dispatch(display_);

        static const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
        };

        EGLint config_attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 1,
            EGL_GREEN_SIZE, 1,
            EGL_BLUE_SIZE, 1,
            EGL_ALPHA_SIZE, 1,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE
        };

        dpy_ = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_));
        eglInitialize(dpy_, NULL, NULL);
        eglBindAPI(EGL_OPENGL_ES_API);

        EGLint n;
        eglChooseConfig(dpy_, config_attribs, &conf_, 1, &n);
        ctx_ = eglCreateContext(dpy_, conf_, EGL_NO_CONTEXT, context_attribs);
        surface_ = wl_compositor_create_surface(compositor_);
        shell_surface_ = wl_shell_get_shell_surface(shell_, surface_);

        surface_ = wl_compositor_create_surface(compositor_);
        shell_surface_ = wl_shell_get_shell_surface(shell_, surface_);

        window_ = wl_egl_window_create(surface_, width, height);
        egl_surface_ = eglCreateWindowSurface(
            dpy_, conf_, reinterpret_cast<EGLNativeWindowType>(window_), NULL);

        wl_shell_surface_set_title(shell_surface_, "SecureVision");

        eglMakeCurrent(dpy_, egl_surface_, egl_surface_, ctx_);
        wl_shell_surface_set_fullscreen(shell_surface_,
                                        WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                        0, NULL);

        callback_ = wl_display_sync(display_);
	wl_callback_add_listener(callback_, &frame_listener_, this);
        cursor_surface_ = wl_compositor_create_surface(compositor_);
    }

    ~Impl() {
        wl_egl_window_destroy(window_);

        wl_shell_surface_destroy(shell_surface_);
        wl_surface_destroy(surface_);

        if (callback_) {
            wl_callback_destroy(callback_);
        }

        eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(dpy_);
        eglReleaseThread();

        wl_surface_destroy(cursor_surface_);
        if (cursor_theme_) {
            wl_cursor_theme_destroy(cursor_theme_);
        }

        if (shell_) {
            wl_shell_destroy(shell_);
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
            ret = wl_display_dispatch(display_);
        }
        return ret;
    }

    inline void stop() {
        stop_ = true;
    }

    inline void render() {
        render_();
    }

    static void global_registry_handler(void *data, wl_registry *registry,
                                        uint32_t id, const char *interface,
                                        uint32_t version) {
        Impl *window = reinterpret_cast<Impl *>(data);
        if (strcmp(interface, "wl_compositor") == 0) {
            window->compositor_ =
                    reinterpret_cast<wl_compositor *>(wl_registry_bind(
                        registry, id, &wl_compositor_interface, 1));
        } else if (strcmp(interface, "wl_shell") == 0) {
            window->shell_ = reinterpret_cast<wl_shell *>(
                wl_registry_bind(registry, id, &wl_shell_interface, 1));
        } else if (strcmp(interface, "wl_seat") == 0) {
            static const struct wl_seat_listener seat_listener = {
                seat_handle_capabilities,
            };

            window->seat_ = reinterpret_cast<wl_seat *>(
                wl_registry_bind(registry, id, &wl_seat_interface, 1));
            wl_seat_add_listener(window->seat_, &seat_listener, window);

        } else if (strcmp(interface, "wl_shm") == 0) {
            wl_shm *shm = reinterpret_cast<wl_shm *>(
                wl_registry_bind(registry, id, &wl_shm_interface, 1));
            window->cursor_theme_ = wl_cursor_theme_load(NULL, 32, shm);
            window->default_cursor_ =
                    wl_cursor_theme_get_cursor(window->cursor_theme_, "left_ptr");
        }

    }
    static void seat_handle_capabilities(void *data, struct wl_seat *seat,
                                         unsigned int caps) {
        Impl *window = reinterpret_cast<Impl *>(data);

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !window->keyboard_) {
            static const struct wl_keyboard_listener keyboard_listener = {
                keyboard_handle_keymap,
                keyboard_handle_enter,
                keyboard_handle_leave,
                keyboard_handle_key,
                keyboard_handle_modifiers
            };

            window->keyboard_ = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(window->keyboard_, &keyboard_listener, window);
        } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && window->keyboard_) {
            wl_keyboard_destroy(window->keyboard_);
            window->keyboard_ = NULL;
        }
    }

    static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                                       uint32_t format, int fd, uint32_t size) {
    }

    static void  keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, struct wl_surface *surface,
                          struct wl_array *keys)  {
    }

    static void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                                      uint32_t serial, struct wl_surface *surface) {
    }

    static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, uint32_t time, uint32_t key,
                                    uint32_t state) {
        if (key == KEY_ESC || key == KEY_Q) {
            reinterpret_cast<Impl *>(data)->stop();
        }
    }

    static void keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                                          uint32_t serial, uint32_t mods_depressed,
                                          uint32_t mods_latched, uint32_t mods_locked,
                                          uint32_t group) {
    }

    static void redraw(void *data, wl_callback *callback, uint32_t time) {
        static bool inited  = false;
        Impl *window = reinterpret_cast<Impl *>(data);
        if (callback) {
            wl_callback_destroy(callback);
        }

        if (inited) {
            window->render();
        } else {
            inited = true;
        }

        window->callback_ = wl_surface_frame(window->surface_);

        wl_callback_add_listener(window->callback_,
                                 &window->frame_listener_,
                                 window);
        eglSwapBuffers(window->dpy_, window->egl_surface_);
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
