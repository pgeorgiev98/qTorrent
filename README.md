# qTorrent

## Overview

qTorrent is an open-source, cross-platform BitTorrent client currently being written in C++ with Qt.
The project is still at an early stage of development.

## Supported platforms

The goal for qTorrent is to be able to work on all major desktop operating systems - Windows, MacOS and Linux on both Xorg and Wayland.

## Compiling

You can build qTorrent with:

	# Create build directory (name and path don't matter)
	mkdir /tmp/qTorrent-build
	cd $_

	# To create a release build
	qmake /path/to/qTorrent/src/qTorrent.pro

	# ... or debug build
	qmake /path/to/qTorrent/src/qTorrent.pro CONFIG+=debug

	make -j5
	# The '-j5' means "Use up to 5 threads to build"
	# which speeds up things quite a bit

	# Make sure you're using the propper Qt5 version of qmake
	# On some systems (e.g. Gentoo) you may need to add a '-qt=5'
	# option to the qmake command.

## Usage

Just call

	./qTorrent

## Current state

Currently, qTorrent:
* Can fully download torrents
* Can seed torrents (more testing needed)
* Has a very basic GUI
* Does **Not** support any extensions
* Can **Not** pause/resume downloads
* Lacks a lot of other stuff
