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

/*TODO wishlist:
 * - In reconstructions when going "home" it would be nice to zoom to the actual reconstructed plane
 * without showing tons of zeros (from the 3d plane shapes)
 *
 * - In Yell format change step_size to step_sizes
*/
