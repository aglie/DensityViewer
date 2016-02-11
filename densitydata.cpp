#include "densitydata.h"
#include <random>
#include <assert.h>

vector<int> xComesFirst(const string& section) {
    vector<int> res;
    if(section=="hkx")
        res={2,0,1};
    else if(section=="hxl")
        res={1,0,2};
    else if(section=="xkl")
        res={0,1,2};

    return res;
}
vector<int> sectionAxes(const string& section) {
    auto t=xComesFirst(section);
    return vector<int>(begin(t)+1,end(t));
}
int crossectedCoordinate(const string& section) {
    return xComesFirst(section)[0];
}



OrthogonalTransformation::OrthogonalTransformation(
        vector<double> i_t,
        vector<double> i_stepSize,
        vector<vector<double>> i_metricTensor) :
    t{i_t},
    stepSize{i_stepSize},
    metricTensor{i_metricTensor}
{
    assert(dimIn()==dimOut());
    sectionIndices=vector<int>(dimIn());
    for(int i=0; i<dimIn(); ++i)
        sectionIndices[i]=i;
}

OrthogonalTransformation::OrthogonalTransformation(
        vector<double> i_t,
        vector<double> i_stepSize,
        vector<int> i_sectionIndices,
        vector<vector<double>> i_metricTensor) :
    t{i_t},
    stepSize{i_stepSize},
    sectionIndices{i_sectionIndices},
    metricTensor{i_metricTensor}
{ }

vector<double> OrthogonalTransformation::operator()(const vector<int> & ind) {
    vector<double> res = t;
    for(int i=0; i<dimIn(); ++i)
        res[sectionIndices[i]]+=ind[i]*stepSize[i];

    return res;
}
double OrthogonalTransformation::transformAxis(int axisN, int index) {
    return t[sectionIndices[axisN]]+index*stepSize[axisN];
}

double OrthogonalTransformation::transformAxisInv(int axisN, double index) {
    return (index-t[sectionIndices[axisN]])/stepSize[axisN];
}

OrthogonalTransformation OrthogonalTransformation::getSection(
        string section,
        int x)
{
    assert(dimIn()==3 && dimOut()==3);
    auto outSectionAxes = sectionAxes(section);
    auto sectionIndex = crossectedCoordinate(section);

    vector<double> outT = t;
    outT[sectionIndex]+=x*stepSize[sectionIndex];

    vector<double> outStepSize =
        {stepSize[outSectionAxes[0]],
         stepSize[outSectionAxes[1]]};

    vector<vector<double>> outMetricTensor(2,vector<double>(2,0));
    for(int i=0; i<2; ++i)
        for(int j=0; j<2; ++j)
            outMetricTensor[i][j]=metricTensor[outSectionAxes[i]][outSectionAxes[j]];

    return OrthogonalTransformation(outT,outStepSize, outSectionAxes,outMetricTensor);
}


DensitySection::DensitySection(
        vector<double> inp_data,
        vector<double> inp_size,
        OrthogonalTransformation inp_tran,
        string section):
    size{inp_size},
    data{inp_data},
    tran{inp_tran},
    axisDirs{sectionAxes(section)},
    sectionDir{crossectedCoordinate(section)}
{ }

vector<double> DensitySection::ind2hkl(const vector<int> & indices) {
    return tran(indices);
}

double DensitySection::lowerLimit(int axisN) {
    auto t=tran({0,0});
    return t[axisDirs[axisN]];
}

double DensitySection::upperLimit(int axisN) {
    vector<int> t(2,0);
    t[axisN]=size[axisN];
    return tran(t)[axisDirs[axisN]];
}

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

template<typename A, hsize_t Nx, hsize_t Ny>
vector<vector<A> > readMatrix(H5File f, const string& datasetName) {
    DataSet dataset = f.openDataSet(datasetName);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();
    assert(rank==2);
    hsize_t count[2] = {Nx, Ny};
    dataspace.getSimpleExtentDims(count);
    assert(count[0]==Nx && count[1]==Ny);

    A out[Nx][Ny];
    dataset.read( out, getH5Type<A>());

    vector<vector<A> > res(Nx);
    for(int i=0; i<Nx; ++i)
        res[i]=vector<A> (begin(out[i]),end(out[i]));

    return res;
}

