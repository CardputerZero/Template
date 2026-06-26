/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "navbar.h"

#include "asset_manager.h"
#include "bindings.h"
#include "linux_input.h"
#include "theme.h"
#include "ui_const.h"

namespace view::widgets {
namespace {

struct IconSpec {
    const char* text;
    lv_event_cb_t click_cb;
};

constexpr std::array<int32_t, 5> kNavButtonXOffsets = {36, 17, 2, -17, -36};

} // namespace

NavBar::NavBar(lv_obj_t* parent, viewmodel::BaseViewModel& view_model, app::AssetManager& assets)
    : BaseWidgets(parent), view_model_(view_model), assets_(assets) {}

NavBar::~NavBar() {
    for (size_t i = 0; i < icon_buttons_.size(); ++i) {
        if (icon_buttons_[i]) {
            platform::unregister_nav_button(i, icon_buttons_[i]->root());
        }
    }

    if (page_observer_) {
        lv_observer_remove(page_observer_);
        page_observer_ = nullptr;
    }
    if (theme_observer_) {
        lv_observer_remove(theme_observer_);
        theme_observer_ = nullptr;
    }
}

void NavBar::build() {
    if (core_obj_) {
        return;
    }

    core_obj_ = lv_obj_create(parent_);
    lv_obj_remove_style_all(core_obj_);
    lv_obj_set_size(core_obj_, LV_PCT(100), kNavBarHeight);
    lv_obj_align(core_obj_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(core_obj_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(core_obj_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(core_obj_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(core_obj_, 8, 0);
    lv_obj_set_style_pad_right(core_obj_, 8, 0);
    reactive::bind_theme(core_obj_, view_model_.dark_mode_subject(), reactive::ThemeRole::Bar);

    create_icon_buttons();
    page_observer_ = reactive::observe_obj(core_obj_, view_model_.current_page_subject(), update_icons_cb, this);
    theme_observer_ = reactive::observe_obj(core_obj_, view_model_.dark_mode_subject(), update_icons_cb, this);
    update_icon_buttons();
}

void NavBar::create_icon_buttons() {
    const auto light_color = view::palette(false).text;
    const auto dark_color = view::palette(true).text;
    icon_font_ = assets_.load_font("Phosphor-Fill.ttf", 22);

    for (size_t i = 0; i < icon_buttons_.size(); ++i) {
        icon_buttons_[i] = std::make_unique<IconButton>(core_obj_,
                                                        view_model_,
                                                        32,
                                                        24,
                                                        "",
                                                        icon_font_ ? icon_font_ : &lv_font_montserrat_14,
                                                        light_color,
                                                        dark_color,
                                                        nullptr,
                                                        &view_model_,
                                                        true);
        icon_buttons_[i]->build();
        lv_obj_set_style_translate_x(icon_buttons_[i]->root(), kNavButtonXOffsets[i], 0);
        platform::register_nav_button(i, icon_buttons_[i]->root());
    }
}

void NavBar::update_icon_buttons() {
    const bool is_butter_page = view_model_.current_page() == model::AppPage::Butter;
    const bool is_dark_mode = view_model_.is_dark_mode();
    const char* theme_icon = is_dark_mode ? ICON_SUN : ICON_MOON;
    /* nav bar icons showed on apple screen */
    const std::array<IconSpec, 5> apple_icons = {{
        {ICON_SIGN_OUT, request_quit_cb},
        {ICON_TEXT_BOLD, toggle_bold_text_cb},
        {theme_icon, toggle_theme_cb},
        {ICON_INFO, show_info_cb},
        {ICON_SQUARE_ARROW_RIGHT, toggle_page_cb},
    }};
    /* nav bar icons showed on butter screen */
    const std::array<IconSpec, 5> butter_icons = {{
        {ICON_SIGN_OUT, request_quit_cb},
        {ICON_MINUS, decrement_counter_cb},
        {theme_icon, toggle_theme_cb},
        {ICON_PLUS, increment_counter_cb},
        {ICON_SQUARE_ARROW_LEFT, toggle_page_cb},
    }};

    const auto& icons = is_butter_page ? butter_icons : apple_icons;
    for (size_t i = 0; i < icon_buttons_.size(); ++i) {
        auto& button = icon_buttons_[i];
        if (!button) {
            continue;
        }

        button->set_text(icons[i].text);
        if (button->root()) {
            lv_obj_remove_flag(button->root(), LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_event_cb(button->root(), toggle_theme_cb);
            lv_obj_remove_event_cb(button->root(), toggle_page_cb);
            lv_obj_remove_event_cb(button->root(), toggle_bold_text_cb);
            lv_obj_remove_event_cb(button->root(), increment_counter_cb);
            lv_obj_remove_event_cb(button->root(), decrement_counter_cb);
            lv_obj_remove_event_cb(button->root(), show_info_cb);
            lv_obj_remove_event_cb(button->root(), request_quit_cb);
            if (icons[i].click_cb) {
                lv_obj_add_event_cb(button->root(), icons[i].click_cb, LV_EVENT_CLICKED, &view_model_);
                button->set_enabled(true);
            }
            else {
                button->set_enabled(false);
            }
        }
    }
}

void NavBar::toggle_theme_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->toggle_dark_mode();
    }
}

void NavBar::toggle_page_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->toggle_page();
    }
}

void NavBar::toggle_bold_text_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->toggle_bold_text();
    }
}

void NavBar::increment_counter_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->increment_counter();
    }
}

void NavBar::decrement_counter_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->decrement_counter();
    }
}

void NavBar::show_info_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->toggle_info();
    }
}

void NavBar::request_quit_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->request_quit();
    }
}

void NavBar::update_icons_cb(lv_observer_t* observer, lv_subject_t* subject) {
    LV_UNUSED(subject);

    auto* nav_bar = static_cast<NavBar*>(lv_observer_get_user_data(observer));
    if (nav_bar) {
        nav_bar->update_icon_buttons();
    }
}

} // namespace view::widgets
