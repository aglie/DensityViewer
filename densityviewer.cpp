#include "densityviewer.h"
#include "ui_densityviewer.h"

#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <random>
#include <QTransform>

DensityViewer::DensityViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DensityViewer)
{
    ui->setupUi(this);
}

DensityViewer::~DensityViewer()
{
    delete ui;
}

void DensityViewer::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    int sz=100;
    QImage image(sz, sz, QImage::Format_RGB32);

    for(int i=0; i<sz; ++i)
        for(int j=0; j<sz; ++j) {
            int v = rand() % 255;
            image.setPixel(i, j, qRgb(v,v,v));
        }

    painter.setWorldTransform(QTransform(1,0,1./2,sqrt(3.)/2,0,0),true);
    painter.scale(5,5);

    //painter.translate(50, 50);
//    painter.rotate(60.0);
//    painter.translate(-50, -50);

    painter.drawImage(0,0,image);
   // setSizePolicy(QSizePolicy(QSizePolicy::ExpandFlag));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

//    auto pen = QPen(Qt::blue, 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

//    painter.setPen(pen);

//    //painter.setBrush(brush);
//    painter.setRenderHint(QPainter::Antialiasing, true);

//    QRect rect(10, 20, 80, 60);
//    painter.drawLine(rect.bottomLeft(), rect.topRight());
//    QPixmap img(100,100);
//    img.se
}

QSize DensityViewer::sizeHint() const
{
    return QSize(400, 400);
}

