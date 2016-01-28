#include "densityviewer.h"
#include "ui_densityviewer.h"

#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <random>
#include <QTransform>
#include <assert.h>

double fractional(double x) {
    double intpart;
    auto fractpart = modf (x , &intpart);
    return fractpart;
}

QRgb falseColor(double data, Colormap cmap, vector<double> clims, ColormapInterpolation interpolation) {
    assert(clims[0]<=clims[1]);
    double trimmedData = (data-clims[0])/(clims[1]-clims[0]);
    trimmedData = min(1.,trimmedData);
    trimmedData = max(0.,trimmedData);

    vector<int> colormap;
    switch(cmap) {
        case Colormap::blackToRed:
            colormap = {102,2,32,255,255,255,31,31,31};
            break;
        default:
            assert(false);//die
    }

    double color = trimmedData*(colormap.size()/3-1);

    switch(interpolation) {
        case ColormapInterpolation::nearest: {
            int idx = (int)round(color);
            return qRgb(colormap[3*idx],colormap[3*idx+1],colormap[3*idx+2]);
        }
        case ColormapInterpolation::linear: {
            int lidx = (int)floor(color);
            int uidx = (int)ceil(color);
            double a = fractional(color);

            return qRgb((1-a)*colormap[3*lidx]+a*colormap[3*uidx],
                        (1-a)*colormap[3*lidx+1]+a*colormap[3*uidx+1],
                        (1-a)*colormap[3*lidx+2]+a*colormap[3*uidx+2]);
        }
    }

}

void DensityViewer::initSpecifics() {
    zoom = 10;
    x_pos = 0;
    y_pos = 0;
    colorSaturation = 1;
}

DensityViewer::DensityViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DensityViewer)
{
    initSpecifics();
    setMouseTracking(true);
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
            image.setPixel(i, j, falseColor(v,Colormap::blackToRed,{-colorSaturation,colorSaturation},ColormapInterpolation::linear));
        }

    painter.setWorldTransform(QTransform(1,0,1./2,sqrt(3.)/2,0,0),true);
    painter.translate(x_pos,y_pos);
    painter.scale(zoom,zoom);


    painter.drawImage(0,0,image);
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

void DensityViewer::changeZoom(double factor) {
    zoom*=factor;
    update();
}

void DensityViewer::pan(double dx,double dy) {
    x_pos+=dx/zoom;
    y_pos+=dy/zoom;
    update();
}

vector<double> DensityViewer::pix2hkl(double x_screen, double y_screen) {
    QTransform tran;
    tran.translate(x_pos,y_pos);
    tran.scale(zoom,zoom);
    tran*=QTransform(1,0,1./2,sqrt(3.)/2,0,0);

    auto res = tran.inverted().map(QPoint(x_screen,y_screen));
    return vector<double> {double(res.x()),double(res.y())};
}

void DensityViewer::mouseMoveEvent(QMouseEvent * event) {
    auto pos = event->pos();
    emit dataCursorMoved(pos.x(),pos.y(),pix2hkl(pos.x(),pos.y()));
}

void DensityViewer::setColorSaturation(double inp) {
    colorSaturation=inp;
    update();
}
