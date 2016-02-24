#include "densitydata.h"
#include <random>
#include <assert.h>
#include <sstream>
#include <cmath>
#include <QMatrix4x4>
#include <iomanip>

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
        vector<double> i_stepSizes,
        vector<vector<double>> i_metricTensor) :
    t{i_t},
    stepSizes{i_stepSizes},
    metricTensor{i_metricTensor}
{
    assert(dimIn()==dimOut());
    sectionIndices=vector<int>(dimIn());
    for(int i=0; i<dimIn(); ++i)
        sectionIndices[i]=i;
}

OrthogonalTransformation::OrthogonalTransformation(
        vector<double> i_t,
        vector<double> i_stepSizes,
        vector<int> i_sectionIndices,
        vector<vector<double>> i_metricTensor) :
    t{i_t},
    stepSizes{i_stepSizes},
    sectionIndices{i_sectionIndices},
    metricTensor{i_metricTensor}
{ }

vector<double> OrthogonalTransformation::operator()(const vector<int> & ind) {
    vector<double> res = t;
    for(int i=0; i<dimIn(); ++i)
        res[sectionIndices[i]]+=ind[i]*stepSizes[i];

    return res;
}
double OrthogonalTransformation::transformAxis(int axisN, int index) {
    return t[sectionIndices[axisN]]+index*stepSizes[axisN];
}

double OrthogonalTransformation::transformAxisInv(int axisN, double index) {
    return (index-t[sectionIndices[axisN]])/stepSizes[axisN];
}

OrthogonalTransformation OrthogonalTransformation::getSection(
        string section,
        int x)
{
    assert(dimIn()==3 && dimOut()==3);
    auto outSectionAxes = sectionAxes(section);
    auto sectionIndex = crossectedCoordinate(section);

    vector<double> outT = t;
    outT[sectionIndex]+=x*stepSizes[sectionIndex];

    vector<double> outStepSize =
        {stepSizes[outSectionAxes[0]],
         stepSizes[outSectionAxes[1]]};

    vector<vector<double>> outMetricTensor(2,vector<double>(2,0));
    for(int i=0; i<2; ++i)
        for(int j=0; j<2; ++j)
            outMetricTensor[i][j]=metricTensor[outSectionAxes[i]][outSectionAxes[j]];

    return OrthogonalTransformation(outT,outStepSize, outSectionAxes,outMetricTensor);
}


DensitySection::DensitySection(vector<double> inp_data,
        vector<double> inp_size,
        OrthogonalTransformation inp_tran,
        string section,
        bool inpIsDirect):
    size{inp_size},
    data{inp_data},
    tran{inp_tran},
    axisDirs{sectionAxes(section)},
    sectionDir{crossectedCoordinate(section)},
    isDirect{inpIsDirect}
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

vector<string> axesNames(bool direct) {
    if(direct)
        return {"u","v","w"};
    else
        return {"h","k","l"};
}

string DensitySection::title() {
    vector<string> res = axesNames(isDirect);

    ostringstream t;
    t << std::setprecision(4);
    t<<tran({0,0})[sectionDir];
    res[sectionDir] = t.str();
    return res[0]+res[1]+res[2];
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
    //Due to (probably) bug, dataset.read overwrites two values for a scalar
    //don't have time to trace it
    A res[2];

    DataSet dataset = f.openDataSet(datasetName);
    DataSpace dataspace = dataset.getSpace();
    assert(dataspace.getSimpleExtentNdims()==0);

    dataset.read(res, getH5Type<A>(),DataSpace(H5S_SCALAR),DataSpace(H5S_SCALAR));

    return res[0];
}

double deg2rad(double a) {
    return a/180*M_PI;
}

vector<vector<double>> metricTensorFromUnitCell(const vector<double>& unitCell, bool invert) {
    double a = unitCell[0];
    double b = unitCell[1];
    double c = unitCell[2];
    double alpha = deg2rad(unitCell[3]);
    double beta  = deg2rad(unitCell[4]);
    double gamma = deg2rad(unitCell[5]);
    QMatrix4x4 t (a*a, a*b*cos(gamma), a*c*cos(beta),0,
                    b*a*cos(gamma),b*b, b*c*cos(alpha),0,
                    c*a*cos(beta),c*b*cos(alpha),c*c,0,
                   0,0,0,1);
    if(invert)
        t=t.inverted();

    auto tt=t.data();
    return {{tt[0],tt[1],tt[2]},
            {tt[4],tt[5],tt[6]},
            {tt[8],tt[9],tt[10]}};
}

