#include "densitydata.h"
#include <random>



DensitySection::DensitySection(vector<double> inp_data, vector<double> inp_size):
    size{inp_size},
    data{inp_data}

{}

template<typename T>
DataType getH5Type() {}

template<>
DataType getH5Type<int> () {
    return PredType::NATIVE_INT;
}
template<>
DataType getH5Type<bool> () {
    return PredType::NATIVE_HBOOL;
}
template<>
DataType getH5Type<double> () {
    return PredType::NATIVE_DOUBLE;
}



template<typename A, hsize_t datasetSize>
void readDatasetRaw(H5File f, const string& datasetName, A* out) {
    DataSet dataset = f.openDataSet(datasetName);

    hsize_t count[1] = {datasetSize};
    hsize_t offset[1] = {0};
    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();
    dataspace.selectAll();

    DataSpace memspace( 1, count );
    memspace.selectHyperslab( H5S_SELECT_SET, count, offset );

    dataset.read( out, getH5Type<A>(), memspace, dataspace );
}

template<typename A>
A readConstant(H5File f, const string& datasetName) {
    A res[1];
    readDatasetRaw<A,1>(f, datasetName, res);
    return res[0];
}

//template<Type A,

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
    auto readProps = FileAccPropList::DEFAULT;
    int mdc_nelmts;
    size_t rdcc_nelmts, rdcc_nbytes;
    double rdcc_w0;
    readProps.getCache(mdc_nelmts, rdcc_nelmts, rdcc_nbytes, rdcc_w0);
    readProps.setCache(mdc_nelmts, 269251, 1024*1024*500, rdcc_w0); //100mb 100mb is a magic number, should be checked on explicit tests. also all the memory evition strategies should be visited, atm it looks like caching algorithm itself takes a lot of time
    dataFile = H5File( "/Users/arkadiy/ag/josh/Diffuse/Crystal2/xds/reconstruction.h5", H5F_ACC_RDONLY, FileCreatPropList::DEFAULT, readProps );

    rebinnedData = dataFile.openDataSet("rebinned_data");
    noPixRebinned = dataFile.openDataSet( "number_of_pixels_rebinned" );

    //Read dataset and crystal data
    isDirect = readConstant<bool>(dataFile, "is_direct");


    DSetCreatPropList cparms = rebinnedData.getCreatePlist();
    hsize_t chunk_dims[3];
    int rank_chunk = cparms.getChunk( 3, chunk_dims);


    hsize_t datasetDimesions[3];
    rebinnedData.getSpace().getSimpleExtentDims( datasetDimesions, NULL);

    size = vector<double>(begin(datasetDimesions), end(datasetDimesions));
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
    if(section==extractSectionMemo.section && x==extractSectionMemo.x)
        return extractSectionMemo.res;


    vector<double> rsize, rdata;

    hsize_t count[3]={1,1,1};
    hsize_t offset[3]={0,0,0};
    if(section=="hkx")
    {
        rsize=vector<double> {size[0],size[1]};
        count[0]=size[0];
        count[1]=size[1];
        offset[2]=x;
    }
    else if(section=="hxl")
    {
        rsize=vector<double> {size[0],size[2]};
        count[0]=size[0];
        count[2]=size[2];
        offset[1]=x;
    }
    else if(section=="xkl")
    {
        rsize=vector<double> {size[1],size[2]};
        count[1]=size[1];
        count[2]=size[2];
        offset[0]=x;
    }

//    DataSet noPixRebinned = dataFile.openDataSet( "number_of_pixels_rebinned" );
//    DataSet rebinnedData = dataFile.openDataSet("rebinned_data");

    int * noPixBuffer = (int*) malloc(sizeof(int)*rsize[0]*rsize[1]);
    double * rebinnedDataBuffer = (double*) malloc(sizeof(double)*rsize[0]*rsize[1]);

    DataSpace dataspace = noPixRebinned.getSpace();
    dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );

    hsize_t     sectionSize[2]={(hsize_t)rsize[0],(hsize_t) rsize[1]};
    DataSpace memspace( 2, sectionSize );

    hsize_t      offset_out[2]={0,0};
    memspace.selectHyperslab( H5S_SELECT_SET, sectionSize, offset_out );

    noPixRebinned.read( noPixBuffer, PredType::NATIVE_INT, memspace, dataspace );
    rebinnedData.read( rebinnedDataBuffer, PredType::NATIVE_DOUBLE, memspace, dataspace );

    data = vector<double>(rsize[0]*rsize[1]);
    for(int i=0; i<rsize[0]*rsize[1]; ++i)
        data[i]=rebinnedDataBuffer[i]/noPixBuffer[i];

    free(noPixBuffer);
    free(rebinnedDataBuffer);


    DensitySection res(data,rsize);
    extractSectionMemo={section,x,res};
    return res;
}
