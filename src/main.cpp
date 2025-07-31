#include <algorithm>
#include <chrono>
#include <raylib.h>
#include <string>
#include <unistd.h>

#include "commands.hpp"
#include "hyprdock.hpp"
#include "utils.hpp"

int main(void) {
  hyprdock::State state;
  if (!state)
    return 1;

  const int unknown_width = MeasureText("?", 20);
  const int font_size = state.config.app_size / 2;

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_Q) &&
        (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
      break;

    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - state.last_command_time);

    if (elapsed_time >= state.command_interval) {
      state.last_command_time = current_time;
      state.mouse_pos = hyprland::command::get_mouse_position(state.sock_path);
      state.active_workspace =
          hyprland::command::get_active_workspace(state.sock_path);

      if (state.is_valid_mouse_pos()) {
        if (state.is_hovering()) {
          if (state.should_wait()) {
            state.start_waiting();
          } else if (state.is_waiting()) {
            if (state.mouse_pos == state.wait_mouse_pos) {
              if (std::chrono::duration_cast<std::chrono::milliseconds>(
                      current_time - state.start_wait_time) >=
                  state.wait_interval) {
                hyprland::command::move_window_to_workspace(
                    state.uuid, state.active_workspace, state.sock_path);
                hyprland::command::move_mouse(state.mouse_pos, state.sock_path);
                state.is_minimized = false;
                state.waiting = false;
              }
            } else {
              state.start_waiting();
            }
          }
        } else if (!state.is_minimized) {
          hyprland::command::hide_window(state.uuid, state.sock_path);
          state.is_minimized = true;
          // Deselect app if the window gets hidden
          state.clicked_app = -1;
        }
      }
    }

    if (state.is_minimized) {
      if (state.first_frame)
        state.first_frame = false;
      else
        continue;
    }

    BeginDrawing();
    ClearBackground(state.config.dock_color);

    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    int cursor = state.config.dock_padding;
    for (int i = 0; i < state.config.applications.size(); i++) {
      const auto &app = state.config.applications[i];
      const auto &icon = state.app_icons[app.name];

      Rectangle overlay_rect{
          static_cast<float>(cursor),
          static_cast<float>(state.config.dock_padding),
          static_cast<float>(state.config.app_size),
          static_cast<float>(state.config.app_size),
      };

      Rectangle icon_rect{
          overlay_rect.x + 2,
          overlay_rect.y + 2,
          overlay_rect.width - 4,
          overlay_rect.height - 4,
      };

      // Draw hover/click overlay
      Vector2 win_mouse_pos = GetMousePosition();
      if (CheckCollisionPointRec(win_mouse_pos, overlay_rect)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (state.animations[i] < 50)
          state.animations[i] += 3;

        Color overlay{180, 180, 180, state.animations[i]};
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
          overlay = Color{150, 150, 150, state.animations[i]};

        DrawRectangleRounded(overlay_rect, 0.1, 0, overlay);

        // Launch app only if start click is is on the app and release is on the
        // app as well
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          state.clicked_app = i;
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          if (state.clicked_app == i) {
            // If the app is running focus it else run new process
            std::string window = hyprland::command::get_window_from_proc(
                app.exec, state.sock_path);
            if (window.empty()) {
              hyprdock::run_app(app);
            } else {
              hyprland::command::focus_window(window, state.sock_path);
              hyprland::command::move_window_to_workspace(state.uuid, "",
                                                          state.sock_path);
              hyprland::command::move_mouse(state.mouse_pos, state.sock_path);
            }
          }
          state.clicked_app = -1;
        }
      } else if (state.animations[i] > 0) {
        state.animations[i] = std::clamp(state.animations[i] - 5, 0, 50);
        DrawRectangleRounded(overlay_rect, 0.1, 0,
                             Color{180, 180, 180, state.animations[i]});
      }

      // Draw app icon
      if (icon.id == 0) {
        DrawText("?",
                 overlay_rect.x + (state.config.app_size - unknown_width) / 2,
                 overlay_rect.y + (state.config.app_size - font_size) / 2,
                 font_size, LIGHTGRAY);
      } else {
        DrawTexturePro(icon,
                       Rectangle{0.0f, 0.0f, static_cast<float>(icon.width),
                                 static_cast<float>(icon.height)},
                       icon_rect, Vector2{0, 0}, 0.0f, WHITE);
      }

      // Draw active dot
      if (elapsed_time >= state.command_interval) {
        if (!hyprland::command::get_window_from_proc(app.exec, state.sock_path)
                 .empty()) {
          DrawCircle(overlay_rect.x + overlay_rect.width / 2,
                     overlay_rect.y + overlay_rect.height +
                         state.config.dock_padding - 5,
                     3, Color{0, 182, 255, 255});
          state.active_cache[i] = true;
        } else {
          state.active_cache[i] = false;
        }
      } else {
        if (state.active_cache[i])
          DrawCircle(overlay_rect.x + overlay_rect.width / 2,
                     overlay_rect.y + overlay_rect.height +
                         state.config.dock_padding - 5,
                     3, Color{0, 182, 255, 255});
      }

      // Move draw cursor
      cursor += state.config.app_size + state.config.app_padding;
    }

    EndDrawing();
  }

  state.unload();
  CloseWindow();
}
