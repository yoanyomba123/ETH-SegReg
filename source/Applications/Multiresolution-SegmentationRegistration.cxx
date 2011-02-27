
#include <stdio.h>
#include <iostream>

#include "argstream.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "ImageUtils.h"
#include "itkImage.h"
#include "itkVectorImage.h"
#include <itkNumericTraitsVectorPixel.h>


#include <fenv.h>
#include <sstream>
#include "itkHistogramMatchingImageFilter.h"
#include "Graph.h"
#include "BaseLabel.h"
#include "SRSPotential.h"
#include "MRF-FAST-PD.h"
#include "MRF-TRW-S.h"
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkVectorResampleImageFilter.h>
#include "itkResampleImageFilter.h"
#include "itkAffineTransform.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkVectorLinearInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkImageIterator.h"
#include "itkImageConstIteratorWithIndex.h"
#include "itkImageIteratorWithIndex.h"
#include "itkImageConstIterator.h"

#include <itkImageAdaptor.h>
#include <itkAddPixelAccessor.h>

#include "itkBSplineInterpolateImageFunction.h"

using namespace std;
using namespace itk;

#define _MANY_LABELS_
#define DOUBLEPAIRWISE
typedef unsigned short PixelType;
const unsigned int D=2;
typedef Image<PixelType,D> ImageType;
typedef ImageType::IndexType IndexType;
typedef itk::Vector<float,D+1> BaseLabelType;
typedef SparseLabelMapper<ImageType,BaseLabelType> LabelMapperType;
template<> int LabelMapperType::nLabels=-1;
template<> int LabelMapperType::nDisplacements=-1;
template<> int LabelMapperType::nSegmentations=-1;
template<> int LabelMapperType::nDisplacementSamples=-1;
template<> int LabelMapperType::k=-1;



int main(int argc, char ** argv)
{
	feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);


	argstream as(argc, argv);
	string targetFilename,movingFilename,fixedSegmentationFilename, movingSegmentationFilename, outputFilename,deformableFilename,defFilename="", segmentationOutputFilename;
	double pairwiseRegistrationWeight=1;
	double pairwiseSegmentationWeight=1;
	int displacementSampling=-1;
	double unaryWeight=1;
	int maxDisplacement=10;
	double simWeight=1;
	double rfWeight=1;
	double segWeight=1;

	as >> parameter ("t", targetFilename, "target image (file name)", true);
	as >> parameter ("m", movingFilename, "moving image (file name)", true);
	as >> parameter ("s", movingSegmentationFilename, "moving segmentation image (file name)", true);
	as >> parameter ("g", fixedSegmentationFilename, "fixed segmentation image (file name)", true);

	as >> parameter ("o", outputFilename, "output image (file name)", true);
	as >> parameter ("O", segmentationOutputFilename, "output segmentation image (file name)", true);
	as >> parameter ("f", defFilename,"deformation field filename", false);
	as >> parameter ("rp", pairwiseRegistrationWeight,"weight for pairwise registration potentials", false);
	as >> parameter ("sp", pairwiseSegmentationWeight,"weight for pairwise segmentation potentials", false);

	as >> parameter ("u", unaryWeight,"weight for unary potentials", false);
	as >> parameter ("max", maxDisplacement,"maximum displacement in pixels per axis", false);
	as >> parameter ("wi", simWeight,"weight for intensity similarity", false);
	as >> parameter ("wr", rfWeight,"weight for segmentation posterior", false);
	as >> parameter ("ws", segWeight,"weight for segmentation similarity", false);

	as >> help();
	as.defaultErrorHandling();
	int nSegmentations=2;
	if (segWeight==0 && pairwiseSegmentationWeight==0 && rfWeight==0 ){
		nSegmentations=1;
	}
	if (displacementSampling==-1) displacementSampling=maxDisplacement;
	bool verbose=false;
	//typedefs

	typedef ImageType::Pointer ImagePointerType;
	//	typedef Image<LabelType> LabelImageType;
	//read input images
	ImageType::Pointer targetImage =
			ImageUtils<ImageType>::readImage(targetFilename);
	ImageType::Pointer movingImage =
			ImageUtils<ImageType>::readImage(movingFilename);
	ImageType::Pointer movingSegmentationImage =
			ImageUtils<ImageType>::readImage(movingSegmentationFilename);
	ImageType::Pointer fixedSegmentationImage =
			ImageUtils<ImageType>::readImage(fixedSegmentationFilename);



	typedef itk::HistogramMatchingImageFilter<
			ImageType,
			ImageType >   MatchingFilterType;
	MatchingFilterType::Pointer matcher = MatchingFilterType::New();
	matcher->SetInput( movingImage );
	typedef itk::ImageRegionIterator< ImageType>       IteratorType;

