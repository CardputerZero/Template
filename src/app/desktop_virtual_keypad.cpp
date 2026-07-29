/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "desktop_virtual_keypad.h"

#if USE_DESKTOP

#include "logger.h"
#include "linux_input.h"

#include <SDL_keycode.h>
#include <SDL_render.h>

#include <array>
#include <cstddef>
#include <string>

namespace app {
namespace {

enum class KeyKind : uint8_t {
    Character,
    Backspace,
    Enter,
    Escape,
    Tab,
    Shift,
    Sym,
    Fn,
    Ctrl,
    Alt,
};

struct KeySpec {
    KeyKind kind;
    char base;
    char sym;
};

constexpr int32_t kColumnCount = 11;
constexpr int32_t kRowCount    = 4;
constexpr int32_t kKeyX        = 8;
constexpr int32_t kKeyY        = 195;
constexpr int32_t kKeyWidth    = 50;
constexpr int32_t kKeyHeight   = 47;
constexpr int32_t kColumnStep  = 57;
constexpr int32_t kRowStep     = 49;

constexpr size_t kMatrixKeyCount = kColumnCount * kRowCount;
constexpr uint8_t kEscapeKeyIndex = kMatrixKeyCount;
constexpr uint8_t kTabKeyIndex    = kMatrixKeyCount + 1;

constexpr std::array<KeySpec, kMatrixKeyCount + 2> kKeys = {{
    {KeyKind::Character, '1', '!'},
    {KeyKind::Character, '2', '@'},
    {KeyKind::Character, '3', '#'},
    {KeyKind::Character, '4', '$'},
    {KeyKind::Character, '5', '%'},
    {KeyKind::Character, '6', '^'},
    {KeyKind::Character, '7', '&'},
    {KeyKind::Character, '8', '*'},
    {KeyKind::Character, '9', '('},
    {KeyKind::Character, '0', ')'},
    {KeyKind::Backspace, 0, 0},

    {KeyKind::Sym, 0, 0},
    {KeyKind::Character, 'q', '~'},
    {KeyKind::Character, 'w', '`'},
    {KeyKind::Character, 'e', '_'},
    {KeyKind::Character, 'r', '-'},
    {KeyKind::Character, 't', '+'},
    {KeyKind::Character, 'y', '='},
    {KeyKind::Character, 'u', '['},
    {KeyKind::Character, 'i', ']'},
    {KeyKind::Character, 'o', '{'},
    {KeyKind::Character, 'p', '}'},

    {KeyKind::Shift, 0, 0},
    {KeyKind::Character, 'a', ';'},
    {KeyKind::Character, 's', ':'},
    {KeyKind::Character, 'd', '\''},
    {KeyKind::Character, 'f', 0},
    {KeyKind::Character, 'g', '"'},
    {KeyKind::Character, 'h', '<'},
    {KeyKind::Character, 'j', '>'},
    {KeyKind::Character, 'k', '\\'},
    {KeyKind::Character, 'l', '|'},
    {KeyKind::Enter, 0, 0},

    {KeyKind::Fn, 0, 0},
    {KeyKind::Ctrl, 0, 0},
    {KeyKind::Alt, 0, 0},
    {KeyKind::Character, 'z', 0},
    {KeyKind::Character, 'x', 0},
    {KeyKind::Character, 'c', 0},
    {KeyKind::Character, 'v', ','},
    {KeyKind::Character, 'b', '.'},
    {KeyKind::Character, 'n', '/'},
    {KeyKind::Character, 'm', '?'},
    {KeyKind::Character, ' ', ' '},

    {KeyKind::Escape, 0, 0},
    {KeyKind::Tab, 0, 0},
}};

constexpr uint32_t kShiftColor = 0xa000a0;
constexpr uint32_t kSymColor   = 0x3976a8;
constexpr uint32_t kFnColor    = 0xff6542;
constexpr uint32_t kCtrlColor  = 0xf2c500;
constexpr uint32_t kAltColor   = 0x35c99a;

SDL_Rect key_rect(size_t index) {
    if (index < kMatrixKeyCount) {
        const auto row = static_cast<int32_t>(index) / kColumnCount;
        const auto column = static_cast<int32_t>(index) % kColumnCount;
        return {kKeyX + column * kColumnStep,
                kKeyY + row * kRowStep,
                kKeyWidth,
                kKeyHeight};
    }
    if (index == kEscapeKeyIndex) {
        return {8, 142, 67, 52};
    }
    return {575, 142, 48, 52};
}

int16_t key_at(int32_t x, int32_t y) {
    for (size_t index = 0; index < kKeys.size(); ++index) {
        const auto rect = key_rect(index);
        if (x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h) {
            return static_cast<int16_t>(index);
        }
    }
    return -1;
}

void fill_key(SDL_Renderer* renderer, size_t index, uint32_t color, uint8_t alpha) {
    const auto rect = key_rect(index);
    SDL_SetRenderDrawColor(renderer,
                           static_cast<uint8_t>(color >> 16U),
                           static_cast<uint8_t>(color >> 8U),
                           static_cast<uint8_t>(color),
                           alpha);
    SDL_RenderFillRect(renderer, &rect);
}

uint32_t fn_key_for(char key) {
    switch (key) {
        case '1':
            return SDLK_F1;
        case '2':
            return SDLK_F2;
        case '3':
            return SDLK_F3;
        case '4':
            return SDLK_F4;
        case '5':
            return SDLK_F5;
        case '6':
            return SDLK_F6;
        case '7':
            return SDLK_F7;
        case '8':
            return SDLK_F8;
        case '9':
            return SDLK_F9;
        case '0':
            return SDLK_F10;
        case 'q':
            return SDLK_AUDIOPLAY;
        case 'w':
            return SDLK_AUDIOPREV;
        case 'e':
            return SDLK_AUDIONEXT;
        case 'u':
            return SDLK_BRIGHTNESSDOWN;
        case 'i':
            return SDLK_BRIGHTNESSUP;
        case 'o':
            return SDLK_F11;
        case 'p':
            return SDLK_F12;
        case 'a':
            return SDLK_MUTE;
        case 's':
            return SDLK_VOLUMEDOWN;
        case 'd':
            return SDLK_VOLUMEUP;
        case 'f':
            return LV_KEY_UP;
        case 'h':
            return SDLK_HELP;
        case 'j':
            return SDLK_PRINTSCREEN;
        case 'k':
            return LV_KEY_HOME;
        case 'l':
            return LV_KEY_PREV;
        case 'z':
            return LV_KEY_LEFT;
        case 'x':
            return LV_KEY_DOWN;
        case 'c':
            return LV_KEY_RIGHT;
        case 'b':
            return SDLK_INSERT;
        case 'n':
            return LV_KEY_END;
        case 'm':
            return LV_KEY_NEXT;
        default:
            return 0;
    }
}

std::string key_kind_name(const KeySpec& key) {
    switch (key.kind) {
        case KeyKind::Character:
            return key.base == ' ' ? "space" : std::string(1, key.base);
        case KeyKind::Backspace:
            return "backspace";
        case KeyKind::Enter:
            return "enter";
        case KeyKind::Escape:
            return "esc";
        case KeyKind::Tab:
            return "tab";
        case KeyKind::Shift:
            return "shift";
        case KeyKind::Sym:
            return "sym";
        case KeyKind::Fn:
            return "fn";
        case KeyKind::Ctrl:
            return "ctrl";
        case KeyKind::Alt:
            return "alt";
    }
    return "unknown";
}

std::string output_key_name(uint32_t key) {
    switch (key) {
        case SDLK_F1:
            return "f1";
        case SDLK_F2:
            return "f2";
        case SDLK_F3:
            return "f3";
        case SDLK_F4:
            return "f4";
        case SDLK_F5:
            return "f5";
        case SDLK_F6:
            return "f6";
        case SDLK_F7:
            return "f7";
        case SDLK_F8:
            return "f8";
        case SDLK_F9:
            return "f9";
        case SDLK_F10:
            return "f10";
        case SDLK_F11:
            return "f11";
        case SDLK_F12:
            return "f12";
        case SDLK_AUDIOPLAY:
            return "media-play-pause";
        case SDLK_AUDIOPREV:
            return "media-previous";
        case SDLK_AUDIONEXT:
            return "media-next";
        case SDLK_MUTE:
            return "mute";
        case SDLK_VOLUMEDOWN:
            return "volume-down";
        case SDLK_VOLUMEUP:
            return "volume-up";
        case SDLK_BRIGHTNESSDOWN:
            return "brightness-down";
        case SDLK_BRIGHTNESSUP:
            return "brightness-up";
        case SDLK_HELP:
            return "help";
        case SDLK_PRINTSCREEN:
            return "print-screen";
        case SDLK_INSERT:
            return "insert";
        case LV_KEY_UP:
            return "up";
        case LV_KEY_DOWN:
            return "down";
        case LV_KEY_LEFT:
            return "left";
        case LV_KEY_RIGHT:
            return "right";
        case LV_KEY_ESC:
            return "esc";
        case LV_KEY_DEL:
            return "delete";
        case LV_KEY_BACKSPACE:
            return "backspace";
        case LV_KEY_ENTER:
            return "enter";
        case LV_KEY_HOME:
            return "home";
        case LV_KEY_END:
            return "end";
        case LV_KEY_PREV:
            return "prev";
        case LV_KEY_NEXT:
            return "next";
        default:
            break;
    }

    if (key >= 32 && key <= 126) {
        return std::string{"'"} + static_cast<char>(key) + "'";
    }
    return "control";
}

} // namespace

DesktopVirtualKeypad::DesktopVirtualKeypad(lv_display_t* display) {
    if (!display) {
        return;
    }

    indev_ = lv_indev_create();
    lv_indev_set_type(indev_, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_, read_cb);
    lv_indev_set_driver_data(indev_, this);
    lv_indev_set_mode(indev_, LV_INDEV_MODE_EVENT);
    lv_indev_set_display(indev_, display);
    platform::attach_key_router(indev_);

    previous_default_group_ = lv_group_get_default();
    group_ = lv_group_create();
    lv_indev_set_group(indev_, group_);
    lv_group_set_default(group_);
}

DesktopVirtualKeypad::~DesktopVirtualKeypad() {
    if (indev_) {
        lv_indev_delete(indev_);
        indev_ = nullptr;
    }
    if (group_) {
        if (lv_group_get_default() == group_) {
            lv_group_set_default(previous_default_group_);
        }
        lv_group_delete(group_);
        group_ = nullptr;
    }
}

void DesktopVirtualKeypad::read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* keypad = static_cast<DesktopVirtualKeypad*>(lv_indev_get_driver_data(indev));
    if (!keypad || keypad->pending_key_ == 0) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->key = keypad->pending_key_;
    data->state = keypad->pending_release_ ? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
}

