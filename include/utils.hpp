#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>

struct DesktopEntry {
  std::string name;
  std::string comment;
  std::string icon;
  std::string exec;
  std::string type;
  bool no_display = false;
  bool hidden = false;
};

namespace fs = std::filesystem;

namespace hyprdock {

std::string trim(const std::string &str);
std::vector<fs::path> get_xdg_data_dirs();
std::string get_name_from_pid(const std::string &pid);
std::string generate_id();
std::string resolve_app_icon(const std::string &icon_name,
                             int desired_size = 48);
std::optional<DesktopEntry> parse_desktop_file(const fs::path &path);
std::optional<DesktopEntry> get_entry_for_name(const std::string &name);
std::string get_first_token(const std::string &str);
void run_app(const DesktopEntry &app);

} // namespace hyprdock
