#ifndef COLORMAP_H
#define COLORMAP_H

#include <QColor>
#include <vector>
#include "math.h"
#include <map>
using namespace std;

enum class ColormapInterpolation {nearest, linear};

class Colormap
{
public:
    Colormap(vector<int> colors):
    colors(colors), nColors(colors.size()/3) {}
    QRgb operator[](int idx) {
        return qRgb(colors[3*idx],colors[3*idx+1],colors[3*idx+2]);
    }

    QRgb getColor(const double& v, const ColormapInterpolation& interpolation) {
        auto trimmedData = min(1., max(0.,v));

        double color = trimmedData*(nColors-1);

        switch(interpolation) {
            case ColormapInterpolation::nearest: {
                operator[]((int)round(color));
            }
            case ColormapInterpolation::linear: {
                int lidx = (int)floor(color);
                int uidx = (int)ceil(color);
                double a = fractional(color);

                return qRgb((1-a)*colors[3*lidx]+a*colors[3*uidx],
                            (1-a)*colors[3*lidx+1]+a*colors[3*uidx+1],
                            (1-a)*colors[3*lidx+2]+a*colors[3*uidx+2]);
            }
        }
    }

    static const Colormap BrewerBrownToGreen;
    static const Colormap BrewerGreenToPink;
    static const Colormap BrewerGreenToPurple;
    static const Colormap BrewerOrangeToPurple;
    static const Colormap BrewerRedToBlue;
    static const Colormap BrewerBlackToRed;
    static const Colormap BrewerRedYellowBlue;
    static const Colormap BrewerRedYellowGreen;
    static const Colormap BrewerRedYellowGreenBlue;

    static const map<string, const Colormap& > AvailableColormaps;
private:

    static double fractional(double x)
    {
        double intpart;
        auto fractpart = modf (x , &intpart);
        return fractpart;
    }

    vector<int> colors;
    int nColors;
};

#endif // COLORMAP_H