void checkFileFormat(H5File& file) {
    std::string format;
    try {
    StrType datatype(0, H5T_VARIABLE);
    DataSpace dataspace(H5S_SCALAR);
    DataSet datset = file.openDataSet("format");

    datset.read(format, datatype, dataspace);
    } catch(H5::FileIException) {
        throw(UnknownFormat());
    }

    if(format!="Yell 1.0")
        throw(UnknownFormat());
}

//template<Type A,
DensityData::DensityData(string filename) {

    //TODO: Here check file format string. currently missing
    auto readProps = FileAccPropList::DEFAULT;
    int mdc_nelmts;
    size_t rdcc_nelmts, rdcc_nbytes;
    double rdcc_w0;
    readProps.getCache(mdc_nelmts, rdcc_nelmts, rdcc_nbytes, rdcc_w0);
    readProps.setCache(mdc_nelmts, 269251, 1024*1024*500, rdcc_w0); //Magic numbers to tune performance
    dataFile = H5File( filename,
                       H5F_ACC_RDONLY,
                       FileCreatPropList::DEFAULT,
                       readProps);

    checkFileFormat(dataFile);

    data = dataFile.openDataSet("data");
    //rebinnedData = dataFile.openDataSet("rebinned_data");
    //noPixRebinned = dataFile.openDataSet( "number_of_pixels_rebinned" );

    //Read dataset and crystal data
    isDirect = readConstant<bool>(dataFile, "is_direct");

    lowerLimits = readVector<double,3>(dataFile,"lower_limits");
    stepSizes = readVector<double, 3>(dataFile, "step_size");
    unitCell = readVector<double, 6>(dataFile, "unit_cell");
    metricTensor = metricTensorFromUnitCell(unitCell, !isDirect);

    tran = OrthogonalTransformation(lowerLimits, stepSizes, metricTensor);

    DSetCreatPropList cparms = data.getCreatePlist();
//    hsize_t chunk_dims[3];
//    int rank_chunk = cparms.getChunk( 3, chunk_dims);


    hsize_t datasetDimesions[3];
    data.getSpace().getSimpleExtentDims( datasetDimesions, NULL);

    size = vector<int>(begin(datasetDimesions), end(datasetDimesions));
}


DensitySection DensityData::extractSection(string section, double x) {
    if(section==extractSectionMemo.section && x==extractSectionMemo.x)
        return extractSectionMemo.res;

    vector<double> rsize, rdata;

    hsize_t count[3]={1,1,1};
    hsize_t offset[3]={0,0,0};

    auto outSectionAxes = sectionAxes(section);
    auto sectionIndex = crossectedCoordinate(section);

    int xi = round(tran.transformAxisInv(sectionIndex,x));
    xi=min(xi,size[sectionIndex]-1);
    xi=max(xi,0);


    offset[sectionIndex]=xi;
    for(auto i : outSectionAxes)
        count[i]=size[i];

    rsize=vector<double> {(double)count[outSectionAxes[0]],(double) count[outSectionAxes[1]]};

    //int * noPixBuffer = (int*) malloc(sizeof(int)*rsize[0]*rsize[1]);
    double * dataBuffer = (double*) malloc(sizeof(double)*rsize[0]*rsize[1]);

    DataSpace dataspace = data.getSpace();
    dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );

    hsize_t     sectionSize[2]={(hsize_t)rsize[0],(hsize_t) rsize[1]};
    DataSpace memspace( 2, sectionSize );

    hsize_t      offset_out[2]={0,0};
    memspace.selectHyperslab( H5S_SELECT_SET, sectionSize, offset_out );

    data.read(dataBuffer, PredType::NATIVE_DOUBLE, memspace, dataspace);
//    noPixRebinned.read( noPixBuffer, PredType::NATIVE_INT, memspace, dataspace );
//    rebinnedData.read( rebinnedDataBuffer, PredType::NATIVE_DOUBLE, memspace, dataspace );

    vector<double> sectionData(rsize[0]*rsize[1]);
    for(int i=0; i<rsize[0]*rsize[1]; ++i)
        sectionData[i]=dataBuffer[i];

    free(dataBuffer);
    //free(rebinnedDataBuffer);

    DensitySection res(sectionData, rsize, tran.getSection(section, xi), section, isDirect);
    extractSectionMemo={section, x, res};
    return res;
}



vector<double> DensityData::ind2hkl(const vector<int> & indices) {
    return tran(indices);
}

double DensityData::lowerLimit(int i) {
    return lowerLimits[i];
}

double DensityData::upperLimit(int i) {
    return lowerLimits[i]+stepSizes[i]*size[i];
}

double DensityData::stepSize(int i) {
    return stepSizes[i];
}
