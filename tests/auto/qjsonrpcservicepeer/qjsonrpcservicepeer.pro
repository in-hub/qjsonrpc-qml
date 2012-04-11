load(qttest_p4)
DEPTH = ../../..
include($$DEPTH/qjsonrpc.pri)

TARGET = tst_qjsonrpcservicepeer
QT = core testlib network
CONFIG -= app_bundle
CONFIG += qtestlib
SOURCES = tst_qjsonrpcservicepeer.cpp
