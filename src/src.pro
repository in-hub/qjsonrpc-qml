include(../qjsonrpc.pri)

INCLUDEPATH += .
TEMPLATE = lib
TARGET = qjsonrpc
QT += core core-private network
QT -= gui
DEFINES += QJSONRPC_BUILD
CONFIG += $${QJSONRPC_LIBRARY_TYPE}
VERSION = $${QJSONRPC_VERSION}
win32:DESTDIR = $$OUT_PWD

# check if we need to build qjson
QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VERSION_MAJOR = $$member(QT_VERSION, 0)
QT_VERSION_MINOR = $$member(QT_VERSION, 1)
lessThan(QT_VERSION_MAJOR, 5) {
    include(json/json.pri)
}

PRIVATE_HEADERS += \
    qjsonrpcservice_p.h \
    qjsonrpcsocket_p.h \
    qjsonrpcabstractserver_p.h \
    qjsonrpcservicereply_p.h

INSTALL_HEADERS += \
    qjsonrpcmessage.h \
    qjsonrpcservice.h \
    qjsonrpcsocket.h \
    qjsonrpcabstractserver.h \
    qjsonrpclocalserver.h \
    qjsonrpctcpserver.h \
    qjsonrpc_export.h \
    qjsonrpcservicereply.h \
    qjsonrpchttpclient.h

HEADERS += \
    $${INSTALL_HEADERS} \
    $${PRIVATE_HEADERS}
       
SOURCES += \
    qjsonrpcmessage.cpp \
    qjsonrpcservice.cpp \
    qjsonrpcsocket.cpp \
    qjsonrpcabstractserver.cpp \
    qjsonrpclocalserver.cpp \
    qjsonrpctcpserver.cpp \
    qjsonrpcservicereply.cpp \
    qjsonrpchttpclient.cpp

# install
headers.files = $${INSTALL_HEADERS}
headers.path = $${PREFIX}/include/qjsonrpc
qjson_headers.files = $${QJSON_INSTALL_HEADERS}
qjson_headers.path = $${PREFIX}/include/qjsonrpc/json
target.path = $${PREFIX}/$${LIBDIR}
INSTALLS += headers qjson_headers target

# pkg-config support
CONFIG += create_pc create_prl no_install_prl
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
equals(QJSONRPC_LIBRARY_TYPE, staticlib) {
    QMAKE_PKGCONFIG_CFLAGS = -DQJSONRPC_STATIC
} else {
    QMAKE_PKGCONFIG_CFLAGS = -DQJSONRPC_SHARED
}
unix:QMAKE_CLEAN += -r pkgconfig lib$${TARGET}.prl