#if 0
	typedef itk::ImageRegionConstIterator< ImageType > ConstIteratorType;
	typedef itk::ImageRegionIterator< ImageType>       IteratorType;
	ImageType::RegionType inputRegion;
	ImageType::RegionType::IndexType inputStart;
	ImageType::RegionType::SizeType  size;
	//	110x190+120+120
	inputStart[0] = int(0.22869*targetImage->GetLargestPossibleRegion().GetSize()[0]);//::atoi( argv[3] );
	inputStart[1] = int(0.508021*targetImage->GetLargestPossibleRegion().GetSize()[1]);
	size[0]  = int(0.25*targetImage->GetLargestPossibleRegion().GetSize()[0]);
	size[1]  = int(0.320856*targetImage->GetLargestPossibleRegion().GetSize()[1]);
	inputRegion.SetSize( size );
	inputRegion.SetIndex( inputStart );
	ImageType::RegionType outputRegion;
	ImageType::RegionType::IndexType outputStart;
	outputStart[0] = 0;
	outputStart[1] = 0;
	outputRegion.SetSize( size );
	outputRegion.SetIndex( outputStart );
	ImageType::Pointer outputImage = ImageType::New();
	outputImage->SetRegions( outputRegion );
	const ImageType::SpacingType& spacing = targetImage->GetSpacing();
	const ImageType::PointType& inputOrigin = targetImage->GetOrigin();
	double   outputOrigin[ D ];

	for(unsigned int i=0; i< D; i++)
	{
		outputOrigin[i] = inputOrigin[i] + spacing[i] * inputStart[i];
	}


	outputImage->SetSpacing( spacing );
	outputImage->SetOrigin(  outputOrigin );
	outputImage->Allocate();
	ConstIteratorType inputIt(   targetImage, inputRegion  );
	IteratorType      outputIt(  outputImage,         outputRegion );

	inputIt.GoToBegin();
	outputIt.GoToBegin();

	while( !inputIt.IsAtEnd() )
	{
		outputIt.Set(  inputIt.Get()  );
		++inputIt;
		++outputIt;
	}

	matcher->SetReferenceImage( outputImage );
#else


	matcher->SetReferenceImage( targetImage );
