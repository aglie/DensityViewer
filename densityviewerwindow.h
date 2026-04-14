#ifndef DENSITYVIEWERWINDOW_H
#define DENSITYVIEWERWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "densityviewer.h"
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QSettings>

namespace Ui {
class DensityViewerWindow;
}

class DensityViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DensityViewerWindow(QWidget *parent = 0);
    ~DensityViewerWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void setXLimits();
    void updateControls();
    void fillInHKX();
    void openFile();
    void updateProgramTitle();
    void updateDataset();
    void loadFileFromPath(const QString &path);
    void refreshFolderList();
    void addToRecentFiles(const QString &path);
    void removeRecentFile(const QPoint &pos);
private:
    QString currentFile;
    QComboBox* sectionComboBox;
    DensityViewer * densityViewer;
    QDoubleSpinBox * sectionIndex;
    QDoubleSpinBox* colorSaturation;
    Ui::DensityViewerWindow *ui;
    QLabel * coordinateCursor;
    QComboBox* colormapComboBox;
    QListWidget* recentFilesList;
    QListWidget* folderFilesList;
    QWidget* fileBrowserWidget;
    QWidget* sidebarWidget;
    QSettings settings;
};

#endif // DENSITYVIEWERWINDOW_H
