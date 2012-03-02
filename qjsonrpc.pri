QJSONRPC_PATH = $${PWD}/lib
QJSONRPC_INCLUDE_PATH = $${PWD}/src
QT += network

INCLUDEPATH += $${PWD}/json
LIBS += $${QJSONRPC_PATH}/libqjsonrpc.a

INCLUDEPATH +=  $${QJSONRPC_INCLUDE_PATH} \
	    	$${PWD}