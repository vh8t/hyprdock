#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <linux/limits.h>
#include <optional>
#include <print>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "utils.hpp"

namespace fs = std::filesystem;

namespace hyprdock {

std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \n\t\r");
  if (std::string::npos == first)
    return str;
  size_t last = str.find_last_not_of(" \n\t\r");
  return str.substr(first, (last - first + 1));
}

std::vector<fs::path> get_xdg_data_dirs() {
  std::vector<fs::path> paths;

  const char *xdg_data_home_env = std::getenv("XDG_DATA_HOME");
  if (xdg_data_home_env && std::strlen(xdg_data_home_env) > 0) {
    paths.push_back(fs::path(xdg_data_home_env));
  } else {
    const char *home_dir_env = std::getenv("HOME");
    if (home_dir_env && std::strlen(home_dir_env) > 0) {
      paths.push_back(fs::path(home_dir_env) / ".local" / "share");
    }
  }

  const char *xdg_data_dirs_env = std::getenv("XDG_DATA_DIRS");
  if (xdg_data_dirs_env && std::strlen(xdg_data_dirs_env) > 0) {
    std::string_view dirs_str(xdg_data_dirs_env);
    size_t start = 0;
    size_t end = dirs_str.find(':');
    while (end != std::string_view::npos) {
      paths.push_back(fs::path(dirs_str.substr(start, end - start)));
      start = end + 1;
      end = dirs_str.find(':', start);
    }
    paths.push_back(fs::path(dirs_str.substr(start)));
  } else {
    paths.push_back("/usr/share");
    paths.push_back("/usr/local/share");
  }

  return paths;
}

std::string get_name_from_pid(const std::string &pid) {
  std::string link_path = "/proc/" + pid + "/exe";
  char buffer[PATH_MAX];
  ssize_t len = readlink(link_path.c_str(), buffer, sizeof(buffer) - 1);

  if (len < 0) {
    std::println(std::cerr, "[ERROR] Failed to read symlink {}: {}", link_path,
                 strerror(errno));
    return "";
  }

  buffer[len] = '\0';
  return std::string{buffer};
}

std::string generate_id() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<int> distrib(0x00000, 0xfffff);

  int random_int = distrib(gen);
  std::stringstream ss;
  ss << std::hex << std::setw(5) << std::setfill('0') << random_int;

  return ss.str();
}

std::string resolve_app_icon(const std::string &icon_name, int desired_size) {
  fs::path test_path{icon_name};
  if (test_path.is_absolute() && fs::exists(test_path) &&
      fs::is_regular_file(test_path))
    return test_path.string();

  const std::vector<std::string> categories = {"apps", "status", "devices",
                                               "mimetypes"};

  std::vector<fs::path> icon_base_dirs;
  for (const auto &xdg_dir : get_xdg_data_dirs())
    icon_base_dirs.push_back(xdg_dir / "icons");

  icon_base_dirs.push_back("/usr/share/pixmaps");

  const std::vector<std::string> themes = {"Adwaita", "hicolor", "gnome"};
  std::vector<int> sizes_to_check = {desired_size, 64,  48,  32, 24,
                                     16,           128, 256, 512};
  std::sort(sizes_to_check.begin(), sizes_to_check.end(),
            [desired_size](int a, int b) {
              return std::abs(a - desired_size) < std::abs(b - desired_size);
            });

  for (const auto &base_dir : icon_base_dirs) {
    if (!fs::exists(base_dir) || !fs::is_directory(base_dir))
      continue;

    for (const auto &theme_name : themes) {
      fs::path theme_path = base_dir / theme_name;
      if (!fs::exists(theme_path) || !fs::is_directory(theme_path))
        continue;

      for (int size : sizes_to_check) {
        std::string size_str =
            std::to_string(size) + "x" + std::to_string(size);
        for (const auto &category : categories) {
          fs::path potential_path =
              theme_path / size_str / category / (icon_name + ".png");
          if (fs::exists(potential_path) && fs::is_regular_file(potential_path))
            return potential_path.string();
        }
      }
    }
  }

  std::println("[WARNING] Icon not found for: {}", icon_name);
  return "";
}

