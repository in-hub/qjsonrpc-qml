include(json/json.pri)

TEMPLATE = lib
TARGET = qjsonrpc
# Dll
#DEFINES += Q_BUILD_JSONRPC
#win32:DEFINES += Q_JSONRPC_DLL
CONFIG += staticlib

QT += core \
      network
DESTDIR = ../lib

HEADERS += qjsonrpcservice.h \
           qjsonrpcservice_p.h \
           qjsonrpcmessage.h \
           qjsonrpcmessage_p.h
SOURCES += qjsonrpcservice.cpp \
           qjsonrpcmessage.cpp
