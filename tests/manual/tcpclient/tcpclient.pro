DEPTH = ../../..
include($$DEPTH/qjsonrpc.pri)

TEMPLATE = app
TARGET = client
INCLUDEPATH += .
CONFIG += console
CONFIG -= app_bundle

HEADERS = tcpclient.h
SOURCES = tcpclient.cpp \
          main.cpp

