include(json/json.pri)

TEMPLATE = lib
TARGET = qjsonrpc
CONFIG += staticlib

QT += core \
      network
DESTDIR = ../lib

HEADERS += qjsonrpcservice.h \
           qjsonrpcmessage.h \
           qjsonrpcmessage_p.h
SOURCES += qjsonrpcservice.cpp \
           qjsonrpcmessage.cpp \
           main.cpp
