#include "densitydata.h"
#include <random>



DensitySection::DensitySection(vector<double> inp_data, vector<double> inp_size):
    size{inp_size},
    data{inp_data}

{}

void DensityData::loadFromHDF5() {

//    <HDF5 dataset "is_direct": shape (), type "|b1">
//    <HDF5 dataset "maxind": shape (3,), type "<f8">
//    <HDF5 dataset "metric_tensor": shape (3, 3), type "<f8">
//    <HDF5 dataset "number_of_pixels": shape (3,), type "<i8">
//    <HDF5 dataset "number_of_pixels_rebinned": shape (801, 801, 561), type "<i8">
//    <HDF5 dataset "rebinned_data": shape (801, 801, 561), type "<f8">
//    <HDF5 dataset "space_group_nr": shape (1,), type "<f8">
//    <HDF5 dataset "step_size": shape (3,), type "<f8">
//    <HDF5 dataset "unit_cell": shape (6,), type "<f8">

// Here we need to load the h5 file
// and (later) load all the information neede to recalculate coordinate system transformation

    //here check file format string. currently missing

    dataFile = H5File( "/Users/arkadiy/ag/josh/Diffuse/Crystal2/xds/reconstruction2.h5", H5F_ACC_RDONLY );
    DataSet noPixRebinned = dataFile.openDataSet( "number_of_pixels_rebinned" );
    DataSet rebinnedData = dataFile.openDataSet("rebinned_data");

    //possibly check consistensy of datatypes
    hsize_t datasetDimesions[3];
    rebinnedData.getSpace().getSimpleExtentDims( datasetDimesions, NULL);

    size = vector<double>(begin(datasetDimesions), end(datasetDimesions));

    int * noPixBuffer = (int*) malloc(sizeof(int)*size[0]*size[1]);
    //memset(noPixBuffer,0,sizeof(int)*size[0]*size[1]);
    double * rebinnedDataBuffer = (double*) malloc(sizeof(double)*size[0]*size[1]);
    //memset(rebinnedDataBuffer,0,sizeof(double)*size[0]*size[1]);

    DataSpace dataspace = noPixRebinned.getSpace();

    hsize_t      offset[3]={0,0,0};   // hyperslab offset in the file
    hsize_t      count[3]={(hsize_t)size[0],(hsize_t) size[1],1};    // size of the hyperslab in the file
    dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );

    hsize_t     sectionSize[2]={(hsize_t)size[0],(hsize_t) size[1]};
    DataSpace memspace( 2, sectionSize );

    hsize_t      offset_out[2]={0,0};
    memspace.selectHyperslab( H5S_SELECT_SET, sectionSize, offset_out );

    noPixRebinned.read( noPixBuffer, PredType::NATIVE_INT, memspace, dataspace );
    rebinnedData.read( rebinnedDataBuffer, PredType::NATIVE_DOUBLE, memspace, dataspace );

    data = vector<double>(size[0]*size[1]);
    for(int i=0; i<size[0]*size[1]; ++i)
        data[i]=rebinnedDataBuffer[i]/noPixBuffer[i];

}

DensityData::DensityData()
{
//    const int sz=100;
//    size={sz,sz,sz};
//    data=vector<double>(sz*sz*sz,0);

//    for(int i=0; i<sz; ++i)
//        for(int j=0; j<sz; ++j)
//            for(int k=0; k<sz; ++k)
//                data[(i*size[0]+j)*size[1]+k] = rand() % 255;
    loadFromHDF5();
}

DensitySection DensityData::extractSection(QString section, int x) {
//    vector<double> rsize,rdata;
//    if(section=="hkx")
//    {
//        rsize=vector<double> {size[0],size[1]};
//        rdata=vector<double> (rsize[0]*rsize[1],0);

//        for(int i=0; i<size[0]; ++i)
//            for(int j=0; j<size[1]; ++j)
//                rdata[i*rsize[0]+j] = this->at(i,j,x);
//    }
//    else if(section=="hxl")
//    {
//        rsize=vector<double> {size[0],size[2]};
//        rdata=vector<double> (rsize[0]*rsize[1],0);

//        for(int i=0; i<size[0]; ++i)
//            for(int j=0; j<size[1]; ++j)
//                rdata[i*rsize[0]+j] = this->at(i,x,j);
//    }
//    else if(section=="xkl")
//    {
//        rsize=vector<double> {size[1],size[2]};
//        rdata=vector<double> (rsize[0]*rsize[1],0);

//        for(int i=0; i<size[0]; ++i)
//            for(int j=0; j<size[1]; ++j)
//                rdata[i*rsize[0]+j] = this->at(x,i,j);
//    }

//    return DensitySection(rdata,rsize);
    vector<double> rsize = {size[0],size[1]};
    return DensitySection(data,rsize);


}