bool DesktopVirtualKeypad::handle_pointer(int32_t x, int32_t y, bool pressed) {
    const auto hit = key_at(x, y);
    if (pressed) {
        pressed_key_ = hit;
        return hit >= 0;
    }

    const auto activated = pressed_key_ >= 0 && pressed_key_ == hit;
    pressed_key_ = -1;
    if (activated) {
        handle_key(static_cast<uint8_t>(hit));
    }
    return activated || hit >= 0;
}

void DesktopVirtualKeypad::handle_key(uint8_t key_index) {
    if (key_index >= kKeys.size()) {
        return;
    }

    const auto& key = kKeys[key_index];
    const auto mode_name = [this]() {
        switch (mode_) {
            case Mode::Base:
                return "base";
            case Mode::Shift:
                return "shift";
            case Mode::Sym:
                return "sym";
        }
        return "unknown";
    };
    const auto log_modifiers = [this, &mode_name]() {
        LOG_INFO("desktop keypad modifier: mode={} fn={} ctrl={} alt={}",
                 mode_name(),
                 fn_active_,
                 ctrl_active_,
                 alt_active_);
    };
    const auto emit_recognized = [this, &key, &mode_name](uint32_t output) {
        auto output_name = output_key_name(output);
        if (fn_active_ && key.kind == KeyKind::Character) {
            if (key.base == 'l') {
                output_name = "page-up";
            }
            else if (key.base == 'm') {
                output_name = "page-down";
            }
        }
        LOG_INFO("desktop keypad: input={} mode={} fn={} ctrl={} alt={} -> {} ({})",
                 key_kind_name(key),
                 mode_name(),
                 fn_active_,
                 ctrl_active_,
                 alt_active_,
                 output_name,
                 output);
        emit_key(output);
    };

    switch (key.kind) {
        case KeyKind::Shift:
            set_mode(mode_ == Mode::Shift ? Mode::Base : Mode::Shift);
            log_modifiers();
            return;
        case KeyKind::Sym:
            set_mode(mode_ == Mode::Sym ? Mode::Base : Mode::Sym);
            log_modifiers();
            return;
        case KeyKind::Fn:
            fn_active_ = !fn_active_;
            log_modifiers();
            return;
        case KeyKind::Ctrl:
            ctrl_active_ = !ctrl_active_;
            log_modifiers();
            return;
        case KeyKind::Alt:
            alt_active_ = !alt_active_;
            log_modifiers();
            return;
        case KeyKind::Backspace:
            emit_recognized(fn_active_ ? LV_KEY_DEL : LV_KEY_BACKSPACE);
            clear_one_shot_modifiers();
            return;
        case KeyKind::Enter:
            emit_recognized(LV_KEY_ENTER);
            clear_one_shot_modifiers();
            return;
        case KeyKind::Escape:
            emit_recognized(LV_KEY_ESC);
            clear_one_shot_modifiers();
            return;
        case KeyKind::Tab:
            emit_recognized(mode_ == Mode::Shift ? LV_KEY_PREV : LV_KEY_NEXT);
            clear_one_shot_modifiers();
            return;
        case KeyKind::Character:
            break;
    }

    uint32_t output = static_cast<uint8_t>(key.base);
    if (fn_active_) {
        output = fn_key_for(key.base);
    }
    else if (mode_ == Mode::Sym) {
        output = static_cast<uint8_t>(key.sym);
    }
    else if (mode_ == Mode::Shift && key.base >= 'a' && key.base <= 'z') {
        output = static_cast<uint8_t>(key.base - 'a' + 'A');
    }

    if (output != 0) {
        if (ctrl_active_ && ((output >= 'a' && output <= 'z') || (output >= 'A' && output <= 'Z'))) {
            output = (output | 0x20U) - 'a' + 1U;
        }
        emit_recognized(output);
    }
    else {
        LOG_WARN("desktop keypad: input={} mode={} fn={} ctrl={} alt={} -> unmapped",
                 key_kind_name(key),
                 mode_name(),
                 fn_active_,
                 ctrl_active_,
                 alt_active_);
    }
    clear_one_shot_modifiers();
}

