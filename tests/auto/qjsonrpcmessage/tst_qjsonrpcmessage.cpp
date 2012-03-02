#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "qjsonrpc.h"

class TestQJsonRpcMessage: public QObject
{
    Q_OBJECT  
private slots:
    void positionalParameters();
    void namedParameters();
    void notification();
    void nonexistantMethod();
    void invalidJson();
    void invalidRequest();
    void emptyArray();

};

void TestQJsonRpcMessage::positionalParameters()
{
    /*
    rpc call with positional parameters:
    --> {"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 1}
    <-- {"jsonrpc": "2.0", "result": 19, "id": 1}
    --> {"jsonrpc": "2.0", "method": "subtract", "params": [23, 42], "id": 2}
    <-- {"jsonrpc": "2.0", "result": -19, "id": 2}
    */
    QCOMPARE(1, 0);
}

void TestQJsonRpcMessage::namedParameters()
{
    /*
    rpc call with named parameters:
    --> {"jsonrpc": "2.0", "method": "subtract", "params": {"subtrahend": 23, "minuend": 42}, "id": 3}
    <-- {"jsonrpc": "2.0", "result": 19, "id": 3}
    --> {"jsonrpc": "2.0", "method": "subtract", "params": {"minuend": 42, "subtrahend": 23}, "id": 4}
    <-- {"jsonrpc": "2.0", "result": 19, "id": 4}
    */
    QCOMPARE(1, 0);
}

void TestQJsonRpcMessage::notification()
{
    /*
    a Notification:
    --> {"jsonrpc": "2.0", "method": "update", "params": [1,2,3,4,5]}
    --> {"jsonrpc": "2.0", "method": "foobar"}
    */
    QCOMPARE(1, 0);
}

void TestQJsonRpcMessage::nonexistantMethod()
{
    /*
    rpc call of non-existent method:
    --> {"jsonrpc": "2.0", "method": "foobar", "id": "1"}
    <-- {"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found."}, "id": "1"}
    */
    QCOMPARE(1, 0);
}

void TestQJsonRpcMessage::invalidJson()
{
    /*
    rpc call with invalid JSON:
    --> {"jsonrpc": "2.0", "method": "foobar, "params": "bar", "baz]
    <-- {"jsonrpc": "2.0", "error": {"code": -32700, "message": "Parse error."}, "id": null}
    */
    QCOMPARE(1, 0);
}

void TestQJsonRpcMessage::invalidRequest()
{
    /*
    rpc call with invalid Request object:
    --> {"jsonrpc": "2.0", "method": 1, "params": "bar"}
    <-- {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request."}, "id": null}
    */
    QCOMPARE(1, 0);
}

//    rpc call Batch, invalid JSON:
//    --> [ {"jsonrpc": "2.0", "method": "sum", "params": [1,2,4], "id": "1"},{"jsonrpc": "2.0", "method" ]
//    <-- {"jsonrpc": "2.0", "error": {"code": -32700, "message": "Parse error."}, "id": null}

void TestQJsonRpcMessage::emptyArray()
{
    /*
    rpc call with an empty Array:
    --> []
    <-- {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request."}, "id": null}
    */
    QCOMPARE(1, 0);
}

/*
rpc call with an invalid Batch (but not empty):
--> [1]
<-- [
          {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request."}, "id": null}
      ]

rpc call with invalid Batch:

--> [1,2,3]
<-- [
          {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request."}, "id": null},
          {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request."}, "id": null},
          {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request."}, "id": null}
      ]
rpc call Batch:

--> [
        {"jsonrpc": "2.0", "method": "sum", "params": [1,2,4], "id": "1"},
        {"jsonrpc": "2.0", "method": "notify_hello", "params": [7]},
        {"jsonrpc": "2.0", "method": "subtract", "params": [42,23], "id": "2"},
        {"foo": "boo"},
        {"jsonrpc": "2.0", "method": "foo.get", "params": {"name": "myself"}, "id": "5"},
        {"jsonrpc": "2.0", "method": "get_data", "id": "9"}
    ]
<-- [
        {"jsonrpc": "2.0", "result": 7, "id": "1"},
        {"jsonrpc": "2.0", "result": 19, "id": "2"},
        {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request."}, "id": null},
        {"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found."}, "id": "5"},
        {"jsonrpc": "2.0", "result": ["hello", 5], "id": "9"}
    ]
rpc call Batch (all notifications):

--> [
        {"jsonrpc": "2.0", "method": "notify_sum", "params": [1,2,4]},
        {"jsonrpc": "2.0", "method": "notify_hello", "params": [7]}
    ]
<-- //Nothing is returned for all notification batches
*/


QTEST_MAIN(TestQJsonRpcMessage)
#include "tst_qjsonrpcmessage.moc"
