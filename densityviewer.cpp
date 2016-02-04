#include "densityviewer.h"

#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <math.h>
#include <QTransform>
#include <assert.h>
#include <math.h>
#include <algorithm>
#include <QLineF>
#include <sstream>

double fractional(double x)
{
    double intpart;
    auto fractpart = modf (x , &intpart);
    return fractpart;
}

QRgb falseColor(double data, Colormap cmap, vector<double> clims, ColormapInterpolation interpolation) {
    assert(clims[0]<=clims[1]);
    if(isnan(data))
        data=0;

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
    zoom = 1;
    x_pos = 0;
    y_pos = 0;
    colorSaturation = 255;
    data = DensityData();
    sectionIndex=0;
    currentSectionDirection = "hkx";
    showGrid=true;
}

DensityViewer::DensityViewer(QWidget *parent) :
    QWidget(parent)
{
    initSpecifics();
    setMouseTracking(true);
    //ui->setupUi(this);
}

DensityViewer::~DensityViewer()
{
    //delete ui;
}

QTransform DensityViewer::imageTransform() {
    QTransform tran;
    tran.translate(x_pos,y_pos);
    tran.scale(zoom,zoom);
    tran*=QTransform(1,0,1./2,sqrt(3.)/2,0,0);
    return tran;
}

vector<double> makeTicks(double llvis, double ulvis, double lldset, double uldset, double minStepSize, double pixStepSize) {
    //Decide on pixel size
    double stepSize;
    vector<double> sensibleStepSizes = {0.0001,0.0002,0.0005,0.001,0.002,0.005,0.01,0.02,0.05,0.1,0.125,0.2,0.25,0.5,1,2,5,10,15,20,25,50,100,200,500,1000,2000,5000};
    if(minStepSize<=pixStepSize)
        stepSize = pixStepSize;
    else if(minStepSize<= 2*pixStepSize)
        stepSize = 2*pixStepSize;
    else {
        stepSize=sensibleStepSizes[0];
        for(auto sz : sensibleStepSizes)
            if(sz<=minStepSize)
                stepSize=sz;
            else
                break;
    }


    //Generate equally spaced list of ticks

    auto ll = max(llvis,lldset);
    auto ul = min(ulvis,uldset);
    double lowerTick = ceil(ll/stepSize)*stepSize;

    vector<double> res;
    for(auto tick=lowerTick; tick<=ul; tick+=stepSize)
        res.push_back(tick);

    //TODO: get the lldset and uldset to the list of ticks, possibly kicking out some of the ticks near them
    return res;
}

void drawTickLables(QLineF & guideLine, vector<double> &xticks, vector<QLineF> &xTickLines, QPainter & painter, string alignment) {
    auto xtick = begin(xticks);
    auto xTickLine = begin(xTickLines);
    for(;xtick != end(xticks); ++xTickLine, ++xtick) {
        QPointF drawPoint;
        guideLine.intersect(*xTickLine,&drawPoint);

        ostringstream res;
        res << *xtick;
        auto formatted = QString::fromStdString(res.str());
        double textMaxLen=50;
        double textHeight=14;
        // Interesting question of corse is how to properly align axis titles over rotating axes
        // At the moment I left-middle and bottom
        if(alignment=="top-middle")
            painter.drawText(drawPoint.x()-textMaxLen/2,
                             drawPoint.y(),
                             textMaxLen,
                             textHeight,
                             Qt::AlignHCenter | Qt::AlignTop,
                             formatted);
        else if(alignment=="middle-left")
            painter.drawText(drawPoint.x()-textMaxLen,
                             drawPoint.y()-textHeight/2,
                             textMaxLen,
                             textHeight,
                             Qt::AlignRight | Qt::AlignVCenter,
                             formatted);
        else
            assert(false);
    }
}

void drawTicks(QLineF & tickBaseline, vector<QLineF> & gridLines,QPainter & painter) {
    painter.setPen(QPen(Qt::black,2, Qt::SolidLine));
    double tickLength=5;

    for(auto gline : gridLines) {
        QPointF intersection;
        auto itype = tickBaseline.intersect(gline, &intersection);
        if(itype == QLineF::BoundedIntersection) {
            auto t = gline.unitVector();
            painter.drawLine(intersection.x(),
                             intersection.y(),
                             intersection.x()+(t.p2().x()-t.p1().x())*tickLength,
                             intersection.y()+(t.p2().y()-t.p1().y())*tickLength);
        }
    }
}

