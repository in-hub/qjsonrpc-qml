DEPTH = ../../..
include($$DEPTH/qjsonrpc.pri)

TEMPLATE = app
TARGET = client
INCLUDEPATH += .
CONFIG += console
CONFIG -= app_bundle

HEADERS = localclient.h
SOURCES = localclient.cpp \
          main.cpp

