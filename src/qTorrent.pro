QT += core network gui widgets

CONFIG += c++11

TARGET = qTorrent
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    bencode.cpp \
    bencodevalue.cpp \
    torrentinfo.cpp \
    trackerclient.cpp \
    torrentclient.cpp \
    qtorrent.cpp \
    torrent.cpp \
    peer.cpp \
    piece.cpp \
    block.cpp

HEADERS += \
    bencode.h \
    bencodevalue.h \
    torrentinfo.h \
    trackerclient.h \
    torrentclient.h \
    qtorrent.h \
    torrent.h \
    peer.h \
    piece.h \
    block.h
