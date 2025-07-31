#pragma once

#include <expected>
#include <optional>
#include <string>

namespace hyprland::IPC {

std::optional<std::string> get_runtime_dir();
std::optional<std::string> get_instance_signature();
std::optional<std::string> get_socket_path();
std::expected<std::string, std::string>
send_command(const std::string &command, const std::string &sock_path);

} // namespace hyprland::IPC
