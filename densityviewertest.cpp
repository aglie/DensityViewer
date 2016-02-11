#include <QtTest/QtTest>

class TestPDFViewer: public QObject
{
    Q_OBJECT
private slots:
    void sectionTitle() {
        QString str = "Hello";
        QVERIFY(str.toUpper() == "HELLO");
        QCOMPARE(str.toUpper(), QString("HELLO"));
    }
};

QTEST_MAIN(TestPDFViewer)
#include "densityviewertest.moc"
