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
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QDir>
#include <QErrorMessage>

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
    }

    currentFile = filename;
    auto p = QDir(filename);
    p.cdUp();

    QDir::setCurrent(p.path());
    updateProgramTitle();
}

void DensityViewerWindow::updateDataset() {
    try {
        densityViewer->updateDataset(currentFile);
    } catch (UnknownFormat) {
      auto error = new QErrorMessage(this);
      error->showMessage("File cannot be opened because it is in unknown format.");
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


    fillInHKX();
    setXLimits();
    updateProgramTitle();
}

DensityViewerWindow::DensityViewerWindow(QWidget *parent) :
    QMainWindow(parent)
{
    //ui->setupUi(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    densityViewer = new DensityViewer;
    mainLayout->addWidget(densityViewer);

    auto controllerBar = new QVBoxLayout;

    auto plusButton = new QPushButton("zoom in");
    connect(plusButton,&QPushButton::clicked,[=](bool){densityViewer->changeZoom(1.2);});
    auto minusButton = new QPushButton("zoom out");
    connect(minusButton,&QPushButton::clicked,[=](bool){densityViewer->changeZoom(1./1.2);});
    auto plusMinus = new QHBoxLayout;

    plusMinus->addWidget(minusButton);
    plusMinus->addWidget(plusButton);

    controllerBar->addLayout(plusMinus);

    colorSaturation = new QDoubleSpinBox;
    colorSaturation->setMaximum(INFINITY);
    colorSaturation->setSingleStep(1);
    colorSaturation->setValue(100);

    controllerBar->addWidget(new QLabel("Color saturation"));
    connect(colorSaturation,
            SIGNAL(valueChanged(double)),
            densityViewer,
            SLOT(setColorSaturation(double)));
    controllerBar->addWidget(colorSaturation);

    //http://forum.qt.io/topic/17409/solved-qt-4-7-qcombobox-custom-item-delegate-doesn-t-affect-the-current-item-displayed/3

    auto colormapComboBox = new QComboBox;
    for (const auto& cmap : Colormap::AvailableColormaps )
        colormapComboBox->addItem(QString::fromStdString(cmap.first));

    connect(colormapComboBox,SIGNAL(activated(QString)),densityViewer,SLOT(setColormap(QString)));

    controllerBar->addWidget(colormapComboBox);

    sectionComboBox = new QComboBox;

    connect(sectionComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            [=](QString sec){densityViewer->setSectionDirection( sec );});

    controllerBar->addWidget(sectionComboBox);

    auto xStretch = new QHBoxLayout;
    xStretch->addWidget(new QLabel("x="));
    sectionIndex = new QDoubleSpinBox;
    sectionIndex->setValue(0);

    connect(sectionIndex, SIGNAL(valueChanged(double)),densityViewer, SLOT(setSectionIndex(double)));
    connect(densityViewer, SIGNAL(changedSectionDirection()),this,SLOT(setXLimits()));

    xStretch->addWidget(sectionIndex);
    controllerBar->addLayout(xStretch);

    auto gridOn = new QCheckBox("grid");

    gridOn->setChecked(false);
    connect(gridOn,SIGNAL(clicked(bool)),densityViewer,SLOT(setGrid(bool)));

    controllerBar->addWidget(gridOn);

    auto infoButton = new QPushButton("info");
    connect(infoButton,
            &QPushButton::clicked,
            [=](){densityViewer->setInteractionMode(DensityViewerInteractionMode::info);});
    controllerBar->addWidget(infoButton);

    auto panButton = new QPushButton("pan");
    connect(panButton,
            &QPushButton::clicked,
            [=](){densityViewer->setInteractionMode(DensityViewerInteractionMode::pan);});
    controllerBar->addWidget(panButton);

    auto zoomButton = new QPushButton("zoom");
    connect(zoomButton,
            &QPushButton::clicked,
            [=](){densityViewer->setInteractionMode(DensityViewerInteractionMode::zoom);});
    controllerBar->addWidget(zoomButton);

    auto homeButton = new QPushButton("home");
    connect(homeButton,
            SIGNAL(pressed()),
            densityViewer,
            SLOT(goHome()));
    controllerBar->addWidget(homeButton);

    auto updateButton = new QPushButton("update");
    connect(updateButton,
            SIGNAL(pressed()),
            this,
            SLOT(updateDataset()));
    controllerBar->addWidget(updateButton);

    controllerBar->addStretch();

    coordinateCursor = new QLabel;
    coordinateCursor->setMinimumWidth(150);
    connect(densityViewer,
            &DensityViewer::dataCursorMoved,
            [=](int, int, vector<double>,string text){coordinateCursor->setText(QString::fromStdString(text));});
    controllerBar->addWidget(coordinateCursor);

    mainLayout->addLayout(controllerBar);

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

    // Set layout in QWidget
    QWidget *inners = new QWidget();
    inners->setLayout(mainLayout);

    // Set QWidget as the central layout of the main window
    setCentralWidget(inners);
    //mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    //For debug purposes
    //densityViewer->loadDensityData("/Users/arkadiy/ag/yell/yell playground/delta-pdf.h5");

}

DensityViewerWindow::~DensityViewerWindow()
{
    //delete ui;
}