#endif

	matcher->SetNumberOfHistogramLevels( 30 );
	matcher->SetNumberOfMatchPoints( 7 );
	//	matcher->ThresholdAtMeanIntensityOn();
	matcher->Update();
	movingImage=matcher->GetOutput();

	//	typedef RegistrationLabel<ImageType> BaseLabelType;

	LabelMapperType * labelmapper=new LabelMapperType(nSegmentations,maxDisplacement);

	typedef NearestNeighborInterpolateImageFunction<ImageType> SegmentationInterpolatorType;
	typedef SegmentationInterpolatorType::Pointer SegmentationInterpolatorPointerType;


	typedef LinearInterpolateImageFunction<ImageType> ImageInterpolatorType;
	typedef ImageInterpolatorType::Pointer ImageInterpolatorPointerType;
	typedef SegmentationRegistrationUnaryPotential< LabelMapperType, ImageType, SegmentationInterpolatorType,ImageInterpolatorType > BaseUnaryPotentialType;
	typedef BaseUnaryPotentialType::Pointer BaseUnaryPotentialPointerType;
	typedef GraphModel<BaseUnaryPotentialType,LabelMapperType,ImageType> GraphModelType;
	ImagePointerType deformedImage,deformedSegmentationImage;
	deformedImage=ImageType::New();
	deformedImage->SetRegions(targetImage->GetLargestPossibleRegion());
	deformedImage->SetOrigin(targetImage->GetOrigin());
	deformedImage->SetSpacing(targetImage->GetSpacing());
	deformedImage->SetDirection(targetImage->GetDirection());
	deformedImage->Allocate();
	deformedSegmentationImage=ImageType::New();
	deformedSegmentationImage->SetRegions(targetImage->GetLargestPossibleRegion());
	deformedSegmentationImage->SetOrigin(targetImage->GetOrigin());
	deformedSegmentationImage->SetSpacing(targetImage->GetSpacing());
	deformedSegmentationImage->SetDirection(targetImage->GetDirection());
	deformedSegmentationImage->Allocate();
	typedef NewFastPDMRFSolver<GraphModelType> MRFSolverType;
	typedef MRFSolverType::LabelImageType LabelImageType;
	typedef itk::ImageRegionIterator< LabelImageType>       LabelIteratorType;
	typedef MRFSolverType::LabelImagePointerType LabelImagePointerType;
	typedef VectorLinearInterpolateImageFunction<LabelImageType, double> LabelInterpolatorType;
	typedef LabelInterpolatorType::Pointer LabelInterpolatorPointerType;
	typedef itk::VectorResampleImageFilter< LabelImageType , LabelImageType>	LabelResampleFilterType;
	LabelImagePointerType fullDeformation,previousFullDeformation;
	previousFullDeformation=LabelImageType::New();
	previousFullDeformation->SetRegions(targetImage->GetLargestPossibleRegion());
	previousFullDeformation->SetOrigin(targetImage->GetOrigin());
	previousFullDeformation->SetSpacing(targetImage->GetSpacing());
	previousFullDeformation->SetDirection(targetImage->GetDirection());
	previousFullDeformation->Allocate();
	BaseUnaryPotentialPointerType unaryPot=BaseUnaryPotentialType::New();
	unaryPot->SetFixedImage(targetImage);
	ImageInterpolatorPointerType movingInterpolator=ImageInterpolatorType::New();
	SegmentationInterpolatorPointerType segmentationInterpolator=SegmentationInterpolatorType::New();
	segmentationInterpolator->SetInputImage(movingSegmentationImage);
	unaryPot->SetMovingImage(movingImage);
	unaryPot->SetMovingInterpolator(movingInterpolator);
	unaryPot->SetSegmentationInterpolator(segmentationInterpolator);
	unaryPot->SetWeights(simWeight,rfWeight,segWeight);
	unaryPot->SetMovingSegmentation(movingSegmentationImage);
	ImagePointerType classified;
	classified=unaryPot->trainClassifiers();
	ImageUtils<ImageType>::writeImage("classified.png",classified);

	typedef ImageType::SpacingType SpacingType;
	int nLevels=5;
	nLevels=maxDisplacement>0?nLevels:1;
	//	int levels[]={4,16,40,100,200};
	int levels[]={2,4,8,20,40,100};
	int nIterPerLevel=5;
	for (int l=0;l<nLevels;++l){
		int level=levels[l];
		SpacingType spacing;
		double labelScalingFactor=1;

		for (int d=0;d<ImageType::ImageDimension;++d){
			spacing[d]=targetImage->GetLargestPossibleRegion().GetSize()[d]/level;

		}
		//at 4th level, we switch to full image grid but allow only 1 displacement in each direction
		if (l==nLevels-1){
			LabelMapperType * labelmapper2=new LabelMapperType(nSegmentations,maxDisplacement>0?1:0);
			spacing.Fill(1.0);
			pairwiseRegistrationWeight*=25;
			pairwiseSegmentationWeight*=25;
			labelScalingFactor=0.01;
			nIterPerLevel=maxDisplacement>0?2:1;
		}
		std::cout<<"spacing at level "<<level<<" :"<<spacing<<std::endl;

		for (int i=0;i<nIterPerLevel;++i){
			std::cout<<std::endl<<std::endl<<"Multiresolution optimization at level "<<l<<" in iteration "<<i<<std::endl<<std::endl;
			movingInterpolator->SetInputImage(movingImage);

			GraphModelType graph(targetImage,unaryPot,spacing,labelScalingFactor, pairwiseSegmentationWeight, pairwiseRegistrationWeight );
			graph.setGradientImage(fixedSegmentationImage);
			//			for (int f=0;f<graph.nNodes();++f){
			//				std::cout<<f<<" "<<graph.getGridPositionAtIndex(f)<<" "<<graph.getImagePositionAtIndex(f)<<std::endl;
			//			}
			unaryPot->SetDisplacementFactor(graph.getDisplacementFactor());
			unaryPot->SetBaseLabelMap(previousFullDeformation);
			graph.setLabelImage(previousFullDeformation);
			std::cout<<"Current displacementFactor :"<<graph.getDisplacementFactor()<<std::endl;
			std::cout<<"Current grid size :"<<graph.getGridSize()<<std::endl;
			std::cout<<"Current grid spacing :"<<graph.getSpacing()<<std::endl;
			//	ok what now: create graph! solve graph! save result!Z
			typedef TRWS_MRFSolver<GraphModelType> MRFSolverType;
//						typedef NewFastPDMRFSolver<GraphModelType> MRFSolverType;
			MRFSolverType mrfSolver(&graph,1,1, false);
			mrfSolver.optimize();

			//Apply/interpolate Transformation

			//Get label image (deformation)
			LabelImagePointerType deformation=mrfSolver.getLabelImage();
			//initialise interpolator
			//deformation

			LabelInterpolatorPointerType labelInterpolator=LabelInterpolatorType::New();
			labelInterpolator->SetInputImage(deformation);
			//initialise resampler

			LabelResampleFilterType::Pointer resampler = LabelResampleFilterType::New();
			//resample deformation field to fixed image dimension
			resampler->SetInput( deformation );
			resampler->SetInterpolator( labelInterpolator );
			resampler->SetOutputOrigin(graph.getOrigin());//targetImage->GetOrigin());
			resampler->SetOutputSpacing ( targetImage->GetSpacing() );
			resampler->SetOutputDirection ( targetImage->GetDirection() );
			resampler->SetSize ( targetImage->GetLargestPossibleRegion().GetSize() );
			if (verbose) std::cout<<"interpolating deformation field"<<std::endl;
			resampler->Update();
//			if (defFilename!=""){
//				//		ImageUtils<LabelImageType>::writeImage(defFilename,deformation);
//				ostringstream labelfield;
//				labelfield<<defFilename<<"-l"<<l<<"-i"<<i<<".mha";
//				ImageUtils<LabelImageType>::writeImage(labelfield.str().c_str(),deformation);
//				ostringstream labelfield2;
//				labelfield2<<defFilename<<"FULL-l"<<l<<"-i"<<i<<".mha";
//				ImageUtils<LabelImageType>::writeImage(labelfield2.str().c_str(),resampler->GetOutput());
//				//
//			}
			//apply deformation to moving image

			IteratorType fixedIt(targetImage,targetImage->GetLargestPossibleRegion());
			fullDeformation=resampler->GetOutput();
			LabelIteratorType labelIt(fullDeformation,fullDeformation->GetLargestPossibleRegion());
			LabelIteratorType newLabelIt(previousFullDeformation,previousFullDeformation->GetLargestPossibleRegion());
			for (newLabelIt.GoToBegin(),fixedIt.GoToBegin(),labelIt.GoToBegin();!fixedIt.IsAtEnd();++fixedIt,++labelIt,++newLabelIt){
				ImageInterpolatorType::ContinuousIndexType idx(fixedIt.GetIndex());

				if (false){
					std::cout<<"Current displacement at "<<fixedIt.GetIndex()<<" ="<<LabelMapperType::getDisplacement(labelIt.Get())<<" with factors:"<<graph.getDisplacementFactor()<<" ="<<LabelMapperType::getDisplacement(labelIt.Get()).elementMult(graph.getDisplacementFactor())<<std::endl;
					std::cout<<"Total displacement including previous iterations ="<<LabelMapperType::getDisplacement(newLabelIt.Get())+LabelMapperType::getDisplacement(labelIt.Get()).elementMult(graph.getDisplacementFactor())<<std::endl;
					std::cout<<"Resulting point in moving image :"<<idx+LabelMapperType::getDisplacement(newLabelIt.Get())+LabelMapperType::getDisplacement(labelIt.Get()).elementMult(graph.getDisplacementFactor())<<std::endl;
					std::cout<<"Total Label :"<<labelIt.Get()<<std::endl;
				}
				idx+=LabelMapperType::getDisplacement(newLabelIt.Get());
				idx+=LabelMapperType::getDisplacement(labelIt.Get()).elementMult(graph.getDisplacementFactor());
				deformedImage->SetPixel(fixedIt.GetIndex(),segmentationInterpolator->EvaluateAtContinuousIndex(idx));
				deformedSegmentationImage->SetPixel(fixedIt.GetIndex(),LabelMapperType::getSegmentation(labelIt.Get())*65535);
				newLabelIt.Set(newLabelIt.Get()+LabelMapperType::scaleDisplacement(labelIt.Get(),graph.getDisplacementFactor()));

			}
			labelScalingFactor*=0.8;
			ostringstream deformedFilename;
			deformedFilename<<outputFilename<<"-l"<<l<<"-i"<<i<<".png";
			ostringstream deformedSegmentationFilename;
			deformedSegmentationFilename<<segmentationOutputFilename<<"-l"<<l<<"-i"<<i<<".png";
//			ImageUtils<ImageType>::writeImage(deformedFilename.str().c_str(), deformedImage);
//			ImageUtils<ImageType>::writeImage(deformedSegmentationFilename.str().c_str(), deformedSegmentationImage);


		}
	}
	ImageUtils<ImageType>::writeImage(outputFilename, deformedImage);
	ImageUtils<ImageType>::writeImage(segmentationOutputFilename, deformedSegmentationImage);


	//deformation
	if (defFilename!=""){
		//		ImageUtils<LabelImageType>::writeImage(defFilename,deformation);
		ImageUtils<LabelImageType>::writeImage(defFilename,previousFullDeformation);
		//
	}

