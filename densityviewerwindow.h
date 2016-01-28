#ifndef DENSITYVIEWERWINDOW_H
#define DENSITYVIEWERWINDOW_H

#include <QMainWindow>
#include <densityviewer.h>

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
};

#endif // DENSITYVIEWERWINDOW_H
