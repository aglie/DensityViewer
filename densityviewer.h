#ifndef DENSITYVIEWER_H
#define DENSITYVIEWER_H

#include <QWidget>
#include <QMouseEvent>
#include "densitydata.h"
#include "colormap.h"
#include <memory>

using namespace std;

namespace Ui {
class DensityViewer;
}

class DensityViewer;

namespace DensityViewerInstruments {
class DensityViewerInteractionInstrument {
public:
    DensityViewerInteractionInstrument (DensityViewer* p): parent(p) {}
    virtual void onMouseMove(QMouseEvent *) {}
    virtual void onMousePress(QMouseEvent *) {}
    virtual void onMouseRelease(QMouseEvent *) {}
    virtual void paint(QPainter &) {}

    virtual ~DensityViewerInteractionInstrument() {}
protected:
    DensityViewer* parent;
};

class DoNothing : public DensityViewerInteractionInstrument {
public:
    DoNothing(DensityViewer* p);
    ~DoNothing() {}
};

class Pan : public DensityViewerInteractionInstrument {
public:
    Pan(DensityViewer* p);
    void onMousePress(QMouseEvent * event);
    void onMouseMove(QMouseEvent * event);
    void onMouseRelease(QMouseEvent * event);

    ~Pan() {}
private:
    bool panning;
    QPoint lastPos;
};

class Zoom : public DensityViewerInteractionInstrument {
public:
    Zoom(DensityViewer* p);
    void onMousePress(QMouseEvent * event);
    void onMouseMove(QMouseEvent * event);
    void onMouseRelease(QMouseEvent * event);
    void paint(QPainter &);

    ~Zoom() {}
private:
    bool drawZoomRect;
    QPoint zoomRectStart, zoomRectEnd;

};

class Info : public DensityViewerInteractionInstrument {
public:
    Info(DensityViewer* p);
    ~Info() {}
    void onMouseMove(QMouseEvent * event);
};

}

enum class DensityViewerInteractionMode {nothing, info, pan, zoom};

class DensityViewer : public QWidget
{
    Q_OBJECT

public:
    explicit DensityViewer(QWidget *parent = 0);
    ~DensityViewer();
    QSize sizeHint() const;

    void changeZoom(double factor);
    void pan(int dx, int dy);

    vector<double> pix2hkl(double, double);

    DensityData data;
    DensitySection currentSection;

    QRect plottingArea();

public slots:
    void setColorSaturation(double);
    void setSectionIndex(double);
    void setSectionDirection(QString);
    void setGrid(bool);
    void setInteractionMode(DensityViewerInteractionMode m);
    void zoomTo(QRectF);
    void goHome();
    void setColormap(QString);
    void loadDensityData(QString,bool gohome=true);
    void updateDataset(QString);

signals:
    void loadedDensityData(DensityData &);
    void changedSectionDirection();
    void dataCursorMoved(int x, int y, vector<double> hkl, string text);

private:
    bool dataIsInitialised;

    unique_ptr<DensityViewerInstruments::DensityViewerInteractionInstrument> interactionInstrument;

    double zoom;
    double x_pos, y_pos;
    double colorSaturation;
    double sectionIndex; //position in hkx like things
    string currentSectionDirection;
    QTransform imageTransform();
    bool showGrid;
    QRectF contentsBoundingRect();

    void mouseMoveEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    int marginTop, marginBottom, marginLeft, marginRight;

    void updateSection();
    void pixelateSection();
    QImage sectionImage;

    Colormap colormap;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    // Here I will need some variables to Pan/Zoom go to plane (later) Periodically repeat (later)
    // Function screen_pix -> hkl index

};

#endif // DENSITYVIEWER_H
