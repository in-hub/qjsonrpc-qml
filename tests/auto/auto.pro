TEMPLATE = subdirs
SUBDIRS += \
    qjsonrpcmessage \
    qjsonrpcsocket \
    qjsonrpcserver \
    qjsonrpcservice \
    qjsonrpchttpclient

lessThan(QT_MAJOR_VERSION, 5) {
    SUBDIRS += json
}
