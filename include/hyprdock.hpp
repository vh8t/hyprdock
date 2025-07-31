#pragma once

#include <chrono>
#include <raylib.h>
#include <unordered_map>
#include <utility>
#include <vector>

#include "commands.hpp"
#include "config.hpp"

namespace hyprdock {

struct State {
  Config config;
  Monitor monitor;

  int dock_width;
  int dock_height;
  int window_x;
  int window_y;
  int fps;

  int clicked_app = -1;

  Rectangle hover_area;

  std::string active_workspace;
  std::string sock_path;
  std::string uuid;

  std::vector<unsigned char> animations;
  std::vector<bool> active_cache;
  std::unordered_map<std::string, Texture2D> app_icons;

  std::chrono::time_point<std::chrono::steady_clock> last_command_time;
  std::chrono::time_point<std::chrono::steady_clock> start_wait_time;

  std::pair<int, int> wait_mouse_pos;
  std::pair<int, int> mouse_pos;

  bool is_minimized;
  bool first_frame;
  bool waiting;

  const std::chrono::milliseconds command_interval{100};
  std::chrono::milliseconds wait_interval;

  bool error = false;

  State(void);

  void unload(void);

  inline bool is_valid_mouse_pos(void) {
    return this->mouse_pos.first >= 0 && this->mouse_pos.second >= 0;
  }

  inline bool is_hovering(void) {
    return CheckCollisionPointRec(
        Vector2{static_cast<float>(this->mouse_pos.first),
                static_cast<float>(this->mouse_pos.second)},
        this->hover_area);
  }

  inline bool should_wait(void) {
    return this->is_minimized && !this->waiting;
  }

  inline bool is_waiting(void) {
    return this->is_minimized && this->waiting;
  }

  inline void start_waiting(void) {
    this->start_wait_time = std::chrono::steady_clock::now();
    this->wait_mouse_pos = this->mouse_pos;
    this->waiting = true;
  }

  operator bool() {
    return !error;
  }
};

} // namespace hyprdock
