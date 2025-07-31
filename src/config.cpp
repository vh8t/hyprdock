#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <print>
#include <pwd.h>
#include <unistd.h>

#include "config.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace hyprdock {

static Config default_config = {
    .monitor = 0,
    .wait_time = 300,

    .dock_padding = 10,
    .dock_margin = 10,
    .dock_color = {40, 48, 85, 255},

    .app_size = 45,
    .app_padding = 15,

    .applications = {},
};

Config load_config() {
  fs::path config_file;

  const char *xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  if (xdg_config_home && std::strlen(xdg_config_home) > 0) {
    config_file = xdg_config_home;
  } else {
    const char *home_dir = std::getenv("HOME");
    if (home_dir && std::strlen(home_dir) > 0) {
      config_file = xdg_config_home;
      config_file /= ".config";
    } else {
      struct passwd *pw = getpwuid(getuid());
      if (pw && pw->pw_dir) {
        config_file = pw->pw_dir;
        config_file /= ".config";
      } else {
        return default_config;
      }
    }
  }

  config_file = config_file / "hypr" / "hyprdock.json";

  if (!fs::exists(config_file))
    return default_config;

  try {
    std::ifstream file_stream(config_file);
    if (!file_stream.is_open()) {
      std::println(std::cerr, "[ERROR] Failed to open config file: {}",
                   config_file.string());
      return default_config;
    }

    json config_json;
    file_stream >> config_json;

    Config loaded_config = default_config;

    if (config_json.contains("monitor") && config_json["monitor"].is_number())
      loaded_config.monitor = config_json["monitor"].get<int>();
    if (config_json.contains("wait_time") &&
        config_json["wait_time"].is_number())
      loaded_config.wait_time = config_json["wait_time"].get<int>();
    if (config_json.contains("dock_style") &&
        config_json["dock_style"].is_object()) {
      json dock_style = config_json["dock_style"];
      if (dock_style.contains("padding") && dock_style["padding"].is_number())
        loaded_config.dock_padding = dock_style["padding"].get<int>();
      if (dock_style.contains("margin") && dock_style["margin"].is_number())
        loaded_config.dock_margin = dock_style["margin"].get<int>();
      if (dock_style.contains("color") && dock_style["color"].is_object()) {
        json color = dock_style["color"];
        if (color.contains("r") && color["r"].is_number_unsigned())
          loaded_config.dock_color.r = color["r"].get<unsigned char>();
        if (color.contains("g") && color["g"].is_number_unsigned())
          loaded_config.dock_color.g = color["g"].get<unsigned char>();
        if (color.contains("b") && color["b"].is_number_unsigned())
          loaded_config.dock_color.b = color["b"].get<unsigned char>();
      }
    }
    if (config_json.contains("app_style") &&
        config_json["app_style"].is_object()) {
      json app_style = config_json["app_style"];
      if (app_style.contains("size") && app_style["size"].is_number())
        loaded_config.app_size = app_style["size"].get<int>();
      if (app_style.contains("padding") && app_style["padding"].is_number())
        loaded_config.app_padding = app_style["padding"].get<int>();
    }
    if (config_json.contains("applications") &&
        config_json["applications"].is_array()) {
      for (const auto &app : config_json["applications"]) {
        if (app.is_string()) {
          std::string app_name = app.get<std::string>();
          auto desktop_entry = get_entry_for_name(app_name);

          if (desktop_entry)
            loaded_config.applications.push_back(*desktop_entry);
        } else if (app.is_object()) {
          if (app.contains("name") && app["name"].is_string()) {
            std::string app_name = app["name"].get<std::string>();
            auto desktop_entry = get_entry_for_name(app_name);

            if (desktop_entry) {
              if (app.contains("icon") && app["icon"].is_string()) {
                std::string icon = app["icon"].get<std::string>();
                (*desktop_entry).icon = icon;
              }
              loaded_config.applications.push_back(*desktop_entry);
            }
          }
        }
      }
    }

    return loaded_config;
  } catch (const json::parse_error &e) {
    std::println(std::cerr, "[ERROR] Failed to parse config: {}", e.what());
    return default_config;
  }
}

} // namespace hyprdock
