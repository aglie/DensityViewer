#include "densityviewer.h"

#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <cmath>
#include <QTransform>
#include <assert.h>
#include <algorithm>
#include <QLineF>
#include <sstream>
#include "colormap.h"
#include <QString>
#include <iomanip>
#include <map>


QRgb falseColor(double data, Colormap cmap, vector<double> clims, ColormapInterpolation interpolation) {
    assert(clims[0]<=clims[1]);
    if(isnan(data))
        data=0;

    return cmap.getColor((data-clims[0])/(clims[1]-clims[0]),interpolation);
}


DensityViewer::DensityViewer(QWidget *parent) :
    QWidget(parent),
    //data("/Users/arkadiy/ag/josh/Diffuse/Crystal2/xds/reconstruction.h5")
    colormap(Colormap::BrewerBlackToRed),
    dataIsInitialised(false)
{
    colorSaturation = 255;
    sectionIndex=0;
    currentSectionDirection = "hkx";
    showGrid=false;
    setInteractionMode(DensityViewerInteractionMode::pan);
    drawZoomRect = false;
    marginLeft=50;
    marginRight=2;
    marginTop=30;
    marginBottom=50;
    zoom = 1;
    x_pos = 0;
    y_pos = 0;

    //ui->setupUi(this);
}

DensityViewer::~DensityViewer()
{
    //delete ui;
}

QTransform DensityViewer::imageTransform() {
    QTransform tran;

    //in place 2d cholesky decomposition
    const auto & m = currentSection.tran.metricTensor;
    double a = sqrt(m[0][0]);
    double b = sqrt(m[1][1]);
    double cosab = m[0][1]/(a*b);
    double sinab = sqrt(1-cosab*cosab);

    auto pa=plottingArea();

    tran.translate(x_pos,y_pos);
    tran.translate(-pa.center().x(),-pa.center().y());
    tran*=QTransform(a,0,b*cosab,b*sinab,0,0);
    tran*=QTransform(zoom,0,0,zoom,0,0);
    tran*=QTransform(1,0,0,-1,0,0);

    tran*=QTransform(1,0,0,1,pa.center().x(),pa.center().y());

    return tran;
}