void DesktopVirtualKeypad::emit_key(uint32_t key) {
    if (!indev_ || key == 0) {
        return;
    }

    pending_key_ = key;
    pending_release_ = false;
    lv_indev_read(indev_);
    pending_release_ = true;
    lv_indev_read(indev_);
    pending_key_ = 0;
    pending_release_ = false;
}

void DesktopVirtualKeypad::set_mode(Mode mode) {
    mode_ = mode;
}

void DesktopVirtualKeypad::clear_one_shot_modifiers() {
    if (mode_ != Mode::Base) {
        mode_ = Mode::Base;
    }
    fn_active_ = false;
    ctrl_active_ = false;
    alt_active_ = false;
}

void DesktopVirtualKeypad::render(SDL_Renderer* renderer) const {
    if (!renderer) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (mode_ == Mode::Shift) fill_key(renderer, 22, kShiftColor, 77);
    if (mode_ == Mode::Sym) fill_key(renderer, 11, kSymColor, 77);
    if (fn_active_) fill_key(renderer, 33, kFnColor, 77);
    if (ctrl_active_) fill_key(renderer, 34, kCtrlColor, 77);
    if (alt_active_) fill_key(renderer, 35, kAltColor, 77);
    if (pressed_key_ >= 0) fill_key(renderer, static_cast<size_t>(pressed_key_), 0xffffff, 51);
}

} // namespace app

#endif
