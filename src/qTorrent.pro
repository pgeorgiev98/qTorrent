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
    core/torrentmanager.cpp \
    ui/mainwindow.cpp \
    ui/panel.cpp \
    ui/torrentslist.cpp \
    ui/torrentslistitem.cpp

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
    core/torrentmanager.h \
    ui/mainwindow.h \
    ui/panel.h \
    ui/torrentslist.h \
    ui/torrentslistitem.h

RESOURCES += \
    resources.qrc
