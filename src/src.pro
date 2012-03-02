include(../json/json.pri)

TEMPLATE = lib
TARGET = qjsonrpc
CONFIG += staticlib 

QT += core \
      network
DESTDIR = ../lib

HEADERS += qjsonrpc.h \
           qjsonrpc_p.h
SOURCES += qjsonrpc.cpp \
           main.cpp