std::optional<DesktopEntry> parse_desktop_file(const fs::path &path) {
  std::ifstream file{path};
  if (!file.is_open())
    return std::nullopt;

  DesktopEntry entry;
  std::string line;
  bool in_desktop_entry_section = false;

  while (std::getline(file, line)) {
    line = trim(line);

    if (line.empty() || line[0] == '#')
      continue;

    if (line == "[Desktop Entry]") {
      in_desktop_entry_section = true;
      continue;
    }

    if (line[0] == '[') {
      in_desktop_entry_section = false;
      continue;
    }

    if (in_desktop_entry_section) {
      size_t eq_pos = line.find('=');
      if (eq_pos != std::string::npos) {
        std::string key = trim(line.substr(0, eq_pos));
        std::string value = trim(line.substr(eq_pos + 1));

        if (key == "Name")
          entry.name = value;
        else if (key == "Comment")
          entry.comment = value;
        else if (key == "Icon")
          entry.icon = resolve_app_icon(value);
        else if (key == "Exec")
          entry.exec = get_first_token(value);
        else if (key == "NoDisplay")
          entry.no_display = (value == "true");
        else if (key == "Hidden")
          entry.hidden = (value == "true");
        else if (key == "Type")
          entry.type = value;
      }
    }
  }

  if (entry.type != "Application" || entry.exec.empty())
    return std::nullopt;

  return entry;
}

std::optional<DesktopEntry> get_entry_for_name(const std::string &name) {
  std::vector<fs::path> search_dirs;

  const char *xdg_data_home = std::getenv("XDG_DATA_HOME");
  if (xdg_data_home && std::strlen(xdg_data_home) > 0) {
    search_dirs.push_back(fs::path(xdg_data_home) / "applications");
  } else {
    const char *home_dir = std::getenv("HOME");
    if (home_dir && std::strlen(home_dir) > 0)
      search_dirs.push_back(fs::path(home_dir) / ".local" / "share" /
                            "applications");
  }

  const char *xdg_data_dirs = std::getenv("XDG_DATA_DIRS");
  if (xdg_data_dirs && std::strlen(xdg_data_dirs) > 0) {
    std::string_view dirs_str(xdg_data_dirs);
    size_t start = 0;
    size_t end = dirs_str.find(":");
    while (end != std::string_view::npos) {
      search_dirs.push_back(fs::path(dirs_str.substr(start, end - start)) /
                            "applications");
      start = end + 1;
      end = dirs_str.find(":", start);
    }
    search_dirs.push_back(fs::path(dirs_str.substr(start)) / "applications");
  } else {
    search_dirs.push_back("/usr/share/applications");
    search_dirs.push_back("/usr/local/share/applications");
  }

  std::string search_name_lower = name;
  std::transform(search_name_lower.begin(), search_name_lower.end(),
                 search_name_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  for (const auto &dir : search_dirs) {
    if (!fs::exists(dir) || !fs::is_directory(dir))
      continue;

    for (const auto &entry : fs::directory_iterator(dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
        auto desktop_entry_opt = parse_desktop_file(entry.path());

        if (desktop_entry_opt) {
          DesktopEntry current_entry = *desktop_entry_opt;

          if (current_entry.no_display || current_entry.hidden)
            continue;

          std::string entry_name_lower = current_entry.name;
          std::transform(entry_name_lower.begin(), entry_name_lower.end(),
                         entry_name_lower.begin(),
                         [](unsigned char c) { return std::tolower(c); });

          if (entry_name_lower == search_name_lower)
            return current_entry;

          std::string filename_without_ext = entry.path().stem().string();
          std::transform(filename_without_ext.begin(),
                         filename_without_ext.end(),
                         filename_without_ext.begin(),
                         [](unsigned char c) { return std::tolower(c); });

          if (filename_without_ext == search_name_lower)
            return current_entry;
        }
      }
    }
  }

  return std::nullopt;
}

std::string get_first_token(const std::string &str) {
  size_t first_space_pos = str.find(' ');
  if (first_space_pos == std::string::npos)
    return str;

  return str.substr(0, first_space_pos);
}

void run_app(const DesktopEntry &app) {
  pid_t pid = fork();
  if (pid == -1) {
    perror("Failed to fork process for app launch");
  } else if (pid == 0) {
    execlp(app.exec.c_str(), app.exec.c_str(), NULL);
    perror("Failed to execute command in child process");
    _exit(1);
  } else {
    std::println("[INFO] Launched app {} with pid {}", app.name, pid);
  }
}

} // namespace hyprdock
