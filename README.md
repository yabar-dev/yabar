# Yabar [![Build Status](https://travis-ci.org/yabar-dev/yabar.svg?branch=master)](https://travis-ci.org/yabar-dev/yabar)

A modern and lightweight status bar for X window managers.

## Screenshots

![screen 01](examples/screenshots/scr01.png)
![screen 02](examples/screenshots/scr02.png)
![screen 03](examples/screenshots/scr03.png)

## Description

Yabar is a modern and lightweight status bar that is intended to be used along with minimal X window managers like `bspwm` and `i3`. Yabar has the following features:

* Extremely configurable with easy configuration system using a single config file.
* A growing set of ready-to-use internal blocks developed in plain c.
* Pango font rendering with support of pango markup language.
* Support for icons and images.
* Support for transparency.
* Multi-monitor support using RandR.
* Entirely clickable.
* Support for several environment variables to help button commands.
* Multiple bars within the same session.

**Warning**: Yabar is still in its infancy and far from being mature. Feel free to contribute or report bugs!

## Installation

### Packages

#### ArchLinux

AUR: [yabar](https://aur.archlinux.org/packages/yabar/) and [yabar-git](https://aur.archlinux.org/packages/yabar-git/)

#### Debian

[yabar](https://packages.debian.org/search?keywords=yabar) in [Testing (Stretch)](https://packages.debian.org/stretch/yabar) and [Unstable (Sid)](https://packages.debian.org/sid/yabar)

#### Ubuntu

[yabar](http://packages.ubuntu.com/search?keywords=yabar&searchon=names&suite=all&section=all) in [Yakkety Yak](http://packages.ubuntu.com/yakkety/yabar)

### From Source
Yabar initially requires libconfig, cairo, pango and alsa. The feature `DYA_INTERNAL_EWMH` in `Makefile` additionaly xcb-ewmh (or xcb-util-wm in some distros) and the feature `-DYA_ICON` requires gdk-pixbuf2. These dependencies can be installed through your distribution's package manager:

* Fedora: `dnf install libconfig-devel cairo-devel pango-devel gdk-pixbuf2-devel alsa-lib-devel`
* Debian / Ubuntu: `apt-get install libcairo2-dev libpango1.0-dev libconfig-dev libxcb-randr0-dev libxcb-ewmh-dev libgdk-pixbuf2.0-dev libasound2-dev`

You can install yabar as follows:

		$ git clone https://github.com/yabar-dev/yabar
		$ cd yabar
		$ make
		$ sudo make install

If you use libconfig 1.4.x (still used in Ubuntu 14.04 and Debian Jessie), please type `export CPPFLAGS=-DOLD_LIBCONFIG` then build using `make` as usual.

## Configuration

[Check our wiki](https://github.com/yabar-dev/yabar/wiki) on [how to customize](https://github.com/yabar-dev/yabar/wiki/Configuration) yabar

## License

Yabar is licensed under the MIT license. For more info check out the file `LICENSE`.
