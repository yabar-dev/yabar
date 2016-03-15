# Yabar

A modern and lightweight status bar for X window managers.

## Screenshots

![screen 01](examples/screenshots/scr01.png)
![screen 02](examples/screenshots/scr02.png)
![screen 03](examples/screenshots/scr03.png)

## Description

Yabar is a modern and lightweight status bar that is intended to be used along with minimal X window managers like `bspwm` and `i3`. Yabar has the following features:

* Simple configuration using a single config file based on libconfig syntax.
* Pango font rendering with support of pango markup language (Xft may be supported in the future).
* Support for transparency (Yabar currently uses a hard-coded depth of 32).
* Multiple bars within the same session.

**Warning**: Yabar is still in its infancy and far from being mature. Feel free to contribute or report bugs!

## Terminology

A Yabar session should contain one or more *bars* within the same session. Each bar should contain one or more *blocks*. Each block should display some useful info to the user (free memory, CPU temperature, etc...).

## Installation
Yabar requires libconfig, cairo, and pango. These dependencies can be installed through your distribution's package manager, such as `dnf install libconfig-devel cairo-devel pango-devel` on Fedora or `apt-get install libconfig-dev cairo-dev pango-dev` on Ubuntu.

Once these are installed, running `make` will output the yabar executable. Copy this onto your PATH (`sudo cp yabar /usr/local/bin`) to install yabar globally on your system.

## Configuration

Yabar currently by default accepts configuration from the config file `~/.config/yabar/yabar.conf` or using `yabar -c [CONFIG_FILE]`. The config file should like something like this:

    bar-list: ["bar1", "bar2", ...];
    
    bar1: {
        \\bar specific options\\
        block-list: ["block1", "block2", ...];
        block1: {
            \\block specific options\\
        }
        block2: {
            \\block specific options\\
        }
    }

A bar or a block can be named to whatever name (preferably a short and meaningful name). Only names that are included in the "bar-list" and "block-list" entries will be drawn on the screen.


### Bar-specific options

Each bar can have its font, position (currently only top and bottom), background color, height, horizontal and vertical gaps, and other options.

* Font: Yabar currently accepts a string that contains a font or a list of fonts (similar to i3). Example:

        font: "Droid Sans, FontAwesome Bold 9";

* Position: Yabar currently accepts top and bottom. Example:

        position: "top";

* Gaps: You can define the size of horizontal and vertical gaps in pixels. Default is zero. Examples:

        gap-horizontal: 20;
        gap-vertical: 5;
    
* Height: Default is 20 pixels. Example:

        height: 25;
        
* Width: The default bar width is `screen size - 2 * horizontal gap`. However, if this option is used, the bar starts at `horizontal gap` and ends at `horizontal gap + width`. Example:

		width: 800;

* Underline and overline sizes: This option defines the thickness of underlines and overlines. Default is 0. Example:

        underline-size: 2;
        overline-size: 2;

* Slack: You can define the size of the slack (i.e. the unused space between blocks). Default is 0. Example:

        slack: 2;



### Block-specific options

Each block can have its command/script, background, foreground (i.e. font), underline and overline colors, alignment and other options.

* Execution: The path to the command/script to be executed. Yabar consumes the output of the command/script's stdout and shows it on the bar. Example:

        exec: "date";

* Alignment: Yabar accepts *left*, *center* or *right* alignments. consecutive blocks will be placed to the right of each other. Example:

        align: "right";

* Type: The block type can be *periodic* where the command/script is executed within a fixed interval of time, *persistent* where the command/script runs in a persistent way like `xtitle` or *once* where the command/script is executed only once where the intended info should not change like in `whoami`. Examples:


        type: "periodic";
        type: "persist";
        type: "once";

* Interval: In seconds. This is only useful when the  block type is periodic. Example:

        interval: 3;

* Fixed size: You should define the fixed width size of the block. Yabar currently only supports fixed widths (this will be improved soon). You can deduce the appropriate width using trial and error. The current default value is 80 but you are encouraged to override it to a more appropriate value. Example:

        fixed-size: 90;

* Pango markup: Yabar accepts either true or false without quotes. Default is false. Example:

        pango-markup: true;

* Colors: A block has 4 kinds of colors. Background, foreground which is the font color when pango markup is not used, underline and overline. Colors are accepted in hex RRGGBB and AARRGGBB representations. Examples:

        foreground-color-rgb    : 0xeeeeee;
        background-color-argb   : 0x1dc93582;
        underline-color-rgb     : 0x1d1d1d;
        overline-color-argb     : 0xf0642356;

    Note that the values are integers and not double-quoted strings.

* Pointer commands: This option is used to invoke a command/script upon a mouse button press. You have 5 buttons that usually represent left click, right click, middle click, scroll up and scroll down respectively but this may not be the case for everyone. Examples:

        command-button1: "pavucontrol";
        command-button4: "pactl set-sink-volume 0 +10%";
        command-button5: "pactl set-sink-volume 0 -10%";

## TODO

There is a lot to do, but among the most important things:
* RandR support.
* Automatic size of blocks.
* Internal blocks.

