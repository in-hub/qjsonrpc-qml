include(../json/json.pri)

TEMPLATE = lib
TARGET = qjsonrpc
CONFIG += static shared
macx: CONFIG += lib_bundle

QT += core \
      network
DESTDIR = ../lib

HEADERS += qjsonrpc.h \
           qjsonrpc_p.h
SOURCES += qjsonrpc.cpp \
           main.cpp

