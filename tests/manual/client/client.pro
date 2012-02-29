include(../../../qjsonrpc.pri)

TEMPLATE = app
TARGET = client
INCLUDEPATH += .
CONFIG += console
CONFIG -= app_bundle

HEADERS = client.h
SOURCES = client.cpp \
          main.cpp

