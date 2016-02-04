#ifndef DENSITYDATA_H
#define DENSITYDATA_H

#import <vector>
#import <QString>

#include "H5Cpp.h"
using namespace H5;

using namespace std;

class OrthogonalTransformation {
public:
    OrthogonalTransformation() {}
    OrthogonalTransformation(vector<double> i_t,
                             vector<double> i_stepSize);
    OrthogonalTransformation(vector<double> i_t,
                             vector<double> i_stepSize,
                             vector<int> i_sectionIndices);
    vector<double> operator()(const vector<int>& ind);
    OrthogonalTransformation getSection(string section, int x);
private:
    vector<double> t;
    vector<double> stepSize;
    vector<int> sectionIndices;
    int dimIn() {return stepSize.size();}
    int dimOut() {return t.size();}
};

class DensitySection {
public:
    //Transformation matrix
    //Where it comes from
    //ind to hkl function
    DensitySection() {}
    DensitySection(vector<double> inp_data,
                   vector<double> inp_size,
                   OrthogonalTransformation inp_tran);
    vector<double> size;
    vector<double> data;
    double at(int x, int y) {return data[x*size[1]+y];}
    vector<double> ind2hkl(const vector<int> & indices);
private:
    vector<int> axisDirs;
    int sectionDir;
    int x;
    OrthogonalTransformation tran;
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

    double at(int x, int y, int z) {return data[(x*size[1]+y)*size[2]+z];}
    vector<double> ind2hkl(const vector<int> & indices);
private:
    void loadFromHDF5();
    H5File dataFile;
    DataSet rebinnedData, noPixRebinned;

    bool isDirect;
    vector<double> lowerLimits;
    vector<double> stepSize;
    vector<vector<double> > metricTensor;
    vector<double> unitCell;
    OrthogonalTransformation tran;
};

#endif // DENSITYDATA_H
