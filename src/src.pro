include(../json/json.pri)

TEMPLATE = lib
TARGET = qjsonrpc

macx: CONFIG += lib_bundle

QT += core \
      network
DESTDIR = ../lib

HEADERS += qjsonrpc.h
SOURCES += qjsonrpc.cpp \
           main.cpp

