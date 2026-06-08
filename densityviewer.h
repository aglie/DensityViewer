#ifndef DENSITYVIEWER_H
#define DENSITYVIEWER_H

#include <QWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPointF>
#include <QVector>
#include <QStringList>
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
    bool zooming;
    QPoint zoomRectStart, zoomRectEnd;

};

class Info : public DensityViewerInteractionInstrument {
public:
    Info(DensityViewer* p);
    ~Info() {}
    void onMouseMove(QMouseEvent * event);
};

}

enum class DensityViewerInteractionMode {nothing, pan, zoom};

// How a split panel stitches several datasets together. The in-plane axes'
// zero crossings (axis0=0 and axis1=0, e.g. h=0/k=0) are symmetry mirror lines
// for suitably symmetric data, so juxtaposing datasets across them is a
// meaningful agreement test.
enum class SplitLayout {HalvesAxis0, HalvesAxis1, Quadrants};

class DensityViewer : public QWidget
{
    Q_OBJECT

public:
    explicit DensityViewer(QWidget *parent = 0);
    ~DensityViewer();
    QSize sizeHint() const override;

    void changeZoom(double factor);
    void pan(int dx, int dy);

    vector<double> pix2hkl(double, double);

    DensityData data;
    DensitySection currentSection;

    QRect plottingArea();

    // -- Synchronisation accessors (used to lock views together) --
    double viewZoom() const {return zoom;}
    QPointF viewCenterHkl();                  // in-plane hkl coordinate under the plot centre
    double colorSaturationValue() const {return colorSaturation;}
    QString colormapName() const {return currentColormapName;}
    bool hasData() const {return dataIsInitialised;}
    bool isSplit() const {return splitMode;}
    double intensityAtHkl(const vector<double>& hkl);  // split-aware value lookup

    // -- Symmetry-aware split --
    void configureSplit(QVector<DensityData*> sources, QStringList labels,
                        QVector<double> saturations, QStringList colormapNames, SplitLayout layout);
    void clearSplit();

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
    void setViewState(QPointF imgCenter, double zoomFactor);  // apply a synced view
    void setActive(bool);                                     // highlight focused panel
    void setCursorMarker(vector<double> hkl, bool show);      // cross-panel cursor

signals:
    void loadedDensityData(DensityData &);
    void changedSectionDirection();
    void dataCursorMoved(int x, int y, vector<double> hkl, string text);
    void viewChanged();      // pan/zoom changed by the user
    void panelActivated();   // user clicked this panel

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
    void wheelEvent(QWheelEvent * event) Q_DECL_OVERRIDE;
    int marginTop, marginBottom, marginLeft, marginRight;

    void updateSection();
    void pixelateSection();
    QImage sectionImage;

    Colormap colormap;
    QString currentColormapName;

    bool active;             // is this the focused panel (draw highlight border)

    // Cross-panel cursor marker (hkl of the point being hovered in a sibling panel)
    bool cursorMarkerShown;
    vector<double> cursorMarkerHkl;

    // Symmetry-aware split state
    bool splitMode;
    SplitLayout splitLayout;
    QVector<DensityData*> splitSources;   // datasets feeding the split (1..4)
    QStringList splitLabels;              // filenames for in-panel annotation
    QVector<double> splitSaturations;     // per-source colour saturation
    QStringList splitColormapNames;       // per-source colormap name
    DensityData* quadrantSource[4];       // resolved dataset per quadrant
    int quadrantSourceIdx[4];             // resolved source index per quadrant
    void resolveQuadrants();
    DensitySection buildSplitSection();
    int quadrantForHkl(const vector<double>& hkl);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

    // Here I will need some variables to Pan/Zoom go to plane (later) Periodically repeat (later)
    // Function screen_pix -> hkl index

};

#endif // DENSITYVIEWER_H
