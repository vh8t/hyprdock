# Hyprdock

Hyprdock is a simple and lightweight dock bar designed specifically for the Hyprland compositor. Built to be a hassle-free, out-of-the-box solution, it provides a clean and efficient workspace with a simple JSON configuration. Developed in C++, it delivers a high-performance and stable experience.
<br>
Thanks to a custom Hyprland IPC library, Hyprdock has direct access to low-level Hyprland functionalities. This allows it to get real-time information such as **mouse position, windows on the desktop, and active workspaces**. This deep integration is what enables Hyprdock to intelligently manage your applications.

## Features
- **Lightweight and Fast**: Built in C++ for maximum performance and minimal resource usage.
- **Simple Configuration**: Easy to set up with a single JSON file.
- **Out-of-the-Box Functionality**: Works with Hyprland without complex setup.
- **Intelligent Application Handling**: Hyprdock uses a custom Hyprland IPC library to detect running applications. It can intelligently focus an application if it's already open or launch it if it's not.
- **Custom Icons**: Supports custom icons for your applications.

## Prerequisites
Before building Hyprdock, ensure you have the following dependencies installed on your system:

- **cmake**: The build system generator.
- **ninja**: A small, fast build system.
- **raylib**: A simple and easy-to-use library for creating graphical applications.
- **nlohmann/json**: A C++ header-only library for JSON.

## Building and Installation
Hyprdock includes a `build.sh` script to simplify the building and installation process.
<br>
Follow these steps to build and install Hyprdock:

```sh
# Clone the repository
git clone https://github.com/vh8t/hyprdock.git
cd hyprdock

# Run the build script
./build.sh

# Install the executable
# You may need to run this with sudo depending on your system's configuration.
sudo cmake --install build
```

## Configuration
Hyprdock's behavior is controlled by a single configuration file located at `~/.config/hypr/hyprdock.json`. If this file doesn't exist, the dock will appear as an empty window. You must create it and add your desired configuration.
<br>
Here is an example of the configuration format:

```json
{
  "monitor": 0,
  "wait_time": 300,

  "dock_style": {
    "padding": 10,
    "margin": 10,
    "color": {
      "r": 56,
      "g": 63,
      "b": 97
    }
  },

  "app_style": {
    "size": 45,
    "padding": 15
  },

  "applications": [
    "ghostty",
    "firefox",
    "gimp",
    {
      "name": "dolphin",
      "icon": "path/to/icon.png"
    }
  ]
}
```

- `monitor`: The ID of the monitor where the dock should appear. `0` is the default.
- `wait_time`: The delay in milliseconds before the dock is revealed when you hover over its location.
- `dock_style`: Defines the appearance of the dock bar.
    - `padding`: The space between the edge of the dock and the application icons, in pixels.
    - `margin`: The space between the dock and the edge of the monitor, in pixels.
    - `color`: The RGB color of the dock bar. Note that transparency is not currently supported.
- `app_style`: Defines the appearance of the application icons.
    - `size`: The size of the application icons, in pixels.
    - `padding`: The space between application icons, in pixels.
- `applications`: An array of applications to be displayed on the dock. Each element can be:
    - A string, which should be the case-insensitive name from the application's `.desktop` file (e.g., `firefox`).
    - An object, which must contain a `"name"` property (the desktop file name) and can optionally include an `"icon"` property to specify a custom icon file path if the default icon cannot be found or parsed.

## Future Plans
Add support for custom positioning of the dock on the screen (e.g., top, left, right).
