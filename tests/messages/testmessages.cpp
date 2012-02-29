#include <QtCore/QVariant>

#include <QtTest/QtTest>

class TestMessages: public QObject
{
    Q_OBJECT  
private slots:
    void testOne();

};

void TestMessages::testOne()
{
    QCOMPARE(1, 1);
}

QTEST_MAIN(TestMessages)
#include "testmessages.moc"
