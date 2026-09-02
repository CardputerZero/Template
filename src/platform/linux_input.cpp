#include "linux_input.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !USE_DESKTOP
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifndef APP_KEY_INPUT_DEVICE
#define APP_KEY_INPUT_DEVICE ""
#endif

namespace platform {
namespace {

std::array<lv_obj_t*, kNavKeyCount> nav_buttons{};
uint32_t last_key = 0;
bool last_key_pressed = false;
bool nav_shortcut_mode = false;
uint32_t pressed_key = 0;
uint32_t press_started_at = 0;
bool long_press_sent = false;
bool pressed_key_consumed = false;
lv_timer_t* long_press_timer = nullptr;
KeyReleaseListener key_release_listener = nullptr;
void* key_release_listener_user_data = nullptr;
GlobalKeyListener global_key_listener = nullptr;
void* global_key_listener_user_data = nullptr;

constexpr uint32_t kLongPressMs = 700;

#if !USE_DESKTOP
struct EvdevKeypad {
    int fd{-1};
    lv_indev_state_t state{LV_INDEV_STATE_RELEASED};
    uint32_t key{0};
    bool router_event_pending{false};
    uint32_t router_key{0};
    bool router_pressed{false};
};
#endif

size_t nav_key_to_index(uint32_t key) {
    if (nav_shortcut_mode) {
        switch (key) {
            case LV_KEY_ESC:
                return 4;
            case 'z':
            case 'Z':
            case LV_KEY_LEFT:
                return 1;
            case 'c':
            case 'C':
            case LV_KEY_RIGHT:
                return 3;
            default:
                return kNavKeyCount;
        }
    }

    switch (key) {
        case '4':
        case LV_KEY_ESC:
            return 0;
        case '5':
            return 1;
        case '6':
            return 2;
        case '7':
            return 3;
        case '8':
            return 4;
        default:
            return kNavKeyCount;
    }
}

void dispatch_nav_key(uint32_t key) {
    const auto index = nav_key_to_index(key);
    if (index >= nav_buttons.size()) {
        return;
    }

    auto* button = nav_buttons[index];
    if (!button || !lv_obj_is_valid(button) || !lv_obj_has_flag(button, LV_OBJ_FLAG_CLICKABLE)) {
        return;
    }

    lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
}

bool emit_global_key(uint32_t key, bool long_pressed) {
    return global_key_listener &&
           global_key_listener(key, describe_key(key), long_pressed, global_key_listener_user_data);
}

void long_press_timer_cb(lv_timer_t*) {
    if (!pressed_key || long_press_sent || lv_tick_elaps(press_started_at) < kLongPressMs) {
        return;
    }

    long_press_sent = true;
    if (emit_global_key(pressed_key, true)) {
        pressed_key_consumed = true;
    }
}

void ensure_long_press_timer() {
    if (!long_press_timer) {
        long_press_timer = lv_timer_create(long_press_timer_cb, 40, nullptr);
    }
    if (long_press_timer) {
        lv_timer_resume(long_press_timer);
        lv_timer_reset(long_press_timer);
    }
}

void route_key_state(uint32_t key, bool pressed) {
    const bool new_hold = pressed && (!last_key_pressed || pressed_key != key);
    if (new_hold) {
        pressed_key = key;
        press_started_at = lv_tick_get();
        long_press_sent = false;
        pressed_key_consumed = emit_global_key(key, false);
        ensure_long_press_timer();
        if (!pressed_key_consumed) {
            dispatch_nav_key(key);
        }
    }
    else if (!pressed && last_key_pressed && pressed_key == key) {
        if (key_release_listener) {
            key_release_listener(key, describe_key(key), key_release_listener_user_data);
        }
        pressed_key = 0;
        pressed_key_consumed = false;
        if (long_press_timer) {
            lv_timer_pause(long_press_timer);
        }
    }

    last_key = key;
    last_key_pressed = pressed;
}

void key_event_cb(lv_event_t* event) {
    LV_UNUSED(event);

    auto* indev = lv_indev_active();
    if (!indev) {
        return;
    }

#if !USE_DESKTOP
    auto* keypad = static_cast<EvdevKeypad*>(lv_indev_get_driver_data(indev));
    if (keypad) {
        // LVGL emits LV_EVENT_KEY on every poll, even when evdev had no new event.
        if (!keypad->router_event_pending) {
            return;
        }

        keypad->router_event_pending = false;
        const auto key = keypad->router_key;
        const bool pressed = keypad->router_pressed;

        route_key_state(key, pressed);
        return;
    }
#endif

    const auto key = lv_indev_get_key(indev);
    const bool pressed = lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED;

    route_key_state(key, pressed);
}

#if !USE_DESKTOP
uint32_t map_evdev_key(uint16_t code) {
    switch (code) {
        case KEY_ESC:
            return LV_KEY_ESC;
        case KEY_LEFT:
            return LV_KEY_LEFT;
        case KEY_RIGHT:
            return LV_KEY_RIGHT;
        case KEY_UP:
            return LV_KEY_UP;
        case KEY_DOWN:
            return LV_KEY_DOWN;

        // Letters
        case KEY_A: return 'a';
        case KEY_B: return 'b';
        case KEY_C: return 'c';
        case KEY_D: return 'd';
        case KEY_E: return 'e';
        case KEY_F: return 'f';
        case KEY_G: return 'g';
        case KEY_H: return 'h';
        case KEY_I: return 'i';
        case KEY_J: return 'j';
        case KEY_K: return 'k';
        case KEY_L: return 'l';
        case KEY_M: return 'm';
        case KEY_N: return 'n';
        case KEY_O: return 'o';
        case KEY_P: return 'p';
        case KEY_Q: return 'q';
        case KEY_R: return 'r';
        case KEY_S: return 's';
        case KEY_T: return 't';
        case KEY_U: return 'u';
        case KEY_V: return 'v';
        case KEY_W: return 'w';
        case KEY_X: return 'x';
        case KEY_Y: return 'y';
        case KEY_Z: return 'z';

        // Digits
        case KEY_0: return '0';
        case KEY_1: return '1';
        case KEY_2: return '2';
        case KEY_3: return '3';
        case KEY_4: return '4';
        case KEY_5: return '5';
        case KEY_6: return '6';
        case KEY_7: return '7';
        case KEY_8: return '8';
        case KEY_9: return '9';

        // Common punctuation / editing keys on the Cardputer keyboard
        case KEY_SPACE:      return ' ';
        case KEY_ENTER:      return LV_KEY_ENTER;
        case KEY_BACKSPACE:  return LV_KEY_BACKSPACE;
        case KEY_TAB:        return LV_KEY_NEXT;
        case KEY_COMMA:      return ',';
        case KEY_DOT:        return '.';
        case KEY_SLASH:      return '/';
        case KEY_SEMICOLON:  return ';';
        case KEY_APOSTROPHE: return '\'';
        case KEY_MINUS:      return '-';
        case KEY_EQUAL:      return '=';
        case KEY_LEFTBRACE:  return '[';
        case KEY_RIGHTBRACE: return ']';
        case KEY_BACKSLASH:  return '\\';
        case KEY_GRAVE:      return '`';

        case KEY_SYSRQ:
            return kKeyPrintScreen;
        case KEY_HELP:
            return kKeyHelp;
        default:
            return 0;
    }
}

bool has_nav_keys(int fd) {
    unsigned long key_bits[(KEY_MAX / (sizeof(unsigned long) * 8)) + 1] = {};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        return false;
    }

