[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/panel-plugins/xfce4-newtonmenu-plugin/-/blob/master/COPYING)

# Xfce4 newtonmenu Plugin

The Xfce4 newtonmenu Plugin provides a macOS-style application and session menu for the Xfce panel (version 4.16 or higher). It offers quick access to system actions and session management.

![newtonmenu Screenshot](image.png)

## Features

*   **macOS-like Menu:** A single button to access common system and session actions.
*   **Configurable Appearance:**
    *   Display either an icon or a text label on the panel button.
    *   Choose a custom icon (themed or full path) or set custom label text.
*   **Menu Actions:**
    *   About This PC
    *   System Settings
    *   App Store (placeholder - to be configured by user or future update)
    *   Force Quit (invokes `xkill`)
    *   Sleep
    *   Restart
    *   Shut Down
    *   Lock Screen
    *   Log Out (displays current username)
*   **Configurable Confirmations:** Choose whether to display confirmation dialogs for Log Out, Restart, Shut Down, and Force Quit actions.

## Requirements

*   Xfce Panel 4.16 or higher
*   GTK+ 3.24 or higher
*   GLib 2.66 or higher
*   libxfce4ui-2 4.16 or higher
*   libxfce4util-1.0 4.16 or higher
*   libxfce4panel-2.0 4.16 or higher
*   exo-2 0.12.0 or higher (for icon chooser dialog)

## Installation

### From Source Code Repository (using Meson - Recommended)

1.  Clone the repository:
    ```bash
    git clone <your_repository_url> xfce4-newtonmenu-plugin
    cd xfce4-newtonmenu-plugin
    ```
2.  Build and install using Meson:
    ```bash
    meson setup builddir
    meson compile -C builddir
    sudo meson install -C builddir
    ```
3.  Restart the Xfce Panel:
    ```bash
    xfce4-panel -r
    ```

### From Source Code Repository (using Autotools - if applicable)

    % cd xfce4-newtonmenu-plugin
    % ./autogen.sh
    % make
    % sudo make install
    % xfce4-panel -r

After installation, right-click on the Xfce panel, choose "Panel" -> "Add New Items", and select "Newton Button Plugin" from the list.

## Configuration

Right-click on the Newton Button on the panel and select "Properties" to configure:
*   **Button Appearance:**
    *   Choose between displaying an icon or text.
    *   Set the icon name/path or label text.
*   **Action Confirmations:**
    *   Enable or disable confirmation dialogs for potentially disruptive actions like Log Out, Restart, Shut Down, and Force Quit.

## Reporting Bugs

Please report any bugs or feature requests on the [GitLab issue tracker](<your_gitlab_project_url>/-/issues).

---

*This plugin is inspired by the desire for a simple, integrated system menu similar to those found on other desktop environments.*
