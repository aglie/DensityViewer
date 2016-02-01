#include "densitydata.h"
#include <random>

DensitySection::DensitySection(vector<double> inp_data, vector<double> inp_size):
    size{inp_size},
    data{inp_data}

{}

DensityData::DensityData()
{
    const int sz=100;
    size={sz,sz,sz};
    data=vector<double>(sz*sz*sz,0);

    for(int i=0; i<sz; ++i)
        for(int j=0; j<sz; ++j)
            for(int k=0; k<sz; ++k)
                data[(i*size[0]+j)*size[1]+k] = rand() % 255;
}

DensitySection DensityData::extractSection(QString section, int x) {
    vector<double> rsize,rdata;
    if(section=="hkx")
    {
        rsize=vector<double> {size[0],size[1]};
        rdata=vector<double> (rsize[0]*rsize[1],0);

        for(int i=0; i<size[0]; ++i)
            for(int j=0; j<size[1]; ++j)
                rdata[i*rsize[0]+j] = this->at(i,j,x);
    }
    else if(section=="hxl")
    {
        rsize=vector<double> {size[0],size[2]};
        rdata=vector<double> (rsize[0]*rsize[1],0);

        for(int i=0; i<size[0]; ++i)
            for(int j=0; j<size[1]; ++j)
                rdata[i*rsize[0]+j] = this->at(i,x,j);
    }
    else if(section=="xkl")
    {
        rsize=vector<double> {size[1],size[2]};
        rdata=vector<double> (rsize[0]*rsize[1],0);

        for(int i=0; i<size[0]; ++i)
            for(int j=0; j<size[1]; ++j)
                rdata[i*rsize[0]+j] = this->at(x,i,j);
    }

    return DensitySection(rdata,rsize);


}
