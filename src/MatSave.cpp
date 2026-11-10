#include <iostream>
#include <string.h> /* For strcmp() */
#include <stdlib.h> /* For EXIT_FAILURE, EXIT_SUCCESS */
#include "mat.h"
#include "MatSave.h"

// #include <filesystem>
using namespace std;
// namespace fs = std::filesystem;
template <typename eT>
int SaveAsMat(const char *file,const char *name,const eT* dataptr,int len)
{
    MATFile *pmat;
    mxArray *pa1;
    // const char *file = "../filter_result.mat";
    // fs::path p(file);
    int status;

    if (dataptr == nullptr || file==nullptr || name==nullptr || len == 0)
    {
        return(EXIT_FAILURE);
    }
    

    pmat = matOpen(file, "w");
    if (pmat == NULL) {
        cerr <<"Error creating file %s\n" << file << endl;
        cerr <<"(Do you have write permission in this directory?)"<< endl;
        return(EXIT_FAILURE);
    }

    pa1 = mxCreateDoubleMatrix(len,1,mxREAL);
    if (pa1 == NULL) {
        cerr <<__FILE__<<" : Out of memory on line "<< __LINE__<< endl; 
        cerr <<"Unable to create mxArray."<< endl;
        return(EXIT_FAILURE);
    }
    memcpy((void *)(mxGetPr(pa1)), reinterpret_cast<void*>(dataptr), len*sizeof(eT));
    // status = matPutVariable(pmat, p.stem().string().c_str(), pa1);
    status = matPutVariable(pmat, name, pa1);
    if (status != 0) {
        cerr <<__FILE__<<" :  Error using matPutVariable on line "<<  __LINE__<< endl;
        return(EXIT_FAILURE);
    }

    /* clean up */
    mxDestroyArray(pa1);

    if (matClose(pmat) != 0) {
        cerr <<"Error closing file %s"<<file<< endl;
        return(EXIT_FAILURE);
    }
    return(EXIT_SUCCESS);
}
template <typename eT>
int ReadFromMat(const char *file,const char *name,eT* dataptr,int len)
{

    // std::cout <<"进了"<<sizeof(double)<< std::endl;

    MATFile *pmat;
    mxArray *pa1;

    if (dataptr == nullptr || file == nullptr || name == nullptr || len == 0)
    {
        std::cerr<<"出错！！！"<< std::endl;
        return(EXIT_FAILURE);
    }

    pmat = matOpen(file, "r");
    if (pmat == NULL) {
        cerr <<"Error reopening file %s"<< file << endl;
        return(EXIT_FAILURE);
    }

    /*
    * Read in each array we just wrote
    */
    pa1 = matGetVariable(pmat, name);
    if (pa1 == NULL) {
        cerr <<"Error reading existing matrix LocalDouble"<< endl;
        return(EXIT_FAILURE);
    }
    if (mxGetNumberOfDimensions(pa1) != 2) {
        cerr <<"Error saving matrix: result does not have two dimensions"<< endl;
        return(EXIT_FAILURE);
    }
    memcpy(reinterpret_cast<void*>(dataptr),mxGetData(pa1),len*sizeof(eT));

    /* clean up */
    mxDestroyArray(pa1);

    if (matClose(pmat) != 0) {
        cerr <<"Error closing file %s"<<file<< endl;
        return(EXIT_FAILURE);
    }
    return(EXIT_SUCCESS);
}