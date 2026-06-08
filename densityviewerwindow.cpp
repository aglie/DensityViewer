#include "densityviewerwindow.h"

#include "densityviewer.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
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
#include <QSignalBlocker>
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

    loadFileFromPath(filename);
}

void DensityViewerWindow::reloadActiveDataset() {
    if(!activePanel)
        return;
    QString path = activePanel->property("file").toString();
    if(path.isEmpty())
        return;
    try {
        activePanel->updateDataset(path);
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
    configureLoadedPanel(activePanel);
    if(splitView)
        rebuildSplit();
}

void DensityViewerWindow::setXLimits() {
    if(!activePanel || !activePanel->hasData())
        return;
    int axis = activePanel->currentSection.sectionDir;
    sectionIndex->setMaximum(activePanel->data.upperLimit(axis));
    sectionIndex->setMinimum(activePanel->data.lowerLimit(axis));
    sectionIndex->setSingleStep(activePanel->data.stepSize(axis));
}

void DensityViewerWindow::fillInHKX() {
    if(!activePanel || !activePanel->hasData())
        return;
    string hkl = axesNames(activePanel->data.isDirect);

    sectionComboBox->clear();
    for (int i : {2,1,0}) {
        string t = hkl;
        t[i]='x';
        sectionComboBox->addItem(QString::fromStdString(t));
    }
}

void DensityViewerWindow::updateControls() {
    sectionIndex->setEnabled(true);
    colorSaturation->setEnabled(true);
    sectionComboBox->setEnabled(true);
    colormapComboBox->setEnabled(true);

    fillInHKX();
    setXLimits();
    updateProgramTitle();
}

// ---------------------------------------------------------------------------
// Comparison-panel plumbing
// ---------------------------------------------------------------------------

QList<DensityViewer*> DensityViewerWindow::syncTargets() {
    QList<DensityViewer*> t = panels;
    if(splitView)
        t << splitViewer;
    return t;
}

void DensityViewerWindow::applyToTargets(const std::function<void(DensityViewer*)>& fn) {
    for(auto v : syncTargets())
        if(v->hasData())
            fn(v);
}

// Mirror the pan/zoom of `source` onto every other synced viewer.
void DensityViewerWindow::broadcastView(DensityViewer* source) {
    if(!source->hasData())
        return;
    QPointF c = source->viewCenterHkl();
    double z = source->viewZoom();
    for(auto v : syncTargets()) {
        if(v==source || !v->hasData())
            continue;
        QSignalBlocker b(v);
        v->setViewState(c, z);
    }
}

// Show a crosshair at the hovered hkl in the OTHER panels and refresh the readout.
// The hovered (source) panel already has the live OS cursor, so skip it.
void DensityViewerWindow::broadcastCursor(const vector<double>& hkl, DensityViewer* source) {
    for(auto v : syncTargets())
        if(v->hasData())
            v->setCursorMarker(hkl, v != source);
    updateCursorReadout(hkl);
}

void DensityViewerWindow::updateCursorReadout(const vector<double>& hkl) {
    if(!activePanel || !activePanel->hasData() || hkl.size()<3)
        return;
    string names = axesNames(activePanel->data.isDirect);
    ostringstream res;
    res << std::setprecision(3);
    res << names[0] << "=" << hkl[0] << "  "
        << names[1] << "=" << hkl[1] << "  "
        << names[2] << "=" << hkl[2] << "\n";

    if(splitView) {
        res << "value=" << splitViewer->intensityAtHkl(hkl);
    } else if(panels.size()==1) {
        res << "intensity=" << panels[0]->intensityAtHkl(hkl);
    } else {
        for(int i=0;i<panels.size();++i) {
            if(!panels[i]->hasData())
                continue;
            QString nm = QFileInfo(panels[i]->property("file").toString()).fileName();
            if(nm.isEmpty())
                nm = QString("panel %1").arg(i+1);
            res << nm.toStdString() << ": " << panels[i]->intensityAtHkl(hkl) << "\n";
        }
    }
    coordinateCursor->setText(QString::fromStdString(res.str()));
}

// Show either the single shared saturation control (scales linked) or one
// saturation control per panel (scales unlinked, >=2 panels) so different
// exposures can be tuned independently.
void DensityViewerWindow::rebuildSaturationControls() {
    bool perPanel = !linkScalesCheck->isChecked() && panels.size()>=2;
    masterSatRow->setVisible(!perPanel);
    perPanelSatBox->setVisible(perPanel);

    auto lay = static_cast<QVBoxLayout*>(perPanelSatBox->layout());
    QLayoutItem* item;
    while((item = lay->takeAt(0)) != nullptr) {
        if(item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    if(!perPanel)
        return;

    for(int i=0;i<panels.size();++i) {
        DensityViewer* p = panels[i];
        QString nm = QFileInfo(p->property("file").toString()).fileName();
        if(nm.isEmpty())
            nm = QString("Panel %1").arg(i+1);
        auto lbl = new QLabel(QString("Saturation — %1").arg(nm));
        lbl->setStyleSheet("color:#a0a0a0; font-size:11px;");
        lbl->setToolTip(nm);
        auto spin = new QDoubleSpinBox;
        spin->setMaximum(INFINITY);
        spin->setMinimum(0);
        spin->setDecimals(4);
        spin->setSingleStep(1);
        spin->setValue(p->hasData() ? p->colorSaturationValue() : 100);
        spin->setEnabled(p->hasData());
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spin->installEventFilter(this);
        connect(spin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                [this,p](double val){ if(p->hasData()){ p->setColorSaturation(val); if(splitView) rebuildSplit(); } });
        lay->addWidget(lbl);
        lay->addWidget(spin);
    }
}

// Align a freshly-loaded panel with the shared controls and the group's view.
void DensityViewerWindow::configureLoadedPanel(DensityViewer* v) {
    QSignalBlocker b(v);
    if(sectionComboBox->count()>0)
        v->setSectionDirection(sectionComboBox->currentText());
    v->setSectionIndex(sectionIndex->value());
    if(linkScalesCheck->isChecked()) {
        v->setColormap(colormapComboBox->currentText());
        v->setColorSaturation(colorSaturation->value());
    }
    v->setGrid(gridCheck->isChecked());
    v->setInteractionMode(currentTool);
    for(auto p : panels) {
        if(p!=v && p->hasData()) {
            v->setViewState(p->viewCenterHkl(), p->viewZoom());
            break;
        }
    }
}

void DensityViewerWindow::setActivePanel(DensityViewer* v) {
    if(!v)
        return;
    activePanel = v;
    for(auto p : panels)
        p->setActive(p==v && panels.size()>1);
    currentFile = v->property("file").toString();
    updateProgramTitle();
    // When scales are independent, reflect the focused panel's scale in the controls.
    if(!linkScalesCheck->isChecked() && v->hasData()) {
        QSignalBlocker b1(colorSaturation), b2(colormapComboBox);
        colorSaturation->setValue(v->colorSaturationValue());
        int idx = colormapComboBox->findText(v->colormapName());
        if(idx>=0)
            colormapComboBox->setCurrentIndex(idx);
    }
    removePanelBtn->setEnabled(panels.size()>1);
    // Keep the section-index range matched to the focused panel's data.
    if(v->hasData())
        setXLimits();
}

DensityViewer* DensityViewerWindow::createPanel() {
    auto v = new DensityViewer;
    connect(v, &DensityViewer::panelActivated, this, [this,v](){ setActivePanel(v); });
    connect(v, &DensityViewer::viewChanged,    this, [this,v](){ broadcastView(v); });
    connect(v, &DensityViewer::changedSectionDirection, this, &DensityViewerWindow::setXLimits);
    connect(v, &DensityViewer::dataCursorMoved, this,
            [this,v](int, int, vector<double> hkl, string){ broadcastCursor(hkl, v); });
    return v;
}

// Place the panels into the 2x2 grid: first two in the top row, next two below.
void DensityViewerWindow::relayoutPanels() {
    for(int i=0;i<panels.size();++i) {
        QSplitter* row = (i<2) ? topRow : bottomRow;
        row->addWidget(panels[i]);     // reparents into the right row, in order
    }
    bottomRow->setVisible(panels.size() > 2);
    auto equalize = [](QSplitter* s){
        if(s->count()<=0) return;
        QList<int> sizes; for(int i=0;i<s->count();++i) sizes<<1; s->setSizes(sizes);
    };
    equalize(topRow);
    equalize(bottomRow);
    if(panels.size()>2)
        viewerSplitter->setSizes({1,1});
}

void DensityViewerWindow::addPanel() {
    if(panels.size()>=4)
        return;
    auto v = createPanel();
    panels << v;
    relayoutPanels();
    setActivePanel(v);
    removePanelBtn->setEnabled(panels.size()>1);
    rebuildSaturationControls();
}

void DensityViewerWindow::removeActivePanel() {
    if(panels.size()<=1)
        return;
    auto v = activePanel;
    panels.removeOne(v);
    v->setParent(nullptr);
    v->deleteLater();
    relayoutPanels();
    setActivePanel(panels.first());
    removePanelBtn->setEnabled(panels.size()>1);
    rebuildSaturationControls();
    if(splitView) {
        int loaded=0;
        for(auto p:panels) if(p->hasData()) loaded++;
        if(loaded<2)
            setSplitView(false);
        else
            rebuildSplit();
    }
}

void DensityViewerWindow::rebuildSplit() {
    QSignalBlocker b(splitViewer);
    QVector<DensityData*> sources;
    QStringList labels;
    QVector<double> saturations;
    QStringList colormaps;
    for(auto p : panels) {
        if(!p->hasData())
            continue;
        sources << &p->data;
        QString nm = QFileInfo(p->property("file").toString()).fileName();
        labels << (nm.isEmpty() ? QString("panel %1").arg(sources.size()) : nm);
        saturations << p->colorSaturationValue();    // each region keeps its own scale
        colormaps << p->colormapName();
    }
    if(sources.isEmpty())
        return;

    SplitLayout layout = SplitLayout::Quadrants;
    int li = splitLayoutCombo->currentIndex();
    if(li==0)      layout = SplitLayout::HalvesAxis0;
    else if(li==1) layout = SplitLayout::HalvesAxis1;

    splitViewer->configureSplit(sources, labels, saturations, colormaps, layout);
    if(sectionComboBox->count()>0)
        splitViewer->setSectionDirection(sectionComboBox->currentText());
    splitViewer->setSectionIndex(sectionIndex->value());
    splitViewer->setGrid(gridCheck->isChecked());
    splitViewer->setInteractionMode(currentTool);
}

void DensityViewerWindow::setSplitView(bool on) {
    if(on) {
        int loaded=0;
        for(auto p:panels) if(p->hasData()) loaded++;
        if(loaded<2) {
            QSignalBlocker b(splitToggleBtn);
            splitToggleBtn->setChecked(false);
            auto error = new QErrorMessage(this);
            error->showMessage("Load at least two datasets (use 'Add panel') before splitting.");
            return;
        }
    }

    splitView = on;
    {
        QSignalBlocker b(splitToggleBtn);
        splitToggleBtn->setChecked(on);
    }
    if(on)
        rebuildSplit();
    else
        splitViewer->clearSplit();

    viewerSplitter->setVisible(!on);
    splitViewer->setVisible(on);
    splitLayoutCombo->setEnabled(on);
    splitToggleBtn->setText(on ? "Split (on)" : "Split");

    // Carry the current pan/zoom across the switch.
    if(on) {
        for(auto p:panels)
            if(p->hasData()) {
                QSignalBlocker bl(splitViewer);
                splitViewer->setViewState(p->viewCenterHkl(), p->viewZoom());
                break;
            }
    } else if(activePanel && activePanel->hasData()) {
        broadcastView(activePanel);
    }
}

DensityViewerWindow::DensityViewerWindow(QWidget *parent) :
    QMainWindow(parent),
    activePanel(nullptr),
    splitView(false),
    currentTool(DensityViewerInteractionMode::pan)
{
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
    auto leftToggle = new QPushButton(QString::fromUtf8("◀"));
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

    // -- Viewer area: a splitter of side-by-side panels + a composite split view --
    viewerArea = new QWidget;
    auto viewerAreaLayout = new QVBoxLayout(viewerArea);
    viewerAreaLayout->setContentsMargins(0,0,0,0);
    viewerAreaLayout->setSpacing(0);

    // Panels are arranged in a 2x2 grid: an outer vertical splitter holding two
    // horizontal rows. Panels 1-2 go in the top row, 3-4 in the row below.
    QString splitterStyle = "QSplitter::handle { background: #3c3c40; }";
    viewerSplitter = new QSplitter(Qt::Vertical);
    viewerSplitter->setHandleWidth(2);
    viewerSplitter->setStyleSheet(splitterStyle);
    topRow = new QSplitter(Qt::Horizontal);
    topRow->setHandleWidth(2);
    topRow->setStyleSheet(splitterStyle);
    bottomRow = new QSplitter(Qt::Horizontal);
    bottomRow->setHandleWidth(2);
    bottomRow->setStyleSheet(splitterStyle);
    viewerSplitter->addWidget(topRow);
    viewerSplitter->addWidget(bottomRow);
    bottomRow->setVisible(false);
    viewerAreaLayout->addWidget(viewerSplitter, 1);

    auto firstPanel = createPanel();
    panels << firstPanel;
    topRow->addWidget(firstPanel);
    activePanel = firstPanel;

    splitViewer = new DensityViewer;
    splitViewer->hide();
    connect(splitViewer, &DensityViewer::viewChanged, this, [this](){ broadcastView(splitViewer); });
    connect(splitViewer, &DensityViewer::changedSectionDirection, this, &DensityViewerWindow::setXLimits);
    connect(splitViewer, &DensityViewer::dataCursorMoved, this,
            [this](int, int, vector<double> hkl, string){ broadcastCursor(hkl, splitViewer); });
    viewerAreaLayout->addWidget(splitViewer, 1);

    mainLayout->addWidget(viewerArea, 1);

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

    auto zoomPrimary = [this](double f){
        auto p = splitView ? splitViewer : activePanel;
        if(p && p->hasData())
            p->changeZoom(f);
    };

    auto minusButton = new QPushButton("- [O]");
    connect(minusButton,&QPushButton::clicked,[=](bool){ zoomPrimary(1./1.2); });
    controllerBar->addWidget(minusButton);
    auto plusButton = new QPushButton("+ [I]");
    connect(plusButton,&QPushButton::clicked,[=](bool){ zoomPrimary(1.2); });
    controllerBar->addWidget(plusButton);

    auto zoomInShortcut = new QShortcut(QKeySequence(Qt::Key_I), this);
    connect(zoomInShortcut, &QShortcut::activated, [=](){ zoomPrimary(1.2); });
    auto zoomOutShortcut = new QShortcut(QKeySequence(Qt::Key_O), this);
    connect(zoomOutShortcut, &QShortcut::activated, [=](){ zoomPrimary(1./1.2); });

    auto homeButton = new QPushButton("Home");
    connect(homeButton, &QPushButton::clicked, [this](){
        auto p = splitView ? splitViewer : activePanel;
        if(p && p->hasData())
            p->goHome();
    });
    controllerBar->addWidget(homeButton);

    controllerBar->addSpacing(8);

    // -- Compare section --
    auto compareLabel = new QLabel("Compare");
    compareLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(compareLabel);

    auto panelBtnRow = new QHBoxLayout;
    panelBtnRow->setSpacing(6);
    auto addPanelBtn = new QPushButton("+ Panel");
    addPanelBtn->setToolTip("Add a dataset panel (up to 4) for side-by-side comparison");
    connect(addPanelBtn, &QPushButton::clicked, [this](){ addPanel(); });
    removePanelBtn = new QPushButton("- Panel");
    removePanelBtn->setEnabled(false);
    connect(removePanelBtn, &QPushButton::clicked, [this](){ removeActivePanel(); });
    panelBtnRow->addWidget(addPanelBtn);
    panelBtnRow->addWidget(removePanelBtn);
    controllerBar->addLayout(panelBtnRow);

    splitToggleBtn = new QPushButton("Split");
    splitToggleBtn->setCheckable(true);
    splitToggleBtn->setToolTip("Stitch the loaded datasets into halves/quadrants at the mirror lines (h=0 / k=0)");
    connect(splitToggleBtn, &QPushButton::clicked, [this](bool on){ setSplitView(on); });
    controllerBar->addWidget(splitToggleBtn);

    splitLayoutCombo = new QComboBox;
    splitLayoutCombo->addItem("Halves (axis 1, e.g. h=0)");
    splitLayoutCombo->addItem("Halves (axis 2, e.g. k=0)");
    splitLayoutCombo->addItem("Quadrants (both)");
    splitLayoutCombo->setCurrentIndex(2);
    splitLayoutCombo->setEnabled(false);
    splitLayoutCombo->setFocusPolicy(Qt::ClickFocus);
    connect(splitLayoutCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            [this](int){ if(splitView) rebuildSplit(); });
    controllerBar->addWidget(splitLayoutCombo);

    linkScalesCheck = new QCheckBox("Link color scales");
    linkScalesCheck->setChecked(true);
    linkScalesCheck->setToolTip("When off, each panel keeps its own colormap/saturation (compare different exposures)");
    connect(linkScalesCheck, &QCheckBox::toggled, [this](bool on){
        if(on && activePanel && activePanel->hasData()) {
            QString cm = activePanel->colormapName();
            double cs = activePanel->colorSaturationValue();
            { QSignalBlocker b(colormapComboBox); int idx=colormapComboBox->findText(cm); if(idx>=0) colormapComboBox->setCurrentIndex(idx); }
            { QSignalBlocker b(colorSaturation); colorSaturation->setValue(cs); }
            applyToTargets([=](DensityViewer*v){ v->setColormap(cm); v->setColorSaturation(cs); });
        }
        rebuildSaturationControls();
    });
    controllerBar->addWidget(linkScalesCheck);

    controllerBar->addSpacing(8);

    // -- Tools section --
    auto toolsLabel = new QLabel("Tools");
    toolsLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(toolsLabel);

    auto toolsRow1 = new QHBoxLayout;
    toolsRow1->setSpacing(6);
    panButton = new QPushButton("Pan [P]");
    panButton->setCheckable(true);
    panButton->setChecked(true);
    zoomButton = new QPushButton("Zoom [Z]");
    zoomButton->setCheckable(true);
    toolsRow1->addWidget(panButton);
    toolsRow1->addWidget(zoomButton);
    controllerBar->addLayout(toolsRow1);

    auto selectTool = [this](DensityViewerInteractionMode mode) {
        currentTool = mode;
        panButton->setChecked(mode == DensityViewerInteractionMode::pan);
        zoomButton->setChecked(mode == DensityViewerInteractionMode::zoom);
        applyToTargets([=](DensityViewer*v){ v->setInteractionMode(mode); });
    };

    connect(panButton, &QPushButton::clicked, [=](){selectTool(DensityViewerInteractionMode::pan);});
    connect(zoomButton, &QPushButton::clicked, [=](){selectTool(DensityViewerInteractionMode::zoom);});

    auto panShortcut = new QShortcut(QKeySequence(Qt::Key_P), this);
    connect(panShortcut, &QShortcut::activated, [=](){selectTool(DensityViewerInteractionMode::pan);});
    auto zoomToolShortcut = new QShortcut(QKeySequence(Qt::Key_Z), this);
    connect(zoomToolShortcut, &QShortcut::activated, [=](){selectTool(DensityViewerInteractionMode::zoom);});

    gridCheck = new QCheckBox("Show Grid [G]");
    gridCheck->setChecked(false);
    connect(gridCheck,&QCheckBox::toggled,[this](bool on){ applyToTargets([=](DensityViewer*v){ v->setGrid(on); }); });
    controllerBar->addWidget(gridCheck);
    auto gridShortcut = new QShortcut(QKeySequence(Qt::Key_G), this);
    connect(gridShortcut, &QShortcut::activated, [=](){ gridCheck->toggle(); });

    controllerBar->addSpacing(8);

    // -- Display section --
    auto displayLabel = new QLabel("Display");
    displayLabel->setStyleSheet("color: #d4d4d4; font-weight: bold; font-size: 12px; padding-bottom: 2px;");
    controllerBar->addWidget(displayLabel);

    masterSatRow = new QWidget;
    auto masterSatLayout = new QVBoxLayout(masterSatRow);
    masterSatLayout->setContentsMargins(0,0,0,0);
    masterSatLayout->setSpacing(2);
    masterSatLayout->addWidget(new QLabel("Color saturation"));
    colorSaturation = new QDoubleSpinBox;
    colorSaturation->setMaximum(INFINITY);
    colorSaturation->setMinimum(0);
    colorSaturation->setDecimals(4);
    colorSaturation->setSingleStep(1);
    colorSaturation->setValue(100);
    colorSaturation->setEnabled(false);
    colorSaturation->setButtonSymbols(QAbstractSpinBox::NoButtons);
    colorSaturation->installEventFilter(this);
    connect(colorSaturation, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            [this](double val){
        if(linkScalesCheck->isChecked())
            applyToTargets([=](DensityViewer*v){ v->setColorSaturation(val); });
        else if(activePanel && activePanel->hasData())
            activePanel->setColorSaturation(val);
        if(splitView) rebuildSplit();
    });
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
    masterSatLayout->addLayout(csatRow);
    controllerBar->addWidget(masterSatRow);

    // Per-panel saturation controls (shown when colour scales are unlinked)
    perPanelSatBox = new QWidget;
    auto perPanelSatLayout = new QVBoxLayout(perPanelSatBox);
    perPanelSatLayout->setContentsMargins(0,0,0,0);
    perPanelSatLayout->setSpacing(2);
    perPanelSatBox->setVisible(false);
    controllerBar->addWidget(perPanelSatBox);

    controllerBar->addWidget(new QLabel("Colormap"));
    colormapComboBox = new QComboBox;
    colormapComboBox->setEnabled(false);
    colormapComboBox->setFocusPolicy(Qt::ClickFocus);
    for (const auto& cmap : Colormap::AvailableColormaps )
        colormapComboBox->addItem(QString::fromStdString(cmap.first));
    connect(colormapComboBox, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::textActivated),
            [this](QString cmap){
        if(linkScalesCheck->isChecked())
            applyToTargets([=](DensityViewer*v){ v->setColormap(cmap); });
        else if(activePanel && activePanel->hasData())
            activePanel->setColormap(cmap);
        if(splitView) rebuildSplit();
    });
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
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::textActivated),
            [this](QString sec){
        applyToTargets([=](DensityViewer*v){ QSignalBlocker b(v); v->setSectionDirection(sec); });
        setXLimits();
    });
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
    connect(sectionIndex, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            [this](double val){
        applyToTargets([=](DensityViewer*v){ QSignalBlocker b(v); v->setSectionIndex(val); });
    });
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
    connect(updateButton, &QPushButton::clicked, this, &DensityViewerWindow::reloadActiveDataset);
    controllerBar->addWidget(updateButton);
    auto reloadShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    connect(reloadShortcut, &QShortcut::activated, this, &DensityViewerWindow::reloadActiveDataset);

    // Section direction shortcuts: H -> index 0, L -> index 1, K -> index 2
    auto setSectionByIndex = [this](int idx) {
        if (sectionComboBox->count() > idx) {
            sectionComboBox->setCurrentIndex(idx);
            QString sec = sectionComboBox->currentText();
            applyToTargets([=](DensityViewer*v){ QSignalBlocker b(v); v->setSectionDirection(sec); });
            setXLimits();
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
    coordinateCursor->setWordWrap(true);
    controllerBar->addWidget(coordinateCursor);

    // Right toggle arrow (collapses/expands controls panel)
    auto rightToggle = new QPushButton(QString::fromUtf8("▶"));
    rightToggle->setStyleSheet(toggleBtnStyle);
    rightToggle->setToolTip("Toggle controls panel");
    mainLayout->addWidget(rightToggle);

    mainLayout->addWidget(sidebarWidget);

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
        leftToggle->setText(checked ? QString::fromUtf8("◀") : QString::fromUtf8("▶"));
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
        rightToggle->setText(checked ? QString::fromUtf8("▶") : QString::fromUtf8("◀"));
    });
    connect(rightToggle, &QPushButton::clicked, [=](){
        toggleSidebarAction->setChecked(!toggleSidebarAction->isChecked());
    });
    viewMenu->addAction(toggleSidebarAction);

    auto addPanelAction = new QAction("Add Comparison Panel", this);
    addPanelAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(addPanelAction, &QAction::triggered, [this](){ addPanel(); });
    viewMenu->addAction(addPanelAction);

    // Set layout in QWidget
    QWidget *inners = new QWidget();
    inners->setLayout(mainLayout);

    // Set QWidget as the central layout of the main window
    setCentralWidget(inners);
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
    if (!QFileInfo::exists(path) || !activePanel)
        return;

    try {
        activePanel->loadDensityData(path);
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

    onPanelLoaded(activePanel, path);

    auto p = QDir(path);
    p.cdUp();
    QDir::setCurrent(p.path());
    refreshFolderList();
}

void DensityViewerWindow::onPanelLoaded(DensityViewer* v, const QString& path) {
    v->setProperty("file", path);
    currentFile = path;
    setActivePanel(v);

    bool first = !sectionComboBox->isEnabled();
    if(first)
        updateControls();
    else
        setXLimits();

    configureLoadedPanel(v);
    updateProgramTitle();
    addToRecentFiles(path);
    rebuildSaturationControls();

    if(splitView)
        rebuildSplit();
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
}
