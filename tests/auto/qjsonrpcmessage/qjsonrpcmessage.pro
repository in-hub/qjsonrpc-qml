load(qttest_p4)
include(../../../qjsonrpc.pri)

TARGET = tst_qjsonrpcmessage
QT = core testlib network
CONFIG -= app_bundle
CONFIG  += qtestlib
SOURCES = tst_qjsonrpcmessage.cpp
