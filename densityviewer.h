#ifndef DENSITYVIEWER_H
#define DENSITYVIEWER_H

#include <QWidget>


namespace Ui {
class DensityViewer;
}

class DensityViewer : public QWidget
{
    Q_OBJECT

public:
    explicit DensityViewer(QWidget *parent = 0);
    ~DensityViewer();
    QSize sizeHint() const;

public slots:


private:
    Ui::DensityViewer *ui;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    // Here I will need some variables to Pan/Zoom go to plane (later) Periodically repeat (later)
    // Function screen_pix -> hkl index

};

#endif // DENSITYVIEWER_H
