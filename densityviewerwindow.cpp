#include "densityviewerwindow.h"

#include "densityviewer.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QTextEdit>
#include <sstream>

#include <QComboBox>
#include <QCheckBox>
#include "colormap.h"

QString hkl2str(vector<double> hkl) {
    ostringstream res;
    res << "h=" << hkl[0] << " k=" << hkl[1] << " l=" << hkl[2];
    return QString::fromStdString(res.str());
}

void DensityViewerWindow::setXLimits() {
    int axis = densityViewer->currentSection.sectionDir;
    sectionIndex->setMaximum(densityViewer->data.upperLimit(axis));
    sectionIndex->setMinimum(densityViewer->data.lowerLimit(axis));
    sectionIndex->setSingleStep(densityViewer->data.stepSize(axis));
}

DensityViewerWindow::DensityViewerWindow(QWidget *parent) :
    QMainWindow(parent)
{
    //ui->setupUi(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    densityViewer = new DensityViewer;
    mainLayout->addWidget(densityViewer);

    auto controllerBar = new QVBoxLayout;

    auto plusButton = new QPushButton(" + ");
    connect(plusButton,&QPushButton::clicked,[=](bool){densityViewer->changeZoom(1.1);});
    auto minusButton = new QPushButton(" – ");
    connect(minusButton,&QPushButton::clicked,[=](bool){densityViewer->changeZoom(1./1.1);});
    auto plusMinus = new QHBoxLayout;

    plusMinus->addWidget(minusButton);
    plusMinus->addWidget(plusButton);

    controllerBar->addLayout(plusMinus);

    const auto disip_ampl = 100;
    auto upButton = new QPushButton("↑");
    connect(upButton,&QPushButton::clicked,[=](bool){densityViewer->pan(0,-disip_ampl);});
    auto leftButton = new QPushButton("←");
    connect(leftButton,&QPushButton::clicked,[=](bool){densityViewer->pan(-disip_ampl,0);});
    auto rightButton = new QPushButton("→");
    connect(rightButton,&QPushButton::clicked,[=](bool){densityViewer->pan(disip_ampl,0);});
    auto downButton = new QPushButton("↓");
    connect(downButton,&QPushButton::clicked,[=](bool){densityViewer->pan(0,disip_ampl);});

    controllerBar->addWidget(upButton);
    auto leftRightButtons = new QHBoxLayout;
    leftRightButtons->addWidget(leftButton);
    leftRightButtons->addWidget(rightButton);
    controllerBar->addLayout(leftRightButtons);
    controllerBar->addWidget(downButton);

    //TODO: figure out the maximum value from the maximum value of the data
    auto colorSaturation = new QSpinBox;
    colorSaturation->setMaximum(255);
    colorSaturation->setSingleStep(10);
    colorSaturation->setValue(255);

    controllerBar->addWidget(new QLabel("Color saturation"));
    connect(colorSaturation,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [=](int val) {densityViewer->setColorSaturation(val);});
    controllerBar->addWidget(colorSaturation);

    //http://forum.qt.io/topic/17409/solved-qt-4-7-qcombobox-custom-item-delegate-doesn-t-affect-the-current-item-displayed/3

    auto colormapComboBox = new QComboBox;
    for (const auto& cmap : Colormap::AvailableColormaps )
        colormapComboBox->addItem(QString::fromStdString(cmap.first));

    connect(colormapComboBox,SIGNAL(activated(QString)),densityViewer,SLOT(setColormap(QString)));

    controllerBar->addWidget(colormapComboBox);

    auto sectionComboBox = new QComboBox;
    sectionComboBox->addItem(tr("hkx"));
    sectionComboBox->addItem(tr("hxl"));
    sectionComboBox->addItem(tr("xkl"));

    connect(sectionComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            [=](QString sec){densityViewer->setSectionDirection( sec );});

    controllerBar->addWidget(sectionComboBox);

    auto xStretch = new QHBoxLayout;
    xStretch->addWidget(new QLabel("x="));
    sectionIndex = new QDoubleSpinBox;
    setXLimits();
    sectionIndex->setValue(0);

    connect(sectionIndex, SIGNAL(valueChanged(double)),densityViewer, SLOT(setSectionIndex(double)));
    connect(densityViewer, SIGNAL(changedSectionDirection()),this,SLOT(setXLimits()));

    xStretch->addWidget(sectionIndex);
    controllerBar->addLayout(xStretch);



    auto gridOn = new QCheckBox("grid");

    gridOn->setChecked(true);
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


    controllerBar->addStretch();

    coordinateCursor = new QLabel;
    coordinateCursor->setMinimumWidth(150);
    connect(densityViewer,
            &DensityViewer::dataCursorMoved,
            [=](int, int, vector<double> hkl){coordinateCursor->setText(hkl2str(hkl));});
    controllerBar->addWidget(coordinateCursor);

    mainLayout->addLayout(controllerBar);

    //centralWidget()->setLayout(mainLayout);

    // Set layout in QWidget
    QWidget *inners = new QWidget();
    inners->setLayout(mainLayout);

    // Set QWidget as the central layout of the main window
    setCentralWidget(inners);
    //mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);



}

DensityViewerWindow::~DensityViewerWindow()
{
    //delete ui;
}



