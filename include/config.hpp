#pragma once

#include <raylib.h>
#include <string>
#include <vector>

#include "utils.hpp"

struct App {
  std::string name;
  std::string icon;
  std::string run;
};

struct Config {
  int monitor;
  int wait_time;

  int dock_padding;
  int dock_margin;
  Color dock_color;

  int app_size;
  int app_padding;

  std::vector<DesktopEntry> applications;
};

namespace hyprdock {

Config load_config();

} // namespace hyprdock
