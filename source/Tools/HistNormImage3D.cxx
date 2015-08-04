#include "Log.h"

#include <stdio.h>
#include <iostream>
#include "ArgumentParser.h"
#include "ImageUtils.h"
#include "FilterUtils.hpp"
#include <fstream>
#include "itkHistogramMatchingImageFilter.h"
using namespace std;
using namespace itk;



int main(int argc, char ** argv)
{

	//feraiseexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);
    typedef  short PixelType;
    const unsigned int D=3;
    typedef Image<PixelType,D> ImageType;
    typedef  ImageType::IndexType IndexType;
    typedef  ImageType::PointType PointType;
    typedef  ImageType::DirectionType DirectionType;
    typedef  ImageType::SpacingType SpacingType;

    typedef ImageType::Pointer ImagePointerType;
    typedef ImageType::ConstPointer ImageConstPointerType;
 
    ArgumentParser * as=new ArgumentParser(argc,argv);
    string inFile, outFile,refFile="";
    double factor=-1;
    bool noSmoothing=false;
    bool nnResampling=false;
    bool rectifyAlignment=false;
    double spacing=-1;
    as->parameter ("in", inFile, " filename...", true);
    as->parameter ("out", outFile, " filename...", true);
    as->parameter ("ref", refFile, " filename...", true);

    as->parse();
    

    ImagePointerType img = ImageUtils<ImageType>::readImage(inFile);
    ImagePointerType refImage = ImageUtils<ImageType>::readImage(refFile);
    typedef itk::HistogramMatchingImageFilter<ImageType,ImageType> HEFilterType;
    HEFilterType::Pointer IntensityEqualizeFilter = HEFilterType::New();
    IntensityEqualizeFilter->SetReferenceImage( refImage  );
    IntensityEqualizeFilter->SetInput( img );
    IntensityEqualizeFilter->SetNumberOfHistogramLevels( 512);
    IntensityEqualizeFilter->SetNumberOfMatchPoints( 64);
    IntensityEqualizeFilter->ThresholdAtMeanIntensityOff();
    IntensityEqualizeFilter->Update();
    ImagePointerType outImage=IntensityEqualizeFilter->GetOutput();

    ImageUtils<ImageType>::writeImage(outFile,outImage);

	return 1;
}
