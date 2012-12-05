QJSONRPC_VERSION = 1.0.0

isEmpty(QJSONRPC_LIBRARY_TYPE) {
    QJSONRPC_LIBRARY_TYPE = shared
}

QT += network
QJSONRPC_INCLUDEPATH = $${PWD}/src
QJSONRPC_LIBS = -lqjsonrpc
contains(QJSONRPC_LIBRARY_TYPE, staticlib) {
    DEFINES += QJSONRPC_STATIC
} else {
    DEFINES += QJSONRPC_SHARED
}

isEmpty(PREFIX) {
    PREFIX = /usr/local
}
isEmpty(LIBDIR) {
    LIBDIR = lib
}