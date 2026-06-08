#include "densityviewer.h"

#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QFont>
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
#include <limits>


QRgb falseColor(double data, Colormap cmap, vector<double> clims, ColormapInterpolation interpolation) {
    assert(clims[0]<=clims[1]);
    if(std::isnan(data))
        data=0;

    return cmap.getColor((data-clims[0])/(clims[1]-clims[0]),interpolation);
}


namespace DensityViewerInstruments {

DoNothing::DoNothing(DensityViewer* p) :
    DensityViewerInteractionInstrument(p)
{
    parent->setMouseTracking(true);
}

Pan::Pan(DensityViewer* p) :
    DensityViewerInteractionInstrument(p),
    panning(false)
{
    parent->setMouseTracking(true);
}
void Pan::onMousePress(QMouseEvent * event) {
    panning = true;
    lastPos=event->pos();
}
void Pan::onMouseMove(QMouseEvent * event) {
    auto pos = event->pos();
    if(panning) {
        parent->pan(pos.x()-lastPos.x(),pos.y()-lastPos.y());
        lastPos=pos;
    }
}

void Pan::onMouseRelease(QMouseEvent * event) {
    auto pos = event->pos();

    parent->pan(pos.x()-lastPos.x(),pos.y()-lastPos.y());
    panning=false;
}

Zoom::Zoom(DensityViewer* p) :
    DensityViewerInteractionInstrument(p),
    drawZoomRect(false),
    zooming(false)
{
    parent->setMouseTracking(true);
}
void Zoom::onMousePress(QMouseEvent * event) {
    zooming = true;
    zoomRectStart = event->pos();
}
void Zoom::onMouseMove(QMouseEvent * event) {
    if(zooming) {
        drawZoomRect = true;
        zoomRectEnd = event->pos();
        parent->update();
    }
}
void Zoom::onMouseRelease(QMouseEvent * event) {
    if(!zooming) return;
    auto pos = event->pos();

    drawZoomRect = false;
    zooming = false;
    parent->zoomTo(QRect(zoomRectStart, pos));
}

void Zoom::paint(QPainter & painter) {
    if(drawZoomRect) {
        painter.setPen(QPen(QColor(70, 130, 200), 1, Qt::SolidLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRect(zoomRectEnd,zoomRectStart));
    }
}



Info::Info(DensityViewer* p) :
    DensityViewerInteractionInstrument(p)
{
    parent->setMouseTracking(true);
}
void Info::onMouseMove(QMouseEvent * event) {
    auto pos = event->pos();

    auto x=pos.x(), y=pos.y();
    auto hkl=parent->pix2hkl(x,y);

    auto intensity = parent->intensityAtHkl(hkl);
//    auto intensity = parent->currentSection.at(x, y);

    string hklNames = axesNames(parent->data.isDirect);

    ostringstream res;
    res << std::setprecision(3);
    res << hklNames[0] << "=" << hkl[0] << endl << hklNames[1] << "=" << hkl[1] << endl << hklNames[2] << "=" << hkl[2] << endl << "intensity=" << intensity;

    emit parent->dataCursorMoved(x, y, hkl,res.str());
}
}


DensityViewer::DensityViewer(QWidget *parent) :
    QWidget(parent),
    //data("/Users/arkadiy/ag/josh/Diffuse/Crystal2/xds/reconstruction.h5")
    dataIsInitialised(false),
    colormap(Colormap::BrewerBlackToRed),
    interactionInstrument(new DensityViewerInstruments::DoNothing(this))
{
    colorSaturation = 100;
    sectionIndex=0;
    currentSectionDirection = "hkx";
    showGrid=false;
    setInteractionMode(DensityViewerInteractionMode::pan);
    marginLeft=50;
    marginRight=20;
    marginTop=30;
    marginBottom=30;
    zoom = 1;
    x_pos = 0;
    y_pos = 0;

    currentColormapName = "BlackToRed";
    active = false;
    cursorMarkerShown = false;
    splitMode = false;
    splitLayout = SplitLayout::Quadrants;
    for(int i=0; i<4; ++i) quadrantSource[i] = nullptr;

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
    double sa = currentSection.tran.stepSizes[0];
    double sb = currentSection.tran.stepSizes[1];
    double cosab = m[0][1]/(a*b);
    double sinab = sqrt(1-cosab*cosab);

    auto pa=plottingArea();

    tran.translate(x_pos,y_pos);
    tran.translate(-pa.center().x(),-pa.center().y());
    tran*=QTransform(a*sa,0,sb*b*cosab,sb*b*sinab,0,0);
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
    painter.setPen(QPen(QColor(212, 212, 212), 2, Qt::SolidLine));
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
        painter.setPen(QColor(160, 160, 160));
        painter.drawText(this->rect(),Qt::AlignHCenter | Qt::AlignVCenter, "Data will appear here.");

    }
    else
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        auto imTransform=imageTransform();
        painter.setWorldTransform(imageTransform());
        painter.drawImage(0,0,sectionImage);

