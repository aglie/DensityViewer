#include "densityviewerwindow.h"

#include "densityviewer.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QTextEdit>
#include <sstream>
#include <iomanip>

#include <QComboBox>
#include <QCheckBox>
#include "colormap.h"
#include <QKeySequence>
#include <QShortcut>
#include <QKeyEvent>
#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QDir>
#include <QErrorMessage>
#include <QListWidget>
#include <QDirIterator>
#include <QFileInfo>
#include <QSplitter>
#include <QMimeData>
#include <QUrl>

void DensityViewerWindow::updateProgramTitle() {
    if(currentFile=="")
        setWindowTitle("Viewer");
    else
        setWindowTitle("Viewer: " + QDir::toNativeSeparators(currentFile));
}

void DensityViewerWindow::openFile() {
    auto filename = QFileDialog::getOpenFileName(this,
         "Open dataset", QDir::currentPath() , "Yell files (*.h5)");

    if(filename=="")
        return;

    try {
        densityViewer->loadDensityData(filename);
    } catch (UnknownFormat) {
      auto error = new QErrorMessage(this);
      error->showMessage("File cannot be opened because it is in unknown format.");
      return;
    } catch (std::exception &e) {
      auto error = new QErrorMessage(this);
      error->showMessage(QString("Error reading file: %1").arg(e.what()));
      return;
    } catch (...) {
      auto error = new QErrorMessage(this);
      error->showMessage("File cannot be opened due to an unexpected error.");
      return;
    }

    currentFile = filename;
    auto p = QDir(filename);
    p.cdUp();

    QDir::setCurrent(p.path());
    updateProgramTitle();
    addToRecentFiles(filename);
    refreshFolderList();
}

void DensityViewerWindow::updateDataset() {
    try {
        densityViewer->updateDataset(currentFile);
    } catch (UnknownFormat) {
      auto error = new QErrorMessage(this);
      error->showMessage("File cannot be opened because it is in unknown format.");
      return;
    } catch (std::exception &e) {
      auto error = new QErrorMessage(this);
      error->showMessage(QString("Error reloading file: %1").arg(e.what()));
      return;
    } catch (...) {
      auto error = new QErrorMessage(this);
      error->showMessage("Error reloading file.");
      return;
    }
}

void DensityViewerWindow::setXLimits() {
    int axis = densityViewer->currentSection.sectionDir;
    sectionIndex->setMaximum(densityViewer->data.upperLimit(axis));
    sectionIndex->setMinimum(densityViewer->data.lowerLimit(axis));
    sectionIndex->setSingleStep(densityViewer->data.stepSize(axis));
}

void DensityViewerWindow::fillInHKX() {
    string hkl = axesNames(densityViewer->data.isDirect);

    sectionComboBox->clear();
    for (int i : {2,1,0}) {
        string t = hkl;
        t[i]='x';
        sectionComboBox->addItem(QString::fromStdString(t));
    }
}

void DensityViewerWindow::updateControls() {
    //TODO: figure out the maximum value from the maximum value of the data

    sectionIndex->setEnabled(true);
    colorSaturation->setEnabled(true);
    sectionComboBox->setEnabled(true);
    colormapComboBox->setEnabled(true);

    fillInHKX();
    setXLimits();
    updateProgramTitle();
}

