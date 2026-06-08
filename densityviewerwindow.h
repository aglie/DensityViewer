#ifndef DENSITYVIEWERWINDOW_H
#define DENSITYVIEWERWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "densityviewer.h"
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QSplitter>
#include <QSettings>
#include <QList>
#include <functional>

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
    void reloadActiveDataset();
    void loadFileFromPath(const QString &path);
    void refreshFolderList();
    void addToRecentFiles(const QString &path);
    void removeRecentFile(const QPoint &pos);

private:
    QString currentFile;

    // -- Comparison panels --
    QList<DensityViewer*> panels;   // side-by-side dataset views (own the data)
    DensityViewer* activePanel;     // focused panel (target of per-panel actions)
    DensityViewer* splitViewer;     // composite m3m split view (own widget)
    QSplitter* viewerSplitter;      // holds the side-by-side panels
    QWidget* viewerArea;            // wraps splitter + splitViewer, toggles visibility
    bool splitView;                 // true when the composite split is shown
    DensityViewerInteractionMode currentTool;

    DensityViewer* createPanel();
    void onPanelLoaded(DensityViewer* v, const QString& path);
    void setActivePanel(DensityViewer* v);
    QList<DensityViewer*> syncTargets();                 // viewers that should stay locked
    void applyToTargets(const std::function<void(DensityViewer*)>& fn);
    void broadcastView(DensityViewer* source);
    void broadcastCursor(const vector<double>& hkl);
    void configureLoadedPanel(DensityViewer* v);
    void addPanel();
    void removeActivePanel();
    void setSplitView(bool on);
    void rebuildSplit();
    void updateCursorReadout(const vector<double>& hkl);

    // -- Controls --
    QComboBox* sectionComboBox;
    QDoubleSpinBox* sectionIndex;
    QDoubleSpinBox* colorSaturation;
    QComboBox* colormapComboBox;
    QCheckBox* gridCheck;
    QCheckBox* linkScalesCheck;
    QComboBox* splitLayoutCombo;
    QPushButton* splitToggleBtn;
    QPushButton* removePanelBtn;
    QPushButton* panButton;
    QPushButton* zoomButton;
    QLabel* coordinateCursor;
    QListWidget* recentFilesList;
    QListWidget* folderFilesList;
    QWidget* fileBrowserWidget;
    QWidget* sidebarWidget;
    Ui::DensityViewerWindow *ui;
    QSettings settings;
};

#endif // DENSITYVIEWERWINDOW_H
