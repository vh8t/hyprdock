#include <chrono>
#include <iostream>
#include <print>
#include <raylib.h>
#include <utility>
#include <vector>

#include "commands.hpp"
#include "config.hpp"
#include "hyprdock.hpp"
#include "ipc.hpp"
#include "utils.hpp"

namespace hyprdock {

State::State(void) {
  this->config = hyprdock::load_config();

  auto sock_path = hyprland::IPC::get_socket_path();
  if (!sock_path) {
    std::println(std::cerr, "[ERROR] Failed to get socket path");
    this->error = true;
    return;
  }

  this->sock_path = *sock_path;

  auto monitors = hyprland::command::get_monitors(*sock_path);
  if (monitors.empty()) {
    std::println(std::cerr, "[ERROR] No monitors found");
    this->error = true;
    return;
  }

  auto it =
      std::find_if(monitors.begin(), monitors.end(), [this](const Monitor &m) {
        return m.id == this->config.monitor;
      });
  if (it == monitors.end()) {
    std::println(std::cerr, "[ERROR] No main monitor found");
    this->error = true;
    return;
  }

  this->monitor = *it;

  this->active_workspace = hyprland::command::get_active_workspace(*sock_path);

  this->dock_width =
      this->config.app_size * this->config.applications.size() +
      this->config.app_padding * (this->config.applications.size() > 0
                                      ? this->config.applications.size() - 1
                                      : 0) +
      this->config.dock_padding * 2;
  this->dock_height =
      this->config.app_size + this->config.dock_padding * 2 + 10;
  this->window_x = (this->monitor.width - this->dock_width) / 2;
  this->window_y =
      this->monitor.height - this->dock_height - this->config.dock_margin;
  this->fps = 30;

  this->hover_area = Rectangle{
      .x = static_cast<float>(this->window_x),
      .y = static_cast<float>(this->window_y),
      .width = static_cast<float>(this->dock_width),
      .height =
          static_cast<float>(this->dock_height + this->config.dock_margin),
  };

  this->uuid = "hyprdock-" + hyprdock::generate_id();
  this->animations.resize(this->config.applications.size(), 0);
  this->active_cache.resize(this->config.applications.size(), false);

  this->last_command_time = std::chrono::steady_clock::now();
  this->start_wait_time = std::chrono::steady_clock::now();

  this->wait_mouse_pos = hyprland::command::get_mouse_position(*sock_path);
  this->mouse_pos = this->wait_mouse_pos;

  // this->is_minimized = true;
  this->is_minimized = false;
  this->first_frame = true;
  this->waiting = false;

  this->wait_interval = std::chrono::milliseconds{this->config.wait_time};

  SetConfigFlags(FLAG_WINDOW_UNDECORATED);

  InitWindow(this->dock_width, this->dock_height, this->uuid.c_str());

  SetWindowMonitor(this->monitor.id);

  SetWindowPosition(this->window_x, this->window_y);
  // SetTargetFPS(this->fps);
  SetExitKey(0); // Disable default exit key

  for (const auto &app : this->config.applications) {
    if (this->app_icons.contains(app.name))
      continue;

    Texture2D icon = LoadTexture(app.icon.c_str());
    if (icon.id == 0)
      std::println("[WARNING] Failed to load icon for {} from {}", app.name,
                   app.icon);
    this->app_icons[app.name] = icon;
  }

  hyprland::command::set_plain_window(this->uuid, *sock_path);
  hyprland::command::set_unmoveable_window(this->uuid, *sock_path);
}

void State::unload(void) {
  for (const auto &texture : this->app_icons)
    UnloadTexture(texture.second);
}

} // namespace hyprdock
