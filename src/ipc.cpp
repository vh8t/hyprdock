#include <cstdlib>
#include <cstring>
#include <expected>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#include "ipc.hpp"

namespace hyprland::IPC {

std::optional<std::string> get_runtime_dir() {
  const char *xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");
  if (!xdg_runtime_dir)
    return std::nullopt;

  return std::string{xdg_runtime_dir};
}

std::optional<std::string> get_instance_signature() {
  const char *hyprland_instance_signature =
      std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (!hyprland_instance_signature)
    return std::nullopt;

  return std::string{hyprland_instance_signature};
}

std::optional<std::string> get_socket_path() {
  auto runtime_dir = get_runtime_dir();
  auto instance_signature = get_instance_signature();

  if (!runtime_dir || !instance_signature)
    return std::nullopt;

  return *runtime_dir + "/hypr/" + *instance_signature + "/.socket.sock";
}

std::expected<std::string, std::string>
send_command(const std::string &command, const std::string &sock_path) {
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    return std::unexpected{"Failed to open socket"};

  struct sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sock);
    return std::unexpected{"Failed to connect to socket"};
  }

  if (send(sock, command.c_str(), command.length(), 0) < 0) {
    close(sock);
    return std::unexpected{"Failed to send command"};
  }

  std::vector<char> buffer(4096);
  ssize_t bytes_read = recv(sock, buffer.data(), buffer.size() - 1, 0);
  close(sock);

  if (bytes_read < 0)
    return std::unexpected{"Failed to read from socket"};

  buffer[bytes_read] = '\0';
  return std::string{buffer.data()};
}

} // namespace hyprland::IPC
