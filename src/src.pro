QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VERSION_MAJOR = $$member(QT_VERSION, 0)
lessThan(QT_VERSION_MAJOR, 5) {
message("building QJson")
include(json/json.pri)
}

TEMPLATE = lib
TARGET = qjsonrpc
# Dll
#DEFINES += Q_BUILD_JSONRPC
#win32:DEFINES += Q_JSONRPC_DLL
CONFIG += staticlib

QT += core network
DESTDIR = ../lib

HEADERS += \
    qjsonrpcservice.h \
    qjsonrpcservice_p.h \
    qjsonrpcmessage.h \
    qjsonrpcmessage_p.h
SOURCES += \
    qjsonrpcservice.cpp \
    qjsonrpcmessage.cpp
