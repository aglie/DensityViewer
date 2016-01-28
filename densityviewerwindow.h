#ifndef DENSITYVIEWERWINDOW_H
#define DENSITYVIEWERWINDOW_H

#include <QMainWindow>
#include <densityviewer.h>
#include <QLabel>

namespace Ui {
class DensityViewerWindow;
}

class DensityViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DensityViewerWindow(QWidget *parent = 0);
    ~DensityViewerWindow();

private:
    Ui::DensityViewerWindow *ui;
    QLabel * coordinateCursor;
};

#endif // DENSITYVIEWERWINDOW_H
