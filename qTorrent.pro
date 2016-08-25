QT += core
QT -= gui

CONFIG += c++11

TARGET = qTorrent
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    bencode.cpp \
    bencodevalue.cpp \
    torrentinfo.cpp

HEADERS += \
    bencode.h \
    bencodevalue.h \
    torrentinfo.h
