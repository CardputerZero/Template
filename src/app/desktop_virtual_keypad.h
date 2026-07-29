/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if USE_DESKTOP

#include "lvgl.h"

#include <cstdint>

struct SDL_Renderer;

namespace app {

class DesktopVirtualKeypad {
public:
    explicit DesktopVirtualKeypad(lv_display_t* display);
    ~DesktopVirtualKeypad();

    DesktopVirtualKeypad(const DesktopVirtualKeypad&) = delete;
    DesktopVirtualKeypad& operator=(const DesktopVirtualKeypad&) = delete;

    bool handle_pointer(int32_t x, int32_t y, bool pressed);
    void emit_key(uint32_t key);
    void render(SDL_Renderer* renderer) const;

private:
    enum class Mode : uint8_t {
        Base,
        Shift,
        Sym,
    };

    static void read_cb(lv_indev_t* indev, lv_indev_data_t* data);
    void handle_key(uint8_t key_index);
    void set_mode(Mode mode);
    void clear_one_shot_modifiers();

    lv_indev_t* indev_{nullptr};
    lv_group_t* group_{nullptr};
    lv_group_t* previous_default_group_{nullptr};
    uint32_t pending_key_{0};
    bool pending_release_{false};
    bool fn_active_{false};
    bool ctrl_active_{false};
    bool alt_active_{false};
    int16_t pressed_key_{-1};
    Mode mode_{Mode::Base};
};

} // namespace app

#endif
