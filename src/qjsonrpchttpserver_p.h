#ifndef QJSONRPCHTTPSERVER_P_H
#define QJSONRPCHTTPSERVER_P_H

#include <QHash>
#include "http_parser.h"
#include "qjsonrpcservice.h"

class QAbstractSocket;
class QJsonRpcHttpRequest : public QIODevice
{
    Q_OBJECT
public:
    explicit QJsonRpcHttpRequest(QAbstractSocket *socket, QObject *parent = 0);
    ~QJsonRpcHttpRequest();

    bool isSequential() const;

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private Q_SLOTS:
    void readIncomingData();

private:
    static int onMessageBegin(http_parser *parser);
    static int onUrl(http_parser *parser, const char *at, size_t length);
    static int onHeaderField(http_parser *parser, const char *at, size_t length);
    static int onHeaderValue(http_parser *parser, const char *at, size_t length);
    static int onHeadersComplete(http_parser *parser);
    static int onBody(http_parser *parser, const char *at, size_t length);
    static int onMessageComplete(http_parser *parser);

private:
    Q_DISABLE_COPY(QJsonRpcHttpRequest)

    QAbstractSocket *m_requestSocket;

    // request
    QByteArray m_requestPayload;
    http_parser *m_requestParser;
    http_parser_settings m_requestParserSettings;

    // for header processing
    QHash<QString, QString> m_requestHeaders;
    QString m_currentHeaderField;
    QString m_currentHeaderValue;

    // response
    QByteArray m_responseBuffer;

};

#endif
