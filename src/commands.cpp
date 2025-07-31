#include <iostream>
#include <nlohmann/json.hpp>
#include <print>
#include <string>
#include <utility>
#include <vector>

#include "commands.hpp"
#include "ipc.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace hyprland::command {

std::vector<Monitor> get_monitors(const std::string &sock_path) {
  auto raw_resp = hyprland::IPC::send_command("j/monitors", sock_path);
  if (!raw_resp) {
    std::println(std::cerr, "[ERROR] {}", raw_resp.error());
    return {};
  }

  try {
    json resp = json::parse(*raw_resp);
    std::vector<Monitor> result{};

    if (resp.is_array()) {
      for (const auto &monitor : resp) {
        if (monitor.contains("id") && monitor["id"].is_number() &&
            monitor.contains("width") && monitor["width"].is_number() &&
            monitor.contains("height") && monitor["height"].is_number()) {
          result.push_back({
              .id = monitor["id"].get<int>(),
              .width = monitor["width"].get<int>(),
              .height = monitor["height"].get<int>(),
          });
        }
      }
    } else {
      std::println(std::cerr, "[ERROR] Monitors IPC response not an array");
      return {};
    }

    return result;
  } catch (const json::parse_error &e) {
    std::println(std::cerr, "[ERROR] Failed to parse monitors IPC response: {}",
                 e.what());
    return {};
  }
}

std::pair<int, int> get_mouse_position(const std::string &sock_path) {
  auto raw_resp = hyprland::IPC::send_command("j/cursorpos", sock_path);
  if (!raw_resp) {
    std::println(std::cerr, "[ERROR] {}", raw_resp.error());
    return {-1, -1};
  }

  try {
    json resp = json::parse(*raw_resp);

    if (resp.contains("x") && resp["x"].is_number() && resp.contains("y") &&
        resp["y"].is_number()) {
      return {
          resp["x"].get<int>(),
          resp["y"].get<int>(),
      };
    } else {
      std::println(std::cerr, "[ERROR] Cursorpos IPC response is invalid");
      return {-1, -1};
    }
  } catch (const json::parse_error &e) {
    std::println(std::cerr,
                 "[ERROR] Failed to parse cursorpos IPC response: {}",
                 e.what());
    return {-1, -1};
  }
}

std::string hide_window(const std::string &title,
                        const std::string &sock_path) {
  auto raw_resp = hyprland::IPC::send_command("j/clients", sock_path);
  if (!raw_resp) {
    std::println(std::cerr, "[ERROR] {}", raw_resp.error());
    return {};
  }

  try {
    json resp = json::parse(*raw_resp);

    if (resp.is_array()) {
      std::string target_address = "";
      std::string target_workspace = "";

      for (const auto &client : resp) {
        if (client.contains("title") && client["title"].is_string() &&
            client.contains("address") && client["address"].is_string() &&
            client.contains("workspace") && client["workspace"].is_object() &&
            client["workspace"].contains("name") &&
            client["workspace"]["name"].is_string()) {
          if (client["title"].get<std::string>() == title) {
            target_address = client["address"].get<std::string>();
            target_workspace = client["workspace"]["name"].get<std::string>();
            break;
          }
        }
      }

      if (target_address.empty()) {
        std::println(std::cerr, "[ERROR] Window with title '{}' not found",
                     title);
        return "";
      }

      std::string command =
          "dispatch movetoworkspacesilent special:hidden_apps,address:" +
          target_address;
      auto _ = hyprland::IPC::send_command(command, sock_path);

      return target_workspace;
    } else {
      std::println(std::cerr, "[ERROR] Clients IPC response not an array");
      return "";
    }
  } catch (const json::parse_error &e) {
    std::println(std::cerr, "[ERROR] Failed to parse clients IPC response: {}",
                 e.what());
    return "";
  }
}

std::string move_window_to_workspace(const std::string &title,
                                     const std::string &workspace,
                                     const std::string &sock_path) {
  auto raw_resp = hyprland::IPC::send_command("j/clients", sock_path);
  if (!raw_resp) {
    std::println(std::cerr, "[ERROR] {}", raw_resp.error());
    return {};
  }

  try {
    json resp = json::parse(*raw_resp);

    if (resp.is_array()) {
      std::string target_address = "";
      std::string target_workspace = "";

      for (const auto &client : resp) {
        if (client.contains("title") && client["title"].is_string() &&
            client.contains("address") && client["address"].is_string() &&
            client.contains("workspace") && client["workspace"].is_object() &&
            client["workspace"].contains("name") &&
            client["workspace"]["name"].is_string()) {
          if (client["title"].get<std::string>() == title) {
            target_address = client["address"].get<std::string>();
            target_workspace = client["workspace"]["name"].get<std::string>();
            break;
          }
        }
      }

      if (target_address.empty()) {
        std::println(std::cerr, "[ERROR] Window with title '{}' not found",
                     title);
        return "";
      }

      std::string command;
      if (workspace.empty()) {
        std::string active_workspace = get_active_workspace(sock_path);
        if (active_workspace.empty())
          return "";

        command = "dispatch movetoworkspace name:" + active_workspace +
                  ",address:" + target_address;
      } else {
        command = "dispatch movetoworkspace name:" + workspace +
                  ",address:" + target_address;
      }

      auto _ = hyprland::IPC::send_command(command, sock_path);

      return target_workspace;
    } else {
      std::println(std::cerr, "[ERROR] Clients IPC response not an array");
      return "";
    }
  } catch (const json::parse_error &e) {
    std::println(std::cerr, "[ERROR] Failed to parse clients IPC response: {}",
                 e.what());
    return "";
  }
}