DensityViewerWindow::DensityViewerWindow(QWidget *parent) :
    QMainWindow(parent)
{
    //ui->setupUi(this);

    setAcceptDrops(true);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // -- File browser panel (left side) --
    fileBrowserWidget = new QWidget;
    fileBrowserWidget->setFixedWidth(180);
    fileBrowserWidget->setStyleSheet(
        "QWidget { background: #1e1e1e; }"
        "QLabel { color: #a0a0a0; font-size: 11px; }"
        "QListWidget { background: #1e1e1e; color: #c0c0c0; border: none;"
        "  font-size: 11px; outline: none; }"
        "QListWidget::item { padding: 3px 6px; }"
        "QListWidget::item:hover { background: #2d2d30; }"
        "QListWidget::item:selected { background: #4682c8; color: #fff; }"
    );
    auto fileBrowserLayout = new QVBoxLayout(fileBrowserWidget);
    fileBrowserLayout->setContentsMargins(0, 8, 0, 0);
    fileBrowserLayout->setSpacing(4);

    auto recentLabel = new QLabel("  Recent Files");
    recentLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 11px; padding: 2px 0;");
    fileBrowserLayout->addWidget(recentLabel);

    recentFilesList = new QListWidget;
    recentFilesList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(recentFilesList, &QListWidget::customContextMenuRequested, this, &DensityViewerWindow::removeRecentFile);
    connect(recentFilesList, &QListWidget::itemDoubleClicked, [=](QListWidgetItem *item){
        loadFileFromPath(item->data(Qt::UserRole).toString());
    });
    fileBrowserLayout->addWidget(recentFilesList, 1);

    auto folderLabel = new QLabel("  Current Folder");
    folderLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 11px; padding: 2px 0; border-top: 1px solid #333;");
    fileBrowserLayout->addWidget(folderLabel);

    folderFilesList = new QListWidget;
    connect(folderFilesList, &QListWidget::itemDoubleClicked, [=](QListWidgetItem *item){
        loadFileFromPath(item->data(Qt::UserRole).toString());
    });
    fileBrowserLayout->addWidget(folderFilesList, 1);

    mainLayout->addWidget(fileBrowserWidget);

    // Left toggle arrow (collapses/expands file browser)
    QString toggleBtnStyle = "QPushButton { background: #2d2d30; border: none; color: #888; font-size: 10px; padding: 0; min-width: 12px; max-width: 12px; }"
                             "QPushButton:hover { background: #3c3c40; color: #d4d4d4; }";
    auto leftToggle = new QPushButton(QString::fromUtf8("\u25C0"));
    leftToggle->setStyleSheet(toggleBtnStyle);
    leftToggle->setToolTip("Toggle file browser");
    mainLayout->addWidget(leftToggle);

    // Load saved recent files
    auto savedRecent = settings.value("recentFiles").toStringList();
    for (const auto &path : savedRecent) {
        if (QFileInfo::exists(path)) {
            auto *item = new QListWidgetItem(QFileInfo(path).fileName());
            item->setData(Qt::UserRole, path);
            item->setToolTip(path);
            recentFilesList->addItem(item);
        }
    }
    refreshFolderList();

    densityViewer = new DensityViewer;
    mainLayout->addWidget(densityViewer);

    // Sidebar widget with its own background
    sidebarWidget = new QWidget;
    sidebarWidget->setFixedWidth(200);
    sidebarWidget->setStyleSheet("QWidget { background: #252528; } QLabel { color: #a0a0a0; font-size: 11px; }");
    auto controllerBar = new QVBoxLayout(sidebarWidget);
    controllerBar->setContentsMargins(12, 12, 12, 12);
    controllerBar->setSpacing(6);

    // -- Navigation section --
    auto navLabel = new QLabel("Navigation");
    navLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(navLabel);

    auto minusButton = new QPushButton("- [O]");
    connect(minusButton,&QPushButton::clicked,[=](bool){densityViewer->changeZoom(1./1.2);});
    controllerBar->addWidget(minusButton);
    auto plusButton = new QPushButton("+ [I]");
    connect(plusButton,&QPushButton::clicked,[=](bool){densityViewer->changeZoom(1.2);});
    controllerBar->addWidget(plusButton);

    auto zoomInShortcut = new QShortcut(QKeySequence(Qt::Key_I), this);
    connect(zoomInShortcut, &QShortcut::activated, [=](){densityViewer->changeZoom(1.2);});
    auto zoomOutShortcut = new QShortcut(QKeySequence(Qt::Key_O), this);
    connect(zoomOutShortcut, &QShortcut::activated, [=](){densityViewer->changeZoom(1./1.2);});

    auto homeButton = new QPushButton("Home");
    connect(homeButton, SIGNAL(pressed()), densityViewer, SLOT(goHome()));
    controllerBar->addWidget(homeButton);

    controllerBar->addSpacing(8);

    // -- Tools section --
    auto toolsLabel = new QLabel("Tools");
    toolsLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(toolsLabel);

    auto toolsRow1 = new QHBoxLayout;
    toolsRow1->setSpacing(6);
    auto panButton = new QPushButton("Pan [P]");
    panButton->setCheckable(true);
    panButton->setChecked(true);
    auto zoomButton = new QPushButton("Zoom [Z]");
    zoomButton->setCheckable(true);
    toolsRow1->addWidget(panButton);
    toolsRow1->addWidget(zoomButton);
    controllerBar->addLayout(toolsRow1);

    auto selectTool = [=](DensityViewerInteractionMode mode) {
        panButton->setChecked(mode == DensityViewerInteractionMode::pan);
        zoomButton->setChecked(mode == DensityViewerInteractionMode::zoom);
        densityViewer->setInteractionMode(mode);
    };

    connect(panButton, &QPushButton::clicked, [=](){selectTool(DensityViewerInteractionMode::pan);});
    connect(zoomButton, &QPushButton::clicked, [=](){selectTool(DensityViewerInteractionMode::zoom);});

    auto panShortcut = new QShortcut(QKeySequence(Qt::Key_P), this);
    connect(panShortcut, &QShortcut::activated, [=](){selectTool(DensityViewerInteractionMode::pan);});
    auto zoomToolShortcut = new QShortcut(QKeySequence(Qt::Key_Z), this);
    connect(zoomToolShortcut, &QShortcut::activated, [=](){selectTool(DensityViewerInteractionMode::zoom);});

    auto gridOn = new QCheckBox("Show Grid [G]");
    gridOn->setChecked(false);
    connect(gridOn,SIGNAL(clicked(bool)),densityViewer,SLOT(setGrid(bool)));
    controllerBar->addWidget(gridOn);
    auto gridShortcut = new QShortcut(QKeySequence(Qt::Key_G), this);
    connect(gridShortcut, &QShortcut::activated, [=](){gridOn->toggle();});

    controllerBar->addSpacing(8);

    // -- Display section --
    auto displayLabel = new QLabel("Display");
    displayLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(displayLabel);

    controllerBar->addWidget(new QLabel("Color saturation"));
    colorSaturation = new QDoubleSpinBox;
    colorSaturation->setMaximum(INFINITY);
    colorSaturation->setMinimum(0);
    colorSaturation->setDecimals(4);
    colorSaturation->setSingleStep(1);
    colorSaturation->setValue(100);
    colorSaturation->setEnabled(false);
    colorSaturation->setButtonSymbols(QAbstractSpinBox::NoButtons);
    colorSaturation->installEventFilter(this);
    connect(colorSaturation, SIGNAL(valueChanged(double)),
            densityViewer, SLOT(setColorSaturation(double)));
    auto csatRow = new QHBoxLayout;
    csatRow->setSpacing(2);
    csatRow->addWidget(colorSaturation, 1);
    auto csatBtns = new QVBoxLayout;
    csatBtns->setSpacing(1);
    auto csatPlus = new QPushButton("+");
    auto csatMinus = new QPushButton("-");
    QString smallBtnStyle = "padding: 0px; min-height: 0px; max-height: 14px; min-width: 22px; font-size: 11px;";
    csatPlus->setStyleSheet(smallBtnStyle);
    csatMinus->setStyleSheet(smallBtnStyle);
    connect(csatPlus, &QPushButton::clicked, [=](){ colorSaturation->stepUp(); });
    connect(csatMinus, &QPushButton::clicked, [=](){ colorSaturation->stepDown(); });
    csatBtns->addWidget(csatPlus);
    csatBtns->addWidget(csatMinus);
    csatRow->addLayout(csatBtns);
    controllerBar->addLayout(csatRow);

    controllerBar->addWidget(new QLabel("Colormap"));
    colormapComboBox = new QComboBox;
    colormapComboBox->setEnabled(false);
    colormapComboBox->setFocusPolicy(Qt::ClickFocus);
    for (const auto& cmap : Colormap::AvailableColormaps )
        colormapComboBox->addItem(QString::fromStdString(cmap.first));
    connect(colormapComboBox,SIGNAL(activated(QString)),densityViewer,SLOT(setColormap(QString)));
    controllerBar->addWidget(colormapComboBox);

    controllerBar->addSpacing(8);

    // -- Section selection --
    auto sectionLabel = new QLabel("Section");
    sectionLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(sectionLabel);

    controllerBar->addWidget(new QLabel("Direction"));
    sectionComboBox = new QComboBox;
    sectionComboBox->setEnabled(false);
    sectionComboBox->setFocusPolicy(Qt::ClickFocus);
    connect(sectionComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            [=](QString sec){densityViewer->setSectionDirection( sec );});
    controllerBar->addWidget(sectionComboBox);

    auto xLabel = new QLabel("x =");
    xLabel->setStyleSheet("color: #d4d4d4; font-size: 12px;");
    controllerBar->addWidget(xLabel);
    sectionIndex = new QDoubleSpinBox;
    sectionIndex->setDecimals(4);
    sectionIndex->setValue(0);
    sectionIndex->setEnabled(false);
    sectionIndex->setButtonSymbols(QAbstractSpinBox::NoButtons);
    sectionIndex->installEventFilter(this);
    connect(sectionIndex, SIGNAL(valueChanged(double)),densityViewer, SLOT(setSectionIndex(double)));
    connect(densityViewer, SIGNAL(changedSectionDirection()),this,SLOT(setXLimits()));
    auto secRow = new QHBoxLayout;
    secRow->setSpacing(2);
    secRow->addWidget(sectionIndex, 1);
    auto secBtns = new QVBoxLayout;
    secBtns->setSpacing(1);
    auto secPlus = new QPushButton("+");
    auto secMinus = new QPushButton("-");
    secPlus->setStyleSheet(smallBtnStyle);
    secMinus->setStyleSheet(smallBtnStyle);
    connect(secPlus, &QPushButton::clicked, [=](){ sectionIndex->stepUp(); });
    connect(secMinus, &QPushButton::clicked, [=](){ sectionIndex->stepDown(); });
    secBtns->addWidget(secPlus);
    secBtns->addWidget(secMinus);
    secRow->addLayout(secBtns);
    controllerBar->addLayout(secRow);

    controllerBar->addSpacing(8);

    auto updateButton = new QPushButton("Reload Dataset [R]");
    connect(updateButton, SIGNAL(pressed()), this, SLOT(updateDataset()));
    controllerBar->addWidget(updateButton);
    auto reloadShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    connect(reloadShortcut, &QShortcut::activated, this, &DensityViewerWindow::updateDataset);

    // Section direction shortcuts: H→hkx (index 0), L→hxl (index 1), K→xkl (index 2)
    auto setSectionByIndex = [=](int idx) {
        if (sectionComboBox->count() > idx) {
            sectionComboBox->setCurrentIndex(idx);
            densityViewer->setSectionDirection(sectionComboBox->currentText());
        }
    };
    auto hShortcut = new QShortcut(QKeySequence(Qt::Key_H), this);
    connect(hShortcut, &QShortcut::activated, [=](){ setSectionByIndex(0); });
    auto lShortcut = new QShortcut(QKeySequence(Qt::Key_L), this);
    connect(lShortcut, &QShortcut::activated, [=](){ setSectionByIndex(1); });
    auto kShortcut = new QShortcut(QKeySequence(Qt::Key_K), this);
    connect(kShortcut, &QShortcut::activated, [=](){ setSectionByIndex(2); });

    // Layer navigation: [ = previous layer, ] = next layer
    auto layerPrevShortcut = new QShortcut(QKeySequence(Qt::Key_BracketLeft), this);
    connect(layerPrevShortcut, &QShortcut::activated, [=](){ sectionIndex->stepDown(); });
    auto layerNextShortcut = new QShortcut(QKeySequence(Qt::Key_BracketRight), this);
    connect(layerNextShortcut, &QShortcut::activated, [=](){ sectionIndex->stepUp(); });

    controllerBar->addStretch();

    // -- Coordinate info at bottom --
    auto infoLabel = new QLabel("Cursor");
    infoLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(infoLabel);

    coordinateCursor = new QLabel;
    coordinateCursor->setMinimumWidth(150);
    coordinateCursor->setStyleSheet("color: #c0c0c0; font-family: 'Menlo'; font-size: 11px; padding: 6px; background: #1e1e1e; border-radius: 4px;");
    coordinateCursor->setMinimumHeight(70);
    connect(densityViewer,
            &DensityViewer::dataCursorMoved,
            [=](int, int, vector<double>,string text){coordinateCursor->setText(QString::fromStdString(text));});
    controllerBar->addWidget(coordinateCursor);

    // Right toggle arrow (collapses/expands controls panel)
    auto rightToggle = new QPushButton(QString::fromUtf8("\u25B6"));
    rightToggle->setStyleSheet(toggleBtnStyle);
    rightToggle->setToolTip("Toggle controls panel");
    mainLayout->addWidget(rightToggle);

    mainLayout->addWidget(sidebarWidget);

    connect(densityViewer,
            SIGNAL(loadedDensityData(DensityData&)),
            this,
            SLOT(updateControls()));

    //Context menu
    auto openDataset = new QAction("&Open dataset", this);
    openDataset->setShortcuts(QKeySequence::Open);
    connect(openDataset, SIGNAL(triggered()), this, SLOT(openFile()));

    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(openDataset);

    // View menu with panel toggles
    QMenu* viewMenu = menuBar()->addMenu("&View");
    auto toggleFileBrowserAction = new QAction("File Browser", this);
    toggleFileBrowserAction->setCheckable(true);
    toggleFileBrowserAction->setChecked(true);
    toggleFileBrowserAction->setShortcut(QKeySequence("Ctrl+1"));
    connect(toggleFileBrowserAction, &QAction::toggled, [=](bool checked){
        fileBrowserWidget->setVisible(checked);
        leftToggle->setText(checked ? QString::fromUtf8("\u25C0") : QString::fromUtf8("\u25B6"));
    });
    connect(leftToggle, &QPushButton::clicked, [=](){
        toggleFileBrowserAction->setChecked(!toggleFileBrowserAction->isChecked());
    });
    viewMenu->addAction(toggleFileBrowserAction);

    auto toggleSidebarAction = new QAction("Controls Panel", this);
    toggleSidebarAction->setCheckable(true);
    toggleSidebarAction->setChecked(true);
    toggleSidebarAction->setShortcut(QKeySequence("Ctrl+2"));
    connect(toggleSidebarAction, &QAction::toggled, [=](bool checked){
        sidebarWidget->setVisible(checked);
        rightToggle->setText(checked ? QString::fromUtf8("\u25B6") : QString::fromUtf8("\u25C0"));
    });
    connect(rightToggle, &QPushButton::clicked, [=](){
        toggleSidebarAction->setChecked(!toggleSidebarAction->isChecked());
    });
    viewMenu->addAction(toggleSidebarAction);

    // Set layout in QWidget
    QWidget *inners = new QWidget();
    inners->setLayout(mainLayout);

    // Set QWidget as the central layout of the main window
    setCentralWidget(inners);

    //For debug purposes
    //densityViewer->loadDensityData("/Users/arkadiy/ag/yell/yell playground/delta-pdf.h5");

}

