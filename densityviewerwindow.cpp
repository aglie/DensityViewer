#include "densityviewerwindow.h"
#include "ui_densityviewerwindow.h"

#include "densityviewer.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QTextEdit>

DensityViewerWindow::DensityViewerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DensityViewerWindow)
{
    ui->setupUi(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    auto densityViewer = new DensityViewer;
    mainLayout->addWidget(densityViewer);

    auto controllerBar = new QVBoxLayout;

    auto plusButton = new QPushButton(" + ");
    auto minusButton = new QPushButton(" – ");
    auto plusMinus = new QHBoxLayout;
    plusMinus->addWidget(plusButton);
    plusMinus->addWidget(minusButton);


    controllerBar->addLayout(plusMinus);

    mainLayout->addLayout(controllerBar);
    //centralWidget()->setLayout(mainLayout);

    // Set layout in QWidget
    QWidget *inners = new QWidget();
    inners->setLayout(mainLayout);

    // Set QWidget as the central layout of the main window
    setCentralWidget(inners);



}

DensityViewerWindow::~DensityViewerWindow()
{
    delete ui;
}



