DEPTH = ../../..
include($${DEPTH}/qjsonrpc.pri)
include($${DEPTH}/tests/tests.pri)
include(http-parser/http-parser.pri)

TARGET = tst_qjsonrpchttpclient
HEADERS += \
    testhttpserver.h
SOURCES += \
    testhttpserver.cpp \
    tst_qjsonrpchttpclient.cpp
