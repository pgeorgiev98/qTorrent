QT += core network gui widgets

CONFIG += c++11

TARGET = qTorrent

TEMPLATE = app

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

SOURCES += main.cpp \
    global.cpp \
    qtorrent.cpp \
    core/bencodeparser.cpp \
    core/bencodevalue.cpp \
    core/torrentinfo.cpp \
    core/trackerclient.cpp \
    core/torrent.cpp \
    core/peer.cpp \
    core/piece.cpp \
    core/block.cpp \
    core/torrentmessage.cpp \
    core/torrentserver.cpp \
    core/resumeinfo.cpp \
    core/torrentmanager.cpp \
    core/torrentsettings.cpp \
    core/remote.cpp \
    core/filecontroller.cpp \
    ui/mainwindow.cpp \
    ui/panel.cpp \
    ui/torrentslist.cpp \
    ui/torrentslistitem.cpp \
    ui/addtorrentdialog.cpp \
    ui/torrentitemdelegate.cpp \
    ui/torrentinfopanel.cpp

HEADERS += \
    qtorrent.h \
    global.h \
    core/bencodeparser.h \
    core/bencodevalue.h \
    core/torrentinfo.h \
    core/trackerclient.h \
    core/torrent.h \
    core/peer.h \
    core/piece.h \
    core/block.h \
    core/torrentmessage.h \
    core/torrentserver.h \
    core/resumeinfo.h \
    core/torrentmanager.h \
    core/torrentsettings.h \
    core/remote.h \
    core/filecontroller.h \
    ui/mainwindow.h \
    ui/panel.h \
    ui/torrentslist.h \
    ui/torrentslistitem.h \
    ui/addtorrentdialog.h \
    ui/torrentitemdelegate.h \
    ui/torrentinfopanel.h

RESOURCES += \
    resources.qrc

win32:RC_ICONS = ../res/icons/qtorrent.ico
macx:ICON = ../res/icons/qtorrent.icns

# UNIX-specific configuration
unix:!macx: include(../unixconf.pri)