template<typename A, hsize_t datasetSize>
vector<A> readVector(H5File f, const string& datasetName) {
    DataSet dataset = f.openDataSet(datasetName);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();
    assert(rank==1);
    hsize_t count[1] = {datasetSize};
    dataspace.getSimpleExtentDims(count);
    assert(count[0]==datasetSize);

    A out[datasetSize];
    dataset.read( out, getH5Type<A>());

    return vector<A>(begin(out),end(out));
}

template<typename A>
A readConstant(H5File f, const string& datasetName) {
    A res[1];

    DataSet dataset = f.openDataSet(datasetName);
    DataSpace dataspace = dataset.getSpace();
    assert(dataspace.getSimpleExtentNdims()==0);

    dataset.read( res, getH5Type<A>());

    return res[0];
}

//template<Type A,

void DensityData::loadFromHDF5() {

    //TODO: Here check file format string. currently missing
    auto readProps = FileAccPropList::DEFAULT;
    int mdc_nelmts;
    size_t rdcc_nelmts, rdcc_nbytes;
    double rdcc_w0;
    readProps.getCache(mdc_nelmts, rdcc_nelmts, rdcc_nbytes, rdcc_w0);
    readProps.setCache(mdc_nelmts, 269251, 1024*1024*500, rdcc_w0); //Magic numbers here
    dataFile = H5File( "/Users/arkadiy/ag/josh/Diffuse/Crystal2/xds/reconstruction2.h5",
                       H5F_ACC_RDONLY,
                       FileCreatPropList::DEFAULT,
                       readProps);

    rebinnedData = dataFile.openDataSet("rebinned_data");
    noPixRebinned = dataFile.openDataSet( "number_of_pixels_rebinned" );

    //Read dataset and crystal data
    isDirect = readConstant<bool>(dataFile, "is_direct");

    lowerLimits = readVector<double,3>(dataFile,"maxind");
    for(auto & ll : lowerLimits)
        ll=-ll;
    stepSize = readVector<double, 3>(dataFile, "step_size");
    unitCell = readVector<double, 6>(dataFile, "unit_cell");
    metricTensor = readMatrix<double, 3, 3>(dataFile, "metric_tensor");

    tran = OrthogonalTransformation(lowerLimits,stepSize,metricTensor);

    DSetCreatPropList cparms = rebinnedData.getCreatePlist();
//    hsize_t chunk_dims[3];
//    int rank_chunk = cparms.getChunk( 3, chunk_dims);


    hsize_t datasetDimesions[3];
    rebinnedData.getSpace().getSimpleExtentDims( datasetDimesions, NULL);

    size = vector<double>(begin(datasetDimesions), end(datasetDimesions));
}

DensityData::DensityData()
{
    loadFromHDF5();
}

DensitySection DensityData::extractSection(QString section, int x) {
    if(section==extractSectionMemo.section && x==extractSectionMemo.x)
        return extractSectionMemo.res;


    vector<double> rsize, rdata;

    hsize_t count[3]={1,1,1};
    hsize_t offset[3]={0,0,0};

    auto outSectionAxes = sectionAxes(section.toStdString());
    auto sectionIndex = crossectedCoordinate(section.toStdString());

    offset[sectionIndex]=x;
    for(auto i : outSectionAxes)
        count[i]=size[i];

    rsize=vector<double> {(double)count[outSectionAxes[0]],(double) count[outSectionAxes[1]]};

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


    DensitySection res(data,rsize,tran.getSection(section.toStdString(), x),section.toStdString());
    extractSectionMemo={section,x,res};
    return res;
}



vector<double> DensityData::ind2hkl(const vector<int> & indices) {
    return tran(indices);
}
