DEPTH = ../../..
include($$DEPTH/qjsonrpc.pri)

TEMPLATE = app
TARGET = qjsonrpc
INCLUDEPATH += .
CONFIG += console
CONFIG -= app_bundle

SOURCES = qjsonrpc.cpp