        // The axes start



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


        QColor marginColor(45, 45, 48);
        QColor textColor(212, 212, 212);
        QColor gridColor(80, 130, 200);
        QColor borderColor(100, 100, 100);

        if(showGrid) {
            painter.setPen(QPen(gridColor, 1, Qt::DashLine));
            for (auto line : xTickLines) {
                painter.drawLine(line);
            }
            for (auto line : yTickLines) {
                painter.drawLine(line);
            }
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(marginColor));

        //Drawing margins
        painter.drawPolygon(QPolygon(QRect(0,0,width(),height())).subtracted(pa));

        //Draw figure title
        painter.setPen(QPen(textColor, 1, Qt::SolidLine));
        painter.drawText(QRect(pa.left(),0,pa.width(),pa.top()),Qt::AlignHCenter | Qt::AlignVCenter, QString::fromStdString(currentSection.title()));
        //currentSection

        // Figure out the positions and draw the tick labels
        painter.setPen(QPen(textColor, 1, Qt::SolidLine));

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

        painter.setPen(QPen(borderColor, 2, Qt::SolidLine));

        painter.setBrush(Qt::NoBrush);
        painter.drawRect(pa.adjusted(-1,-1,1,1));


        interactionInstrument->paint(painter);

        // -- Symmetry-aware split: draw mirror seams and per-region labels --
        if(splitMode) {
            double n0 = currentSection.size[0], n1 = currentSection.size[1];

            // Index range [lo,hi] of one side of an axis' zero crossing.
            auto sideRange = [&](int axis, int side, double& lo, double& hi){
                double n = currentSection.size[axis];
                double c = max(0.0, min(currentSection.tran.transformAxisInv(axis,0.0), n));
                bool stepPos = currentSection.tran.stepSizes[axis] > 0;
                bool negFirst = stepPos;            // side 0 (coord<0) at low indices?
                if((side==0)==negFirst) { lo=0; hi=c; } else { lo=c; hi=n; }
            };

            painter.save();
            painter.setClipRect(pa);
            bool drawAxis0 = (splitLayout!=SplitLayout::HalvesAxis1);
            bool drawAxis1 = (splitLayout!=SplitLayout::HalvesAxis0);
            painter.setPen(QPen(QColor(255,255,255,190), 1.5, Qt::DashLine));
            if(drawAxis0) {
                double c0 = currentSection.tran.transformAxisInv(0,0.0);
                if(c0>0 && c0<n0)
                    painter.drawLine(imTransform.map(QPointF(c0,0)), imTransform.map(QPointF(c0,n1)));
            }
            if(drawAxis1) {
                double c1 = currentSection.tran.transformAxisInv(1,0.0);
                if(c1>0 && c1<n1)
                    painter.drawLine(imTransform.map(QPointF(0,c1)), imTransform.map(QPointF(n0,c1)));
            }
            painter.restore();

            // Source index feeding a given quadrant (mirrors resolveQuadrants()).
            auto srcIndex = [&](int q)->int {
                int n = splitSources.size(); if(n==0) return -1;
                int s0=q&1, s1=(q>>1)&1, idx=q;
                if(splitLayout==SplitLayout::HalvesAxis0) idx=s0;
                else if(splitLayout==SplitLayout::HalvesAxis1) idx=s1;
                return max(0, min(idx, n-1));
            };
            auto mid=[&](int axis,int side){ double lo,hi; sideRange(axis,side,lo,hi); return (lo+hi)/2; };

            // Regions to label: halves -> 2, quadrants -> 4.
            struct Lbl{ double i,j; int idx; };
            vector<Lbl> lbls;
            if(splitLayout==SplitLayout::HalvesAxis0) {
                lbls.push_back({mid(0,0), n1/2, srcIndex(0)});
                lbls.push_back({mid(0,1), n1/2, srcIndex(1)});
            } else if(splitLayout==SplitLayout::HalvesAxis1) {
                lbls.push_back({n0/2, mid(1,0), srcIndex(0)});
                lbls.push_back({n0/2, mid(1,1), srcIndex(2)});
            } else {
                for(int q=0;q<4;++q) lbls.push_back({mid(0,q&1), mid(1,(q>>1)&1), srcIndex(q)});
            }

            painter.save();
            painter.setClipRect(pa);
            QFont lf = painter.font(); lf.setPointSize(10); lf.setBold(true); painter.setFont(lf);
            for(const auto& l : lbls) {
                if(l.idx<0 || l.idx>=splitLabels.size()) continue;
                QPointF sp = imTransform.map(QPointF(l.i, l.j));
                QString text = splitLabels.at(l.idx);
                QRectF box(sp.x()-90, sp.y()-11, 180, 22);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(20,20,20,160));
                QRectF tb = painter.boundingRect(box, Qt::AlignCenter, text).adjusted(-4,-2,4,2);
                painter.drawRoundedRect(tb, 3, 3);
                painter.setPen(QColor(245,245,245));
                painter.drawText(box, Qt::AlignCenter, text);
            }
            painter.restore();
        }

        // -- Cross-panel cursor marker (hkl hovered in any synced panel) --
        if(cursorMarkerShown && cursorMarkerHkl.size()>=3) {
            auto axes = sectionAxes(currentSectionDirection);
            double fi = currentSection.tran.transformAxisInv(0, cursorMarkerHkl[axes[0]]);
            double fj = currentSection.tran.transformAxisInv(1, cursorMarkerHkl[axes[1]]);
            QPointF sp = imTransform.map(QPointF(fi+0.5, fj+0.5));
            if(pa.contains(sp.toPoint())) {
                painter.save();
                painter.setClipRect(pa);
                painter.setPen(QPen(QColor(255,230,120), 1.2));
                double r=6;
                painter.drawLine(QPointF(sp.x()-r,sp.y()), QPointF(sp.x()+r,sp.y()));
                painter.drawLine(QPointF(sp.x(),sp.y()-r), QPointF(sp.x(),sp.y()+r));
                painter.drawEllipse(sp, 3, 3);
                painter.restore();
            }
        }
    }

    // -- Focused-panel highlight (drawn over everything, identity transform) --
    if(active) {
        painter.setWorldTransform(QTransform());
        painter.setPen(QPen(QColor(70,130,200), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1,1,-2,-2));
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
    emit viewChanged();
}

