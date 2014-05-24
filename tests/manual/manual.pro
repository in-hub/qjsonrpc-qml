TEMPLATE = subdirs
SUBDIRS += localserver \
           localclient \
           tcpserver \
           tcpclient \
           qjsonrpc \
           httpclient \
           benchmark

greaterThan(QT_MAJOR_VERSION, 4) {
    qtHaveModule(script): SUBDIRS += console
} else {
    SUBDIRS += console
}
