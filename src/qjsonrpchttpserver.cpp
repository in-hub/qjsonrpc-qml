#include <QStringList>
#include <QTcpSocket>
#include <QDateTime>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcsocket.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpchttpserver_p.h"
#include "qjsonrpchttpserver.h"

QJsonRpcHttpRequest::QJsonRpcHttpRequest(QAbstractSocket *socket, QObject *parent)
    : QIODevice(parent),
      m_requestSocket(socket),
      m_requestParser(0)
{
    // initialize request parser
    m_requestParser = (http_parser*)malloc(sizeof(http_parser));
    http_parser_init(m_requestParser, HTTP_REQUEST);
    m_requestParserSettings.on_message_begin = onMessageBegin;
    m_requestParserSettings.on_url = onUrl;
    m_requestParserSettings.on_header_field = onHeaderField;
    m_requestParserSettings.on_header_value = onHeaderValue;
    m_requestParserSettings.on_headers_complete = onHeadersComplete;
    m_requestParserSettings.on_body = onBody;
    m_requestParserSettings.on_message_complete = onMessageComplete;
    m_requestParser->data = this;

    m_requestSocket->setParent(this);
    connect(m_requestSocket, SIGNAL(readyRead()), this, SLOT(readIncomingData()));
    open(QIODevice::ReadWrite);
}

QJsonRpcHttpRequest::~QJsonRpcHttpRequest()
{
    free(m_requestParser);
}

bool QJsonRpcHttpRequest::isSequential() const
{
    return true;
}

qint64 QJsonRpcHttpRequest::readData(char *data, qint64 maxSize)
{
    int bytesRead = 0;
    if (!m_requestPayload.isEmpty()) {
        int bytesToRead = qMin(m_requestPayload.size(), (int)maxSize);
        for (int byte = 0; byte < bytesToRead; ++byte, ++bytesRead)
            data[bytesRead] = m_requestPayload[byte];
        m_requestPayload.remove(0, bytesRead);
    }

    return bytesRead;
}

qint64 QJsonRpcHttpRequest::writeData(const char *data, qint64 maxSize)
{
    m_responseBuffer.append(data, (int)maxSize);
    QJsonDocument document = QJsonDocument::fromJson(m_responseBuffer);
    if (document.isObject()) {
        // determine the HTTP code to respond with
        int statusCode = 200;
        QJsonRpcMessage message = QJsonRpcMessage::fromObject(document.object());
        switch (message.type()) {
        case QJsonRpcMessage::Error:
            switch (message.errorCode()) {
            case QJsonRpc::InvalidRequest:
                statusCode = 400;
                break;

            case QJsonRpc::MethodNotFound:
                statusCode = 404;
                break;

            default:
                statusCode = 500;
                break;
            }
            break;

        case QJsonRpcMessage::Invalid:
            statusCode = 400;
            break;

        case QJsonRpcMessage::Notification:
        case QJsonRpcMessage::Response:
        case QJsonRpcMessage::Request:
            statusCode = 200;
            break;
        }

        QTextStream os(m_requestSocket);
        os.setAutoDetectUnicode(true);

        // header
        os << "HTTP/1.1 " << QByteArray::number(statusCode) << " OK\r\n";
        os << "Content-Type: application/json-rpc\r\n"
           << "\r\n";

        // body
        os << m_responseBuffer;
        m_requestSocket->close();

        // then clear the buffer
        m_responseBuffer.clear();
    }

    return maxSize;
}

void QJsonRpcHttpRequest::readIncomingData()
{
    QByteArray requestBuffer = m_requestSocket->readAll();
    http_parser_execute(m_requestParser, &m_requestParserSettings,
                        requestBuffer.constData(), requestBuffer.size());
}

int QJsonRpcHttpRequest::onBody(http_parser *parser, const char *at, size_t length)
{
    QJsonRpcHttpRequest *request = (QJsonRpcHttpRequest *)parser->data;
    request->m_requestPayload = QByteArray(at, length);
    return 0;
}

int QJsonRpcHttpRequest::onMessageComplete(http_parser *parser)
{
    QJsonRpcHttpRequest *request = (QJsonRpcHttpRequest *)parser->data;
    Q_EMIT request->readyRead();
    return 0;
}

