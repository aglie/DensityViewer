#include "densitydata.h"
#include <QtTest/QtTest>



class TestPDFViewer: public QObject
{
    Q_OBJECT
private slots:
    void sectionTitle() {
        DensityData data("/Users/arkadiy/ag/pdf-visual/PDF-viewer/test-files/tiny_dataset.h5");
        auto res = data.extractSection("hkx", 0.).title();
        QCOMPARE(QString("hk0"),QString::fromStdString(res));
        }
};

QTEST_MAIN(TestPDFViewer)
#include "densityviewertest.moc"
