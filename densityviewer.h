#ifndef DENSITYVIEWER_H
#define DENSITYVIEWER_H

#include <QWidget>
#include <QMouseEvent>
#include "densitydata.h"
#include "colormap.h"

using namespace std;

namespace Ui {
class DensityViewer;
}

enum class DensityViewerInteractionMode {info, pan, zoom};

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
    void loadDensityData(QString);

signals:
    void loadedDensityData(DensityData &);
    void changedSectionDirection();
    void dataCursorMoved(int x, int y, vector<double> hkl, string text);

private:
    bool dataIsInitialised;

    DensityViewerInteractionMode interactionMode;

    struct panState {
        bool panning;
        QPoint lastPos;
    };
    panState p;

    struct zoomState {
        bool zoomStarted;
        QPoint zoomRectStart;
    };
    zoomState z;

    bool drawZoomRect;
    QPoint zoomRectStart;
    QPoint zoomRectEnd;

//    Ui::DensityViewer *ui;
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