int QJsonRpcHttpRequest::onHeadersComplete(http_parser *parser)
{
    QJsonRpcHttpRequest *request = (QJsonRpcHttpRequest *)parser->data;
    if (parser->method != HTTP_GET && parser->method != HTTP_POST) {
        // NOTE: close the socket, cleanup, delete, etc..
        qDebug() << "invalid method: " << parser->method;
        return -1;
    }

    // check headers
    // see: http://www.jsonrpc.org/historical/json-rpc-over-http.html#http-header
    if (!request->m_requestHeaders.contains("Content-Type") ||
        !request->m_requestHeaders.contains("Content-Length") ||
        !request->m_requestHeaders.contains("Accept")) {
        // NOTE: signal the error somehow
        return -1;
    }

    QStringList supportedContentTypes =
        QStringList() << "application/json-rpc" << "application/json" << "application/jsonrequest";
    QString contentType = request->m_requestHeaders.value("Content-Type");
    QString acceptType = request->m_requestHeaders.value("Accept");
    if (!supportedContentTypes.contains(contentType) || !supportedContentTypes.contains(acceptType)) {
        // NOTE: signal the error
        return -1;
    }

    return 0;
}

int QJsonRpcHttpRequest::onHeaderField(http_parser *parser, const char *at, size_t length)
{
    QJsonRpcHttpRequest *request = (QJsonRpcHttpRequest *)parser->data;
    if (!request->m_currentHeaderField.isEmpty() && !request->m_currentHeaderValue.isEmpty()) {
        request->m_requestHeaders.insert(request->m_currentHeaderField, request->m_currentHeaderValue);
        request->m_currentHeaderField.clear();
        request->m_currentHeaderValue.clear();
    }

    request->m_currentHeaderField.append(QString::fromLatin1(at, length));
    return 0;
}

int QJsonRpcHttpRequest::onHeaderValue(http_parser *parser, const char *at, size_t length)
{
    QJsonRpcHttpRequest *request = (QJsonRpcHttpRequest *)parser->data;
    request->m_currentHeaderValue.append(QString::fromLatin1(at, length));
    return 0;
}

int QJsonRpcHttpRequest::onMessageBegin(http_parser *parser)
{
    QJsonRpcHttpRequest *request = (QJsonRpcHttpRequest *)parser->data;
    request->m_requestHeaders.clear();
    return 0;
}

int QJsonRpcHttpRequest::onUrl(http_parser *parser, const char *at, size_t length)
{
    Q_UNUSED(parser)
    Q_UNUSED(at)
    Q_UNUSED(length)
//    QString url = QString::fromLatin1(at, length);
//    qDebug() << "requested url: " << url;

    return 0;
}

QJsonRpcHttpServer::QJsonRpcHttpServer(QObject *parent)
    : QTcpServer(parent)
{
}

QJsonRpcHttpServer::~QJsonRpcHttpServer()
{
}

QSslConfiguration QJsonRpcHttpServer::sslConfiguration() const
{
    return m_sslConfiguration;
}

void QJsonRpcHttpServer::setSslConfiguration(const QSslConfiguration &config)
{
    m_sslConfiguration = config;
}

#if QT_VERSION >= 0x050000
void QJsonRpcHttpServer::incomingConnection(qintptr socketDescriptor)
#else
void QJsonRpcHttpServer::incomingConnection(int socketDescriptor)
#endif
{
    QJsonRpcHttpRequest *request;
    if (m_sslConfiguration.isNull()) {
        QTcpSocket *socket = new QTcpSocket;
        socket->setSocketDescriptor(socketDescriptor);
        request = new QJsonRpcHttpRequest(socket, this);
    } else {
        QSslSocket *socket = new QSslSocket;
        socket->setSocketDescriptor(socketDescriptor);
        socket->setSslConfiguration(m_sslConfiguration);
        socket->startServerEncryption();
        // connect ssl error signals etc

        // NOTE: unsafe
        connect(socket, SIGNAL(sslErrors(QList<QSslError>)), socket, SLOT(ignoreSslErrors()));
        request = new QJsonRpcHttpRequest(socket, this);
    }

    QJsonRpcSocket *rpcSocket = new QJsonRpcSocket(request, this);
    m_requests.insert(rpcSocket, request);
    connect(rpcSocket, SIGNAL(messageReceived(QJsonRpcMessage)),
                 this, SLOT(processIncomingMessage(QJsonRpcMessage)));
}

void QJsonRpcHttpServer::processIncomingMessage(const QJsonRpcMessage &message)
{
    QJsonRpcSocket *socket = qobject_cast<QJsonRpcSocket*>(sender());
    if (!socket)
        return;

    processMessage(socket, message);
}

int QJsonRpcHttpServer::connectedClientCount() const
{
    return 0;
}

void QJsonRpcHttpServer::notifyConnectedClients(const QJsonRpcMessage &message)
{
    Q_UNUSED(message);
}

void QJsonRpcHttpServer::notifyConnectedClients(const QString &method, const QJsonArray &params)
{
    Q_UNUSED(method);
    Q_UNUSED(params);
}
