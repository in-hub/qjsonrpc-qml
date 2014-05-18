DEPTH = ../../..
include($${DEPTH}/qjsonrpc.pri)
include($${DEPTH}/tests/tests.pri)

TARGET = tst_benchmark
SOURCES = tst_benchmark.cpp
QT += core-private
