#pragma once

#include <string>
#include <utility>
#include <vector>

struct Monitor {
  int id;
  int width;
  int height;
};

namespace hyprland::command {

std::vector<Monitor> get_monitors(const std::string &sock_path);
std::pair<int, int> get_mouse_position(const std::string &sock_path);
std::string hide_window(const std::string &title, const std::string &sock_path);
std::string move_window_to_workspace(const std::string &title,
                                     const std::string &workspace,
                                     const std::string &sock_path);
void move_mouse(const std::pair<int, int> mouse_pos,
                const std::string &sock_path);
std::string get_active_workspace(const std::string &sock_path);
bool is_empty_workspace(const std::string &uuid, const std::string &workspace,
                        const std::string &sock_path);
std::string get_window_from_proc(const std::string &proc,
                                 const std::string &sock_path);
void focus_window(const std::string &address, const std::string &sock_path);
void set_plain_window(const std::string &uuid, const std::string &sock_path);
void set_unmoveable_window(const std::string &uuid,
                           const std::string &sock_path);

} // namespace hyprland::command
