#include "densityviewerwindow.h"

#include "densityviewer.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QTextEdit>
#include <sstream>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>

QString hkl2str(vector<double> hkl) {
    ostringstream res;
    res << "h=" << hkl[0] << " k=" << hkl[1];
    return QString::fromStdString(res.str());
}

DensityViewerWindow::DensityViewerWindow(QWidget *parent) :
    QMainWindow(parent)
{
    //ui->setupUi(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    auto densityViewer = new DensityViewer;
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

    connect(colorSaturation, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int val) {densityViewer->setColorSaturation(val);});
    controllerBar->addWidget(colorSaturation);


    controllerBar->addWidget(new QLabel("X="));
    auto sectionIndex = new QSpinBox;
    sectionIndex->setMaximum(densityViewer->data.size[2]);
    sectionIndex->setSingleStep(1);
    sectionIndex->setValue(0);

    connect(sectionIndex, SIGNAL(valueChanged(int)),densityViewer, SLOT(setSectionIndex(int)));

    controllerBar->addWidget(sectionIndex);

    auto sectionComboBox = new QComboBox;
    sectionComboBox->addItem(tr("hkx"));
    sectionComboBox->addItem(tr("hxl"));
    sectionComboBox->addItem(tr("xkl"));

    connect(sectionComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            [=](QString sec){densityViewer->setSectionDirection( sec );});

    controllerBar->addWidget(sectionComboBox);

    auto gridOn = new QCheckBox("grid");

    gridOn->setChecked(true);
    connect(gridOn,SIGNAL(clicked(bool)),densityViewer,SLOT(setGrid(bool)));

    controllerBar->addWidget(gridOn);

    controllerBar->addStretch();

    coordinateCursor = new QLabel;
    connect(densityViewer,&DensityViewer::dataCursorMoved,[=](int, int, vector<double> hkl){coordinateCursor->setText(hkl2str(hkl));});
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



