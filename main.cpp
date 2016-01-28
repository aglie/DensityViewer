#include "densityviewer.h"
#include "densityviewerwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DensityViewerWindow w;
    w.show();

    return a.exec();
}
