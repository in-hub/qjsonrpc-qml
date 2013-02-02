load(qttest_p4)
DEPTH = ../../..
include($$DEPTH/qjsonrpc.pri)

TARGET = tst_qtjson
QT = core testlib
CONFIG -= app_bundle
CONFIG += testcase
INCLUDEPATH += ../../../src
LIBS += -L../../../src -lqjsonrpc

TESTDATA += test.json test.bjson test3.json test2.json
SOURCES += tst_qtjson.cpp
