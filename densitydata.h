#ifndef DENSITYDATA_H
#define DENSITYDATA_H

#import <vector>
#import <QString>

#include "H5Cpp.h"
using namespace H5;

using namespace std;

class DensitySection {
public:
    //Transformation matrix
    //Where it comes from
    //ind to hkl function
    DensitySection() {}
    DensitySection(vector<double> inp_data, vector<double> inp_size);
    vector<double> size;
    vector<double> data;
    double at(int x, int y) {return data[x*size[1]+y];}
};

class DensityData
{
public:
    DensityData();
    vector<double> data;
    vector<double> size; //TODO: change to size type, not double
    DensitySection extractSection(QString section,int x);
    struct extractSectionMemoStruc {
        QString section;
        int x;
        DensitySection res;
    };
    extractSectionMemoStruc extractSectionMemo;

    bool isDirect;

    double at(int x, int y, int z) {return data[(x*size[1]+y)*size[2]+z];}
private:
    void loadFromHDF5();
    H5File dataFile;
    DataSet rebinnedData, noPixRebinned;
};

#endif // DENSITYDATA_H
