#include "Log.h"

#include <stdio.h>
#include <iostream>
#include "argstream.h"
#include "ImageUtils.h"
#include "FilterUtils.hpp"
#include <fstream>

using namespace std;
using namespace itk;



int main(int argc, char ** argv)
{

	feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);
    typedef  unsigned char PixelType;
    const unsigned int D=2;
    typedef Image<PixelType,D> ImageType;
    typedef  ImageType::IndexType IndexType;
    typedef  ImageType::PointType PointType;
    typedef  ImageType::DirectionType DirectionType;

    typedef ImageType::Pointer ImagePointerType;
    typedef ImageType::ConstPointer ImageConstPointerType;
 
    argstream * as=new argstream(argc,argv);
    string inFile, outFile,refFile="";
    double thick=1;
    
    (*as) >> parameter ("in", inFile, " filename...", true);
    (*as) >> parameter ("out", outFile, " filename...", true);
    (*as) >> parameter ("s", thick, " line strength", false);
  

    (*as) >> help();
    as->defaultErrorHandling();

    ImagePointerType img = ImageUtils<ImageType>::readImage(inFile);
    PixelType maxVal=FilterUtils<ImageType>::getMax(img);
   
    ImagePointerType dilated=FilterUtils<ImageType>::dilation(img,thick,maxVal);
    

    ImagePointerType outImage=FilterUtils<ImageType>::substract(dilated,img);
   
    
    ImageUtils<ImageType>::writeImage(outFile,outImage);

	return 1;
}
