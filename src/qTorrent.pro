QT += core network gui widgets

CONFIG += c++11

TARGET = qTorrent
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
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
    ui/mainwindow.cpp \
    ui/panel.cpp \
    ui/torrentslist.cpp \
    ui/torrentslistitem.cpp \
    ui/addtorrentdialog.cpp \
    ui/torrentitemdelegate.cpp \
    ui/torrentinfopanel.cpp

HEADERS += \
    qtorrent.h \
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
    ui/mainwindow.h \
    ui/panel.h \
    ui/torrentslist.h \
    ui/torrentslistitem.h \
    ui/addtorrentdialog.h \
    ui/torrentitemdelegate.h \
    ui/torrentinfopanel.h

RESOURCES += \
    resources.qrc