bool DensityViewerWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();
        // Let digits, decimal, minus, backspace, delete, arrows, enter pass to the spinbox
        if ((key >= Qt::Key_0 && key <= Qt::Key_9) ||
            key == Qt::Key_Period || key == Qt::Key_Comma ||
            key == Qt::Key_Minus || key == Qt::Key_Plus ||
            key == Qt::Key_Backspace || key == Qt::Key_Delete ||
            key == Qt::Key_Left || key == Qt::Key_Right ||
            key == Qt::Key_Up || key == Qt::Key_Down ||
            key == Qt::Key_Return || key == Qt::Key_Enter ||
            key == Qt::Key_Tab || key == Qt::Key_Escape) {
            return false;
        }
        // Letter keys: block from spinbox, re-post to main window for shortcut handling
        auto *forwarded = new QKeyEvent(keyEvent->type(), key, keyEvent->modifiers(), keyEvent->text());
        QApplication::postEvent(this, forwarded);
        return true;
    }
    return false;
}

void DensityViewerWindow::loadFileFromPath(const QString &path) {
    if (!QFileInfo::exists(path))
        return;

    try {
        densityViewer->loadDensityData(path);
    } catch (UnknownFormat) {
        auto error = new QErrorMessage(this);
        error->showMessage("File cannot be opened because it is in unknown format.");
        return;
    } catch (std::exception &e) {
        auto error = new QErrorMessage(this);
        error->showMessage(QString("Error reading file: %1").arg(e.what()));
        return;
    } catch (...) {
        auto error = new QErrorMessage(this);
        error->showMessage("File cannot be opened due to an unexpected error.");
        return;
    }

    currentFile = path;
    auto p = QDir(path);
    p.cdUp();
    QDir::setCurrent(p.path());
    updateProgramTitle();
    addToRecentFiles(path);
    refreshFolderList();
}