vector<double> makeTicks(double llvis, double ulvis, double lldset, double uldset, double minStepSize, double pixStepSize) {
    //Decide on pixel size
    double stepSize;
    vector<double> sensibleStepSizes = {0.0001,0.0002,0.0005,0.001,0.002,0.005,
                                        0.01,0.02,0.05,0.1,0.125,0.2,0.25,0.5,
                                        1,2,5,10,15,20,25,50,100,200,500,1000,2000,5000};
    if(minStepSize<=pixStepSize)
        stepSize = pixStepSize;
    else if(minStepSize<= 2*pixStepSize)
        stepSize = 2*pixStepSize;
    else {
        stepSize=sensibleStepSizes[0]; //TODO: rewrite with find_if???
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
        res << std::setprecision(4);
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

template<typename A>
A amax(const vector<A> & inp) {
    return *max_element(begin(inp),end(inp));
}
template<typename A>
A amin(const vector<A> & inp) {
    return *min_element(begin(inp),end(inp));
}

void DensityViewer::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    if(!dataIsInitialised)
    {
        painter.drawText(this->rect(),Qt::AlignHCenter | Qt::AlignVCenter, "Data will appear here.");

    }
    else
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        auto imTransform=imageTransform();
        painter.setWorldTransform(imageTransform());
        painter.drawImage(0,0,sectionImage);

        // The axes start
        auto sz = size();


        QRect pa = plottingArea();
        //figure out the x coordinates of the visible area rectangle
        QPolygon visibleAreaImCoord = imTransform.inverted().map(QPolygon(pa));


        auto getComponent = [](const QPoint & p, int coord) {
            if(coord == 0)
                return p.x();
            else
                return p.y();
        };

        auto generateTicks = [&](int axisN){

            //TODO: change implementation to use bounding Rect
            vector<double> boundaryPointProjections;
            for(int i=0; i<visibleAreaImCoord.size(); ++i) {
                const auto p = visibleAreaImCoord.point(i);
                boundaryPointProjections.push_back(currentSection.tran.transformAxis(axisN, getComponent(p,axisN)));
            }

            const int minimumSeparationBetweenTicks = 150; //pixels
            vector<int> farMostPoint(2,0);
            farMostPoint[axisN]=currentSection.size[axisN];
            auto axisStretch = imTransform.map(QLineF(0,0,farMostPoint[0],farMostPoint[1]));

            auto ll=currentSection.lowerLimit(axisN);
            auto ul=currentSection.upperLimit(axisN);

            auto minStepSize = minimumSeparationBetweenTicks/axisStretch.length()*(ul-ll);

            return makeTicks(amin(boundaryPointProjections),
                                    amax(boundaryPointProjections),
                                    ll,
                                    ul,
                                    minStepSize,
                                    currentSection.tran.stepSizes[axisN]);
        };

        painter.setWorldTransform(QTransform());

        auto gridTransform=imTransform;
        gridTransform.translate(0.5,0.5);

        auto xticks = generateTicks(0);
        auto yticks = generateTicks(1);

        vector<QLineF> xTickLines(xticks.size());
        transform(begin(xticks),end(xticks),
                  begin(xTickLines),
                  [&](double xtick){
            //Not very beautiful since in this place there is double transformation happens
                    auto xpos = currentSection.tran.transformAxisInv(0,xtick);
                    return gridTransform.map(QLineF(QPointF(xpos,-10000),(QPointF(xpos,10000))));});


        vector<QLineF> yTickLines(yticks.size());
        transform(begin(yticks),end(yticks),begin(yTickLines),[&](double ytick){
            auto ypos = currentSection.tran.transformAxisInv(1,ytick);
            return gridTransform.map(QLineF(QPointF(-1000,ypos),QPointF( 1000,ypos)));});


        if(showGrid) {
            painter.setPen(QPen(Qt::blue,1, Qt::DashLine));
            for (auto line : xTickLines) {
                painter.drawLine(line);
            }
            for (auto line : yTickLines) {
                painter.drawLine(line);
            }
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(Qt::white));

        //Drawing margins
        painter.drawPolygon(QPolygon(QRect(0,0,width(),height())).subtracted(pa));

        //Draw figure title
        painter.setPen(QPen(Qt::black,1, Qt::SolidLine));
        painter.drawText(QRect(pa.left(),0,pa.width(),pa.top()),Qt::AlignHCenter | Qt::AlignVCenter, QString::fromStdString(currentSection.title()));
        //currentSection

        // Figure out the positions and draw the tick labels
        painter.setPen(QPen(Qt::black,1, Qt::SolidLine));

        double tickMargin = 5;
        auto lowerImageBorder = QLineF(pa.bottomLeft(),pa.bottomRight());
        auto xTickGuideline = lowerImageBorder;
        xTickGuideline.translate(0,tickMargin);

        auto leftImageBorder = QLineF(pa.bottomLeft(),pa.topLeft());
        auto yTickGuideline = leftImageBorder;
        yTickGuideline.translate(-tickMargin,0);

        drawTickLables(xTickGuideline, xticks, xTickLines, painter, "top-middle");
        drawTickLables(yTickGuideline, yticks, yTickLines, painter, "middle-left");

        drawTicks(lowerImageBorder,xTickLines,painter);
        drawTicks(leftImageBorder ,yTickLines,painter);

        painter.setPen(QPen(Qt::black,2, Qt::SolidLine));

        painter.setBrush(Qt::NoBrush);
        painter.drawRect(pa.adjusted(-1,-1,1,1));


        if(drawZoomRect) {
            painter.setPen(QPen(Qt::black,1, Qt::SolidLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRect(zoomRectEnd,zoomRectStart));
        }
    }

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize DensityViewer::sizeHint() const
{
    return QSize(600, 600);
}

void DensityViewer::changeZoom(double factor) {
    zoom*=factor;
    update();
}

void DensityViewer::pan(int dx,int dy) { //panning in screen pixels
    auto tran = imageTransform();

    QLineF tline = tran.inverted().map(QLineF(0,0,dx,dy));

    x_pos+=tline.dx();
    y_pos+=tline.dy();
    update();
}

vector<double> DensityViewer::pix2hkl(double x_screen, double y_screen) {
    auto tran = imageTransform();

    auto res = tran.inverted().map(QPoint(x_screen,y_screen));

    return currentSection.ind2hkl(vector<int> {res.x(),res.y()});
}

void DensityViewer::mousePressEvent(QMouseEvent * event) {
    switch(interactionMode) {
    case DensityViewerInteractionMode::pan : {
        p.panning=true;
        p.lastPos=event->pos();
        break;
    }
    case DensityViewerInteractionMode::zoom : {
        z.zoomRectStart = event->pos();
        break;
    }
    }
}


void DensityViewer::mouseMoveEvent(QMouseEvent * event) {
    auto pos = event->pos();

    switch(interactionMode) {
    case DensityViewerInteractionMode::pan : {
        if(p.panning) {
            pan(pos.x()-p.lastPos.x(),pos.y()-p.lastPos.y());
            p.lastPos=pos;
        }
        break;
    }
    case DensityViewerInteractionMode::zoom : {
        drawZoomRect = true;
        zoomRectStart = z.zoomRectStart;
        zoomRectEnd = event->pos();
        update();
        break;
    }
    case DensityViewerInteractionMode::info : {
        auto x=pos.x(), y=pos.y();
        auto hkl=pix2hkl(x,y);

        vector<string> hklNames = axesNames(data.isDirect);

        ostringstream res;
        res << std::setprecision(3);
        res << hklNames[0] << "=" << hkl[0] << " " << hklNames[1] << "=" << hkl[1] << " " << hklNames[2] << "=" << hkl[2];

        emit dataCursorMoved(x,y,hkl,res.str());
        break;
    }
    }

}

void DensityViewer::mouseReleaseEvent(QMouseEvent * event) {
    auto pos = event->pos();
    switch(interactionMode) {
    case DensityViewerInteractionMode::pan : {
        pan(pos.x()-p.lastPos.x(),pos.y()-p.lastPos.y());
        p.panning=false;
        break;
    }
    case DensityViewerInteractionMode::zoom : {
        drawZoomRect = false;
        zoomTo(QRect(z.zoomRectStart, pos));
        break;
    }
    }
}

void DensityViewer::setInteractionMode(DensityViewerInteractionMode m) {
    interactionMode = m;
    switch(interactionMode) {
    case DensityViewerInteractionMode::pan : {
        p.panning=false;
        setMouseTracking(false);
        break;
    }
    case DensityViewerInteractionMode::zoom : {
        z.zoomStarted=false;
        setMouseTracking(false);
        break;
    }
    case DensityViewerInteractionMode::info : {
        setMouseTracking(true);
    }
    }
}

void DensityViewer::setColorSaturation(double inp) {
    colorSaturation=inp;
    pixelateSection();
}

void DensityViewer::setSectionIndex(double inp) {
    sectionIndex=inp;
    updateSection();
}

void DensityViewer::setSectionDirection(QString inp) {
    const map<string, string> synonims = {{"hkx","hkx"},
                                          {"hxl","hxl"},
                                          {"xkl","xkl"},
                                          {"uvx","hkx"},
                                          {"uxw","hxl"},
                                          {"xvw","xkl"}};

    currentSectionDirection = synonims.at(inp.toStdString());
    updateSection();
    goHome();
}

void DensityViewer::setGrid(bool inp) {
    showGrid=inp;
    update();
}

QRect DensityViewer::plottingArea() {
    return QRect (marginLeft, marginTop, width()-marginLeft-marginRight, height()-marginTop-marginBottom);
}

void DensityViewer::zoomTo(QRectF target) {
    QRect pa = plottingArea();
    QLineF dr(target.center(),pa.center());
    pan(dr.dx(),dr.dy());

    double factor = min(abs(double(pa.width())/target.width()),
                        abs(double(pa.height())/target.height()));
    changeZoom(factor);
}

QRectF DensityViewer::contentsBoundingRect() {
    auto p=QPolygonF(QRectF(0,0,currentSection.size[0],currentSection.size[1]));
    auto t=imageTransform().map(p);
    auto tt=t.boundingRect();
    return tt;
}

void DensityViewer::goHome() {
    zoomTo(contentsBoundingRect());
}

void DensityViewer::pixelateSection() {
    sectionImage = QImage(currentSection.size[0], currentSection.size[1], QImage::Format_RGB32);

    for(int i=0; i<currentSection.size[0]; ++i)
        for(int j=0; j<currentSection.size[1]; ++j) {
            auto v = currentSection.at(i,j);
            sectionImage.setPixel(i, j, falseColor(v, colormap, {-colorSaturation,colorSaturation}, ColormapInterpolation::nearest));
        }
    update();
}

void DensityViewer::updateSection() {
    currentSection = data.extractSection(currentSectionDirection,sectionIndex);
    pixelateSection();
    emit changedSectionDirection();
}

void DensityViewer::setColormap(QString cmap) {
    colormap = Colormap::AvailableColormaps.at(cmap.toStdString());
    pixelateSection();
}

void DensityViewer::loadDensityData(QString filename) {
    data = DensityData(filename.toStdString());
    dataIsInitialised = true;
    updateSection();
    goHome();

    emit loadedDensityData(data);
}
