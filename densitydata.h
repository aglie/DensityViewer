#ifndef DENSITYDATA_H
#define DENSITYDATA_H

#include <vector>
#include <QString>

#include "H5Cpp.h"
using namespace H5;

using namespace std;

string axesNames(bool direct);

class UnknownFormat : std::exception {
public:
    UnknownFormat() {}
    ~UnknownFormat() throw() {}
};


class OrthogonalTransformation {
public:
    OrthogonalTransformation() {}
    OrthogonalTransformation(vector<double> i_t,
                             vector<double> i_stepSize,
                             vector<vector<double>> i_metricTensor);
    OrthogonalTransformation(vector<double> i_t,
                             vector<double> i_stepSize,
                             vector<int> i_sectionIndices,
                             vector<vector<double>> i_metricTensor);
    vector<double> operator()(const vector<int>& ind);
    vector<int> hkl2ind(const vector<double>& hkl);
    OrthogonalTransformation getSection(string section, int x);
    double transformAxis(int axisN, int index);
    double transformAxisInv(int axisN, double h);
    vector<double> stepSizes;
    vector<vector<double>> metricTensor;
private:
    vector<double> t;

    vector<int> sectionIndices;
    int dimIn() {return stepSizes.size();}
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
                   OrthogonalTransformation inp_tran,
                   string section,
                   bool isDirect);
    vector<double> size;
    vector<double> data;
    double at(int x, int y) {return data[x*size[1]+y];}
    vector<double> ind2hkl(const vector<int> & indices);
    double lowerLimit(int axisN);
    double upperLimit(int axisN);
    OrthogonalTransformation tran;
    string title();
    int sectionDir;

private:
    bool isDirect;
    vector<int> axisDirs;

    int x;
};

class DensityData
{
public:
    DensityData() {};
    DensityData(string filename);

    vector<int> size; //TODO: change to size type, not double
    DensitySection extractSection(string section, double x);
    struct extractSectionMemoStruc {
        string section;
        double x;
        DensitySection res;
    };
    extractSectionMemoStruc extractSectionMemo;

    //vector<double> data;
    double atInd(int x, int y, int z);
    double atHKL(const vector<double> & hkl);
    vector<double> ind2hkl(const vector<int> & indices);
    double lowerLimit(int i);
    double upperLimit(int i);
    double stepSize(int i);
    bool isDirect;
private:
    H5File dataFile;
    DataSet rebinnedData, noPixRebinned, data;

    vector<double> lowerLimits;
    vector<double> stepSizes;
    vector<vector<double> > metricTensor;
    vector<double> unitCell;
    OrthogonalTransformation tran;
};

#endif // DENSITYDATA_H