#if 0
	//deformed image
	ostringstream deformedFilename;
	deformedFilename<<outputFilename<<"-p"<<p<<".png";
	ImagePointerType deformedSegmentationImage;
	//	deformedSegmentationImage=RLC->transformImage(movingSegmentationImage,mrfSolver.getLabelImage());
	deformedSegmentationImage=RLC->transformImage(movingImage,mrfSolver.getLabelImage());
	//	ImageUtils<ImageType>::writeImage(deformedFilename.str().c_str(), deformedSegmentationImage);
	ImageUtils<ImageType>::writeImage(outputFilename, deformedSegmentationImage);
	deformedSegmentationImage=RLC->transformImage(movingSegmentationImage,mrfSolver.getLabelImage());
	ImageUtils<ImageType>::writeImage("deformedSegmentation.png", deformedSegmentationImage);


	//segmentation
	ostringstream segmentedFilename;
	segmentedFilename<<segmentationOutputFilename<<"-p"<<p<<".png";


	ImagePointerType segmentedImage;
	segmentedImage=RLC->getSegmentationField(mrfSolver.getLabelImage());
	//	ImageUtils<ImageType>::writeImage(segmentedFilename.str().c_str(), segmentedImage);
	ImageUtils<ImageType>::writeImage(segmentationOutputFilename, segmentedImage);
#endif

	//	}

	return 1;
}