#ifndef DENSITYVIEWERWINDOW_H
#define DENSITYVIEWERWINDOW_H

#include <QMainWindow>
#include <densityviewer.h>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>

namespace Ui {
class DensityViewerWindow;
}

class DensityViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DensityViewerWindow(QWidget *parent = 0);
    ~DensityViewerWindow();

private slots:
    void setXLimits();
    void initControls();
    void fillInHKX();
    void openFile();
    void updateProgramTitle();
private:
    QString currentFile;
    QComboBox* sectionComboBox;
    DensityViewer * densityViewer;
    QDoubleSpinBox * sectionIndex;
    QSpinBox* colorSaturation;
    Ui::DensityViewerWindow *ui;
    QLabel * coordinateCursor;
};

#endif // DENSITYVIEWERWINDOW_H
