#ifndef DENSITYVIEWER_H
#define DENSITYVIEWER_H

#include <QWidget>
#include <QMouseEvent>
#include "densitydata.h"

using namespace std;

namespace Ui {
class DensityViewer;
}

enum class Colormap {blackToRed};
enum class ColormapInterpolation {nearest, linear};

class DensityViewer : public QWidget
{
    Q_OBJECT

public:
    explicit DensityViewer(QWidget *parent = 0);
    ~DensityViewer();
    QSize sizeHint() const;

    void changeZoom(double factor);
    void pan(double dx,double dy);

    vector<double> pix2hkl(double, double);

    DensityData data;


public slots:
    void setColorSaturation(double);
    void setSectionIndex(int);
    void setSectionDirection(QString);
    void setGrid(bool);

signals:
    void dataCursorMoved(int x, int y, vector<double> hkl);

private:
    Ui::DensityViewer *ui;
    double zoom;
    double x_pos, y_pos;
    double colorSaturation;
    int sectionIndex; //position in hkx like things
    void initSpecifics();
    QString currentSectionDirection;
    QTransform imageTransform();
    bool showGrid;

    void mouseMoveEvent(QMouseEvent * event) Q_DECL_OVERRIDE;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    // Here I will need some variables to Pan/Zoom go to plane (later) Periodically repeat (later)
    // Function screen_pix -> hkl index

};

#endif // DENSITYVIEWER_H