void move_mouse(const std::pair<int, int> mouse_pos,
                const std::string &sock_path) {
  std::string command = "dispatch movecursor " +
                        std::to_string(mouse_pos.first) + " " +
                        std::to_string(mouse_pos.second);
  auto _ = hyprland::IPC::send_command(command, sock_path);
}

std::string get_active_workspace(const std::string &sock_path) {
  auto raw_resp = hyprland::IPC::send_command("j/activeworkspace", sock_path);
  if (!raw_resp) {
    std::println(std::cerr, "[ERROR] {}", raw_resp.error());
    return "";
  }

  try {
    json resp = json::parse(*raw_resp);

    if (resp.contains("name") && resp["name"].is_string()) {
      return resp["name"].get<std::string>();
    } else {
      std::println(std::cerr,
                   "[ERROR] Activeworkspace IPC response is invalid");
      return "";
    }
  } catch (const json::parse_error &e) {
    std::println(std::cerr,
                 "[ERROR] Failed to parse activeworkspace IPC response: {}",
                 e.what());
    return "";
  }
}

// TODO: FIx this
bool is_empty_workspace(const std::string &uuid, const std::string &workspace,
                        const std::string &sock_path) {
  auto raw_resp = hyprland::IPC::send_command("j/workspaces", sock_path);
  if (!raw_resp) {
    std::println(std::cerr, "[ERROR] {}", raw_resp.error());
    return false;
  }

  try {
    json resp = json::parse(*raw_resp);

    if (resp.is_array()) {
      for (const auto &ws : resp) {
        if (ws.contains("name") && ws["name"].is_string() &&
            ws.contains("windows") && ws["windows"].is_number()) {
          int win_count = ws["windows"].get<int>();
          if (win_count == 0)
            return true;

          if (win_count == 1) {
            if (ws.contains("lastwindowtitle") &&
                ws["lastwindowtitle"].is_string()) {
              std::string last_win_title =
                  ws["lastwindowtitle"].get<std::string>();
              if (last_win_title == uuid)
                return true;
            }
          }
        }
      }
    } else {
      std::println(std::cerr, "[ERROR] Workspaces IPC response not an array");
      return false;
    }
  } catch (const json::parse_error &e) {
    std::println(std::cerr,
                 "[ERROR] Failed to parse workspaces IPC response: {}",
                 e.what());
    return false;
  }

  return false;
}

std::string get_window_from_proc(const std::string &proc,
                                 const std::string &sock_path) {
  auto raw_resp = hyprland::IPC::send_command("j/clients", sock_path);
  if (!raw_resp) {
    std::println(std::cerr, "[ERROR] {}", raw_resp.error());
    return {};
  }

  try {
    json resp = json::parse(*raw_resp);

    if (resp.is_array()) {
      std::string target_address = "";

      for (const auto &client : resp) {
        if (client.contains("pid") && client["pid"].is_number() &&
            client.contains("address") && client["address"].is_string()) {
          std::string pid = std::to_string(client["pid"].get<int>());
          std::string client_proc = hyprdock::get_name_from_pid(pid);
          if (client_proc.empty())
            continue;

          fs::path client_proc_path{client_proc};
          std::string client_proc_name = client_proc_path.filename().string();

          fs::path proc_path{proc};
          std::string proc_name = proc_path.filename().string();

          if (client_proc_name == proc_name) {
            target_address = client["address"].get<std::string>();
            break;
          }
        }
      }

      if (target_address.empty())
        return "";

      return target_address;
    } else {
      std::println(std::cerr, "[ERROR] Clients IPC response not an array");
      return "";
    }
  } catch (const json::parse_error &e) {
    std::println(std::cerr, "[ERROR] Failed to parse clients IPC response: {}",
                 e.what());
    return "";
  }
}

void focus_window(const std::string &address, const std::string &sock_path) {
  std::string command = "dispatch focuswindow address:" + address;
  auto _ = hyprland::IPC::send_command(command, sock_path);
}

void set_plain_window(const std::string &uuid, const std::string &sock_path) {
  std::string command = "keyword windowrulev2 noborder,title:" + uuid;
  auto _ = hyprland::IPC::send_command(command, sock_path);

  command = "keyword windowrulev2 decorate:false,title:" + uuid;
  _ = hyprland::IPC::send_command(command, sock_path);

  command = "keyword windowrulev2 noshadow,title:" + uuid;
  _ = hyprland::IPC::send_command(command, sock_path);
}

void set_unmoveable_window(const std::string &uuid,
                           const std::string &sock_path) {
  std::string command = "keyword windowrulev2 nomove,title:" + uuid;
  auto _ = hyprland::IPC::send_command(command, sock_path);
}

} // namespace hyprland::command
