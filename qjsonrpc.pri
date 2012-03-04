QJSONRPC_PATH = $${OUT_PWD}/$${DEPTH}/lib
QJSONRPC_INCLUDE_PATH = $${PWD}/src
QT += network

INCLUDEPATH += $${QJSONRPC_INCLUDE_PATH}
LIBS += -L$${QJSONRPC_PATH} -lqjsonrpc

#Dll
#win32:DEFINES += Q_JSONRPC_DLL

#win32 {
#  LIBS += -L$${QJSONRPC_PATH} -lqjsonrpc0
#} macx {
#  QMAKE_LFLAGS += -F$${QJSONRPC_PATH}
#  LIBS += -framework qjsonrpc
#} unix:!macx {
#  LIBS += -L$${QJSONRPC_PATH} -lqjsonrpc
#}