void DensityViewer::pan(int dx,int dy) { //panning in screen pixels
    auto tran = imageTransform();

    QLineF tline = tran.inverted().map(QLineF(0,0,dx,dy));

    x_pos+=tline.dx();
    y_pos+=tline.dy();
    update();
    emit viewChanged();
}

// Image-space (index) coordinate sitting under the centre of the plotting area.
// Used to mirror the view onto sibling panels regardless of widget size.
QPointF DensityViewer::viewCenterImg() {
    return imageTransform().inverted().map(QPointF(plottingArea().center()));
}

// Apply a view broadcast from another panel: match the zoom and slide so that
// imgCenter lands under the plot centre. Reuses pan() for the index->screen math.
void DensityViewer::setViewState(QPointF imgCenter, double zoomFactor) {
    zoom = zoomFactor;
    QPointF cur = imageTransform().map(imgCenter);
    QPointF want = plottingArea().center();
    pan(qRound(want.x()-cur.x()), qRound(want.y()-cur.y()));
}

void DensityViewer::setActive(bool a) {
    if(active==a) return;
    active = a;
    update();
}

void DensityViewer::setCursorMarker(vector<double> hkl, bool show) {
    cursorMarkerShown = show;
    cursorMarkerHkl = hkl;
    update();
}

vector<double> DensityViewer::pix2hkl(double x_screen, double y_screen) {
    auto translate = QTransform::fromTranslate(0.5, 0.5);

    //TODO: check: does this additional translation by 0.5 pixels probably need to go to the imageItransofm()?
    auto tran = translate*imageTransform();

    auto res = tran.inverted().map(QPoint(x_screen,y_screen));

    return currentSection.ind2hkl(vector<int> {res.x(),res.y()});
}



