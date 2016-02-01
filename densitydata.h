#ifndef DENSITYDATA_H
#define DENSITYDATA_H

#import <vector>
#import <QString>

using namespace std;

class DensitySection {
public:
    //Transformation matrix
    //Where it comes from
    //ind to hkl function
    DensitySection(vector<double> inp_data, vector<double> inp_size);
    vector<double> size;
    vector<double> data;
    double at(int x, int y) {return data[x*size[0]+y];}
};

class DensityData
{
public:
    DensityData();
    vector<double> data;
    vector<double> size;
    DensitySection extractSection(QString section,int x);
    double at(int x,int y,int z) {return data[(x*size[0]+y)*size[1]+z];}
};

#endif // DENSITYDATA_H
