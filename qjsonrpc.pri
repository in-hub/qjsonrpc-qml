QJSONRPC_PATH = $${PWD}/lib
QJSONRPC_INCLUDE_PATH = $${PWD}/src
QT += network

INCLUDEPATH += $${PWD}/json

win32 {
    LIBS += -L$${QJSONRPC_PATH} -lqjsonrpc0
} macx {
    QMAKE_LFLAGS += -F$${QJSONRPC_PATH}
    LIBS += -framework qjsonrpc
} unix:!macx {
    LIBS += -L$${QJSONRPC_PATH} -lqjsonrpc
}

INCLUDEPATH +=  $${QJSONRPC_INCLUDE_PATH} \
	    	$${PWD}