void DensityViewer::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    currentSection = data.extractSection(currentSectionDirection,sectionIndex);
    QImage image(currentSection.size[0], currentSection.size[1], QImage::Format_RGB32);

    for(int i=0; i<currentSection.size[0]; ++i)
        for(int j=0; j<currentSection.size[1]; ++j) {
            auto v = currentSection.at(i,j);
            image.setPixel(i, j, falseColor(v,Colormap::blackToRed,{-colorSaturation,colorSaturation},ColormapInterpolation::linear));
        }

    painter.setRenderHint(QPainter::Antialiasing, true);
    auto imTransform=imageTransform();
    painter.setWorldTransform(imageTransform());
    painter.drawImage(0,0,image);

    // The axes start
    auto margin = 50;
    auto sz = size();

    //auto pen = QPen(Qt::blue, 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    //    //painter.setBrush(brush);

    QRect visibleArea(margin, 0, sz.width(), sz.height()-margin);
    //figure out the x coordinates of the visible area rectangle
    QPolygon visibleAreaImCoord = imTransform.inverted().map(QPolygon(visibleArea));
    vector<double> xPoints, yPoints;
    for(int i=0; i<visibleAreaImCoord.size(); ++i) {
        xPoints.push_back(visibleAreaImCoord.point(i).x());
        yPoints.push_back(visibleAreaImCoord.point(i).y());
    }

    // xPoints = [currentSection.transformAxis(0,x)]
    // yPoints = [currentSection.transformAxis(1,y)]

    painter.setWorldTransform(QTransform());

    auto gridTransform=imTransform;
    gridTransform.translate(0.5,0.5);


    auto xticks = makeTicks(*min_element(begin(xPoints),end(xPoints))/100, //max(xPoints), min(xPoints)
                            //currentSection.minX();
                            //currentSection.maxX();
                            //150 pix
                            //currentSection.xStepSize();
                            *max_element(begin(xPoints),end(xPoints))/100, 0, 1, 1./zoom, 1./100);
    auto yticks = makeTicks(*min_element(begin(yPoints),end(yPoints))/100, *max_element(begin(yPoints),end(yPoints))/100, 0, 1, 1./zoom, 1./100);

    vector<QLineF> xTickLines(xticks.size());
    transform(begin(xticks),end(xticks),
              begin(xTickLines),
              [&](double xtick){return QLineF(gridTransform.map(QPointF(xtick*99, 1000)),
                                              gridTransform.map(QPointF(xtick*99,-1000)));});
    vector<QLineF> yTickLines(yticks.size());
    transform(begin(yticks),end(yticks),begin(yTickLines),[&](double ytick){return QLineF(gridTransform.map(QPointF(-1000,ytick*99)),
                                                                                    gridTransform.map(QPointF( 1000,ytick*99)));});


    if(showGrid) {
        painter.setPen(QPen(Qt::blue,2, Qt::DotLine));
        for (auto line : xTickLines) {
            painter.drawLine(line);
        }
        for (auto line : yTickLines) {
            painter.drawLine(line);
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(Qt::white));

    painter.drawRect(QRect(0, 0, margin, sz.height()));
    painter.drawRect(QRect(0, sz.height()-margin, sz.width(), sz.height()));

    // Figure out the positions and draw the tick labels
    painter.setPen(QPen(Qt::black,1, Qt::SolidLine));

    double tickMargin = 5;
    auto lowerImageBorder = QLineF(margin,sz.height()-margin,sz.width(),sz.height()-margin);
    auto xTickGuideline = lowerImageBorder;
    xTickGuideline.translate(0,tickMargin);

    auto leftImageBorder = QLineF(margin,0,margin,sz.height()-margin);
    auto yTickGuideline = leftImageBorder;
    yTickGuideline.translate(-tickMargin,0);

    drawTickLables(xTickGuideline, xticks, xTickLines, painter, "top-middle");
    drawTickLables(yTickGuideline, yticks, yTickLines, painter, "middle-left");

    drawTicks(lowerImageBorder,xTickLines,painter);
    drawTicks(leftImageBorder ,yTickLines,painter);


    painter.setPen(QPen(Qt::black,2, Qt::SolidLine));
    if(false) {
        painter.drawLine(margin,0,margin,sz.height()-margin);
        painter.drawLine(margin,sz.height()-margin,sz.width(),sz.height()-margin);
    } else {
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(margin,1,sz.width()-margin-1,sz.height()-margin);
    }

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);




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
    auto tran = imageTransform();

    auto res = tran.inverted().map(QPoint(x_screen,y_screen));

    return currentSection.ind2hkl(vector<int> {res.x(),res.y()});
}

void DensityViewer::mouseMoveEvent(QMouseEvent * event) {
    auto pos = event->pos();
    emit dataCursorMoved(pos.x(),
                         pos.y(),
                         pix2hkl(pos.x(),pos.y()));
}

void DensityViewer::setColorSaturation(double inp) {
    colorSaturation=inp;
    update();
}

void DensityViewer::setSectionIndex(int inp) {
    sectionIndex=inp;
    update();
}

void DensityViewer::setSectionDirection(QString inp) {
    currentSectionDirection = inp;
    update();
}

void DensityViewer::setGrid(bool inp) {
    showGrid=inp;
    update();
}
