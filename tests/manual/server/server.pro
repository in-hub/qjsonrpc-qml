include(../../../qjsonrpc.pri)

TEMPLATE = app
TARGET = server
INCLUDEPATH += .
CONFIG += console
CONFIG -= app_bundle

HEADERS = testservice.h
SOURCES = testservice.cpp \
          server.cpp