void DensityViewer::mousePressEvent(QMouseEvent * event) {
    emit panelActivated();
    interactionInstrument->onMousePress(event);
}


void DensityViewer::mouseMoveEvent(QMouseEvent * event) {
    interactionInstrument->onMouseMove(event);

    if(dataIsInitialised) {
        auto pos = event->pos();
        auto x = pos.x(), y = pos.y();
        auto hkl = pix2hkl(x, y);
        auto intensity = intensityAtHkl(hkl);
        string hklNames = axesNames(data.isDirect);

        ostringstream res;
        res << std::setprecision(3);
        res << hklNames[0] << "=" << hkl[0] << "\n"
            << hklNames[1] << "=" << hkl[1] << "\n"
            << hklNames[2] << "=" << hkl[2] << "\n"
            << "intensity=" << intensity;

        emit dataCursorMoved(x, y, hkl, res.str());
    }
}

void DensityViewer::wheelEvent(QWheelEvent * event) {
    double degrees = event->angleDelta().y() / 8.0;
    double factor = pow(1.01, degrees);
    changeZoom(factor);
    event->accept();
}

void DensityViewer::mouseReleaseEvent(QMouseEvent * event) {
    interactionInstrument->onMouseRelease(event);
}

void DensityViewer::setInteractionMode(DensityViewerInteractionMode interactionMode) {
    if(!dataIsInitialised)
        interactionMode = DensityViewerInteractionMode::nothing;

    switch(interactionMode) {
    case DensityViewerInteractionMode::pan : {
        interactionInstrument.reset(new DensityViewerInstruments::Pan(this));
        break;
    }
    case DensityViewerInteractionMode::zoom : {
        interactionInstrument.reset(new DensityViewerInstruments::Zoom(this));
        break;
    }
    case DensityViewerInteractionMode::nothing : {
        interactionInstrument.reset(new DensityViewerInstruments::DoNothing(this));
        break;
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
    if(splitMode && !splitSources.isEmpty())
        currentSection = buildSplitSection();
    else
        currentSection = data.extractSection(currentSectionDirection,sectionIndex);
    pixelateSection();
    emit changedSectionDirection();
}

void DensityViewer::setColormap(QString cmap) {
    colormap = Colormap::AvailableColormaps.at(cmap.toStdString());
    currentColormapName = cmap;
    pixelateSection();
}


void DensityViewer::loadDensityData(QString filename, bool gohome) {
    data = DensityData(filename.toStdString());
    dataIsInitialised = true;
    // Reset to safe defaults for the new data
    currentSectionDirection = "hkx";
    sectionIndex = 0;
    updateSection();
    if(gohome)
        goHome();

    emit loadedDensityData(data);
}

void DensityViewer::updateDataset(QString filename) {
    loadDensityData(filename, false);
}

// ---------------------------------------------------------------------------
// Symmetry-aware split: stitch up to four datasets into one section, divided at
// the in-plane mirror lines (axis0 = 0 and axis1 = 0). In m-3m the principal
// planes (h=0, k=0, l=0) are all {100} mirrors, so a matching pair joins
// seamlessly and any disagreement shows up as a discontinuity at the seam.
// ---------------------------------------------------------------------------

void DensityViewer::resolveQuadrants() {
    int n = splitSources.size();
    if(n==0) { for(int i=0;i<4;++i) quadrantSource[i]=nullptr; return; }
    // quadrant index q = side0 + 2*side1, side = (coord < 0 ? 0 : 1)
    auto src = [&](int i){ return splitSources[ qBound(0, i, n-1) ]; };
    switch(splitLayout) {
    case SplitLayout::HalvesAxis0:                 // split at axis0 = 0 only
        quadrantSource[0]=src(0); quadrantSource[2]=src(0);
        quadrantSource[1]=src(1); quadrantSource[3]=src(1);
        break;
    case SplitLayout::HalvesAxis1:                 // split at axis1 = 0 only
        quadrantSource[0]=src(0); quadrantSource[1]=src(0);
        quadrantSource[2]=src(1); quadrantSource[3]=src(1);
        break;
    case SplitLayout::Quadrants:
        for(int q=0;q<4;++q) quadrantSource[q]=src(q);
        break;
    }
}

DensitySection DensityViewer::buildSplitSection() {
    resolveQuadrants();

    DensityData* base = quadrantSource[0] ? quadrantSource[0] : &data;
    DensitySection baseSec = base->extractSection(currentSectionDirection, sectionIndex);

    // Extract each distinct source's section just once.
    std::map<DensityData*, DensitySection> secs;
    secs[base] = baseSec;
    for(int q=0; q<4; ++q) {
        DensityData* s = quadrantSource[q];
        if(s && !secs.count(s))
            secs[s] = s->extractSection(currentSectionDirection, sectionIndex);
    }

    int n0 = (int)baseSec.size[0];
    int n1 = (int)baseSec.size[1];
    vector<double> stitched((size_t)n0*n1);

    for(int i=0; i<n0; ++i) {
        int side0 = (baseSec.tran.transformAxis(0, i) < 0) ? 0 : 1;
        for(int j=0; j<n1; ++j) {
            int side1 = (baseSec.tran.transformAxis(1, j) < 0) ? 0 : 1;
            int q = side0 + 2*side1;
            DensityData* s = quadrantSource[q] ? quadrantSource[q] : base;
            DensitySection& sec = secs.at(s);
            double v;
            if((int)sec.size[0]==n0 && (int)sec.size[1]==n1)
                v = sec.at(i,j);
            else
                v = std::numeric_limits<double>::quiet_NaN();  // grid mismatch
            stitched[(size_t)i*n1 + j] = v;
        }
    }

    return DensitySection(stitched, baseSec.size, baseSec.tran,
                          currentSectionDirection, base->isDirect);
}

int DensityViewer::quadrantForHkl(const vector<double>& hkl) {
    auto axes = sectionAxes(currentSectionDirection);  // in-plane axis indices
    int side0 = (hkl[axes[0]] < 0) ? 0 : 1;
    int side1 = (hkl[axes[1]] < 0) ? 0 : 1;
    return side0 + 2*side1;
}

double DensityViewer::intensityAtHkl(const vector<double>& hkl) {
    if(splitMode && !splitSources.isEmpty()) {
        resolveQuadrants();
        int q = quadrantForHkl(hkl);
        DensityData* s = quadrantSource[q] ? quadrantSource[q] : &data;
        return s->atHKL(hkl);
    }
    return data.atHKL(hkl);
}

void DensityViewer::configureSplit(QVector<DensityData*> sources, QStringList labels, SplitLayout layout) {
    splitSources = sources;
    splitLabels = labels;
    splitLayout = layout;
    splitMode = !sources.isEmpty();
    if(splitMode) {
        // Borrow the first source as the "primary" dataset so axes names, axis
        // limits and the section combo keep working through the normal paths.
        data = *sources[0];
        dataIsInitialised = true;
        resolveQuadrants();
        updateSection();
    }
}

void DensityViewer::clearSplit() {
    splitMode = false;
    splitSources.clear();
    splitLabels.clear();
    for(int i=0;i<4;++i) quadrantSource[i]=nullptr;
}

