# Install binary
target.path = /usr/bin
INSTALLS += target

# Menu icon
menuicon.files = ../res/qTorrent.desktop
menuicon.path = /usr/share/applications
INSTALLS += menuicon

# Pixmap files
pixmap.files = ../res/icons/qTorrent/qtorrent.png
pixmap.path = /usr/share/pixmaps
INSTALLS += pixmap

# Install manpages
man.files = ../doc/qtorrent.1.gz
man.path = /usr/share/man/man1
INSTALLS += man
