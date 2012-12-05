macx:CONFIG -= app_bundle
CONFIG  += qtestlib
INCLUDEPATH += $${QJSONRPC_INCLUDEPATH}
LIBS += -L$${DEPTH}/src $${QJSONRPC_LIBS}