    auto has_key = [&](int code) {
        const auto bits_per_word = static_cast<int>(sizeof(unsigned long) * 8);
        return (key_bits[code / bits_per_word] & (1UL << (code % bits_per_word))) != 0;
    };

    return has_key(KEY_ESC) || has_key(KEY_LEFT) || has_key(KEY_RIGHT) || has_key(KEY_Z) ||
           has_key(KEY_C) || has_key(KEY_4) || has_key(KEY_5) || has_key(KEY_6) ||
           has_key(KEY_7) || has_key(KEY_8) || has_key(KEY_SYSRQ) || has_key(KEY_HELP);
}

void evdev_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* keypad = static_cast<EvdevKeypad*>(lv_indev_get_driver_data(indev));
    data->continue_reading = false;
    if (!keypad) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    input_event input{};
    while (read(keypad->fd, &input, sizeof(input)) == sizeof(input)) {
        if (input.type != EV_KEY) {
            continue;
        }

        const auto key = map_evdev_key(input.code);
        if (!key || input.value == 2) {
            continue;
        }

        keypad->key = key;
        keypad->state = input.value ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        keypad->router_event_pending = true;
        keypad->router_key = key;
        keypad->router_pressed = keypad->state == LV_INDEV_STATE_PRESSED;
        data->continue_reading = true;
        break;
    }

    data->key = keypad->key;
    data->state = keypad->state;
}

