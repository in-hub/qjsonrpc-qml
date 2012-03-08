DEPTH = ../../..
include($$DEPTH/qjsonrpc.pri)

QT += script network
win32: CONFIG += console
mac:CONFIG -= app_bundle
LIBS += $${QJSONRPC_INTERNAL_LIBS}

HEADERS = interface.h
SOURCES = interface.cpp \
          main.cpp
