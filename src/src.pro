include(json/json.pri)

TEMPLATE = lib
TARGET = qjsonrpc
CONFIG += staticlib

QT += core \
      network
DESTDIR = ../lib

HEADERS += qjsonrpcpeer.h \
           qjsonrpcmessage.h \
           qjsonrpcmessage_p.h
SOURCES += qjsonrpcpeer.cpp \
           qjsonrpcmessage.cpp \
           main.cpp