void DensityViewerWindow::refreshFolderList() {
    folderFilesList->clear();
    QDir dir = QDir::current();
    QStringList filters;
    filters << "*.h5";
    auto entries = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const auto &fi : entries) {
        auto *item = new QListWidgetItem(fi.fileName());
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        item->setToolTip(fi.absoluteFilePath());
        folderFilesList->addItem(item);
    }
}

void DensityViewerWindow::addToRecentFiles(const QString &path) {
    // Remove duplicates from the list widget
    for (int i = 0; i < recentFilesList->count(); ++i) {
        if (recentFilesList->item(i)->data(Qt::UserRole).toString() == path) {
            delete recentFilesList->takeItem(i);
            break;
        }
    }
    // Insert at top
    auto *item = new QListWidgetItem(QFileInfo(path).fileName());
    item->setData(Qt::UserRole, path);
    item->setToolTip(path);
    recentFilesList->insertItem(0, item);

    // Cap at 20 entries
    while (recentFilesList->count() > 20)
        delete recentFilesList->takeItem(recentFilesList->count() - 1);

    // Persist
    QStringList paths;
    for (int i = 0; i < recentFilesList->count(); ++i)
        paths << recentFilesList->item(i)->data(Qt::UserRole).toString();
    settings.setValue("recentFiles", paths);
}

void DensityViewerWindow::removeRecentFile(const QPoint &pos) {
    auto *item = recentFilesList->itemAt(pos);
    if (!item)
        return;

    QMenu menu;
    auto *removeAction = menu.addAction("Remove from list");
    if (menu.exec(recentFilesList->mapToGlobal(pos)) == removeAction) {
        delete recentFilesList->takeItem(recentFilesList->row(item));
        // Update persisted list
        QStringList paths;
        for (int i = 0; i < recentFilesList->count(); ++i)
            paths << recentFilesList->item(i)->data(Qt::UserRole).toString();
        settings.setValue("recentFiles", paths);
    }
}

void DensityViewerWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        for (const auto &url : event->mimeData()->urls()) {
            if (url.toLocalFile().endsWith(".h5", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void DensityViewerWindow::dropEvent(QDropEvent *event) {
    for (const auto &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if (path.endsWith(".h5", Qt::CaseInsensitive)) {
            loadFileFromPath(path);
            return;
        }
    }
}

DensityViewerWindow::~DensityViewerWindow()
{
    //delete ui;
}