void evdev_delete_cb(lv_event_t* event) {
    auto* indev = static_cast<lv_indev_t*>(lv_event_get_target(event));
    auto* keypad = static_cast<EvdevKeypad*>(lv_indev_get_driver_data(indev));
    if (!keypad) {
        return;
    }

    if (keypad->fd >= 0) {
        close(keypad->fd);
    }
    delete keypad;
}

lv_indev_t* create_keypad_from_fd(int fd) {
    auto* keypad = new EvdevKeypad;
    keypad->fd = fd;

    auto* indev = lv_indev_create();
    if (!indev) {
        delete keypad;
        close(fd);
        return nullptr;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, evdev_read_cb);
    lv_indev_set_driver_data(indev, keypad);
    lv_indev_add_event_cb(indev, evdev_delete_cb, LV_EVENT_DELETE, nullptr);
    attach_key_router(indev);
    return indev;
}

lv_indev_t* try_create_keypad(const char* path) {
    if (!path || path[0] == '\0') {
        return nullptr;
    }

    const int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        LV_LOG_WARN("failed to open input device %s: %s", path, strerror(errno));
        return nullptr;
    }

    if (!has_nav_keys(fd)) {
        close(fd);
        return nullptr;
    }

    LV_LOG_INFO("using evdev key input %s", path);
    return create_keypad_from_fd(fd);
}

void discover_keypads(lv_display_t* display) {
    const char* configured_device = APP_KEY_INPUT_DEVICE;
    if (configured_device[0] != '\0') {
        auto* indev = try_create_keypad(configured_device);
        if (indev) {
            lv_indev_set_display(indev, display);
        }
        return;
    }

    auto* dir = opendir("/dev/input");
    if (!dir) {
        LV_LOG_WARN("failed to open /dev/input: %s", strerror(errno));
        return;
    }

    while (auto* entry = readdir(dir)) {
        if (std::strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }

        std::string path = "/dev/input/";
        path += entry->d_name;
        auto* indev = try_create_keypad(path.c_str());
        if (indev) {
            lv_indev_set_display(indev, display);
        }
    }

    closedir(dir);
}
#endif

} // namespace

void init_key_input(lv_display_t* display) {
#if !USE_DESKTOP
    discover_keypads(display);
#else
    LV_UNUSED(display);
#endif
}

void attach_key_router(lv_indev_t* indev) {
    if (!indev || lv_indev_get_type(indev) != LV_INDEV_TYPE_KEYPAD) {
        return;
    }

    lv_indev_add_event_cb(indev, key_event_cb, LV_EVENT_KEY, nullptr);
}

void set_nav_shortcut_mode(bool enabled) {
    nav_shortcut_mode = enabled;
}

void register_nav_button(size_t index, lv_obj_t* button) {
    if (index >= nav_buttons.size()) {
        return;
    }

    nav_buttons[index] = button;
}

void unregister_nav_button(size_t index, lv_obj_t* button) {
    if (index >= nav_buttons.size()) {
        return;
    }

    if (!button || nav_buttons[index] == button) {
        nav_buttons[index] = nullptr;
    }
}

void set_key_release_listener(KeyReleaseListener listener, void* user_data) {
    key_release_listener = listener;
    key_release_listener_user_data = user_data;
}

void clear_key_release_listener(KeyReleaseListener listener, void* user_data) {
    if (key_release_listener == listener && key_release_listener_user_data == user_data) {
        key_release_listener = nullptr;
        key_release_listener_user_data = nullptr;
    }
}

void set_global_key_listener(GlobalKeyListener listener, void* user_data) {
    global_key_listener = listener;
    global_key_listener_user_data = user_data;
}

void clear_global_key_listener(GlobalKeyListener listener, void* user_data) {
    if (global_key_listener == listener && global_key_listener_user_data == user_data) {
        global_key_listener = nullptr;
        global_key_listener_user_data = nullptr;
    }
}

const char* describe_key(uint32_t key) {
    switch (key) {
        case LV_KEY_ESC:
            return "Esc";
        case LV_KEY_LEFT:
            return "Left";
        case LV_KEY_RIGHT:
            return "Right";
        case kKeyPrintScreen:
            return "PrintScreen";
        case kKeyHelp:
            return "Help";
        default:
            break;
    }

    static char buffer[24];
    if (key >= 32 && key <= 126) {
        std::snprintf(buffer, sizeof(buffer), "%c", static_cast<char>(key));
    }
    else {
        std::snprintf(buffer, sizeof(buffer), "Key_%u", key);
    }
    return buffer;
}

} // namespace platform
