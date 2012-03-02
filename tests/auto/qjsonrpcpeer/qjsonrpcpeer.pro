load(qttest_p4)
include(../../../qjsonrpc.pri)

TARGET = tst_qjsonrpcpeer
QT = core testlib network
CONFIG -= app_bundle
CONFIG  += qtestlib
SOURCES = tst_qjsonrpcpeer.cpp
