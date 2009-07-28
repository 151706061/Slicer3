/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Applications/CLI/Multiply.cxx $
  Language:  C++
  Date:      $Date: 2009-05-13 17:16:00 -0400 (Wed, 13 May 2009) $
  Version:   $Revision: 9478 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#ifdef __BORLANDC__
#define ITK_LEAN_AND_MEAN
#endif

#include "itkOrientedImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkNumericTraits.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkBSplineInterpolateImageFunction.h"

#include "itkPluginUtilities.h"
#include "MultiplyCLP.h"

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace {

template<class T> int DoIt( int argc, char * argv[], T )
{

  PARSE_ARGS;

  typedef    T       InputPixelType;
  typedef    T       OutputPixelType;

  typedef itk::OrientedImage< InputPixelType,  3 >   InputImageType;
  typedef itk::OrientedImage< OutputPixelType, 3 >   OutputImageType;

  typedef itk::ImageFileReader< InputImageType >  ReaderType;
  typedef itk::ImageFileWriter< OutputImageType > WriterType;

  typedef itk::BSplineInterpolateImageFunction<InputImageType> Interpolator;

  typename ReaderType::Pointer reader1 = ReaderType::New();
  itk::PluginFilterWatcher watchReader1(reader1, "Read Volume 1",
                                        CLPProcessInformation);
  
  typename ReaderType::Pointer reader2 = ReaderType::New();
  itk::PluginFilterWatcher watchReader2(reader2,
                                        "Read Volume 2",
                                        CLPProcessInformation);
  reader1->SetFileName( inputVolume1.c_str() );
  reader2->SetFileName( inputVolume2.c_str() );

  reader1->Update();
  reader2->Update();

  typename OutputImageType::Pointer output = OutputImageType::New();
  output->CopyInformation(reader1->GetOutput());
  output->SetBufferedRegion(output->GetLargestPossibleRegion());
  output->Allocate();

  typedef itk::ImageRegionIteratorWithIndex<OutputImageType> Iterator;
  typename Interpolator::Pointer interp = Interpolator::New();
  interp->SetInputImage(reader2->GetOutput());
  interp->SetSplineOrder(3);
  
  Iterator it(output, output->GetBufferedRegion());
  Iterator inIt(reader1->GetOutput(), output->GetBufferedRegion());

  typename OutputImageType::PointType point;
  typename Interpolator::ContinuousIndexType index;
  bool isinside;

  typedef typename itk::NumericTraits<OutputPixelType>::AccumulateType AccumulateType;
  AccumulateType tmp;
  while ( ! it.IsAtEnd() )
    {
    output->TransformIndexToPhysicalPoint(it.GetIndex(), point);
    isinside = reader2->GetOutput()->TransformPhysicalPointToContinuousIndex(point, index);

    tmp = static_cast<AccumulateType>(inIt.Get());
    if (isinside)
      {
      tmp *= interp->EvaluateAtContinuousIndex(index);
      }
    else
      {
      tmp = itk::NumericTraits<AccumulateType>::Zero;
      }

    if (tmp < itk::NumericTraits<OutputPixelType>::NonpositiveMin())
      {
      it.Set(itk::NumericTraits<OutputPixelType>::NonpositiveMin());
      }
    else if (tmp > itk::NumericTraits<OutputPixelType>::max())
      {
      it.Set(itk::NumericTraits<OutputPixelType>::max());
      }
    else
      {
      it.Set(static_cast<OutputPixelType>(tmp));
      }

    ++it;
    ++inIt;
    }

  typename WriterType::Pointer writer = WriterType::New();
  itk::PluginFilterWatcher watchWriter(writer,
                                       "Write Volume",
                                       CLPProcessInformation);
  writer->SetFileName( outputVolume.c_str() );
  writer->SetInput( output );
  writer->Update();

  return EXIT_SUCCESS;
}

} // end of anonymous namespace


int main( int argc, char * argv[] )
{
  
  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  try
    {
    itk::GetImageType (inputVolume1, pixelType, componentType);

    // This filter handles all types on input, but only produces
    // signed types
    
    switch (componentType)
      {
      case itk::ImageIOBase::UCHAR:
      case itk::ImageIOBase::CHAR:
        return DoIt( argc, argv, static_cast<char>(0));
        break;
      case itk::ImageIOBase::USHORT:
      case itk::ImageIOBase::SHORT:
        return DoIt( argc, argv, static_cast<short>(0));
        break;
      case itk::ImageIOBase::UINT:
      case itk::ImageIOBase::INT:
        return DoIt( argc, argv, static_cast<int>(0));
        break;
      case itk::ImageIOBase::ULONG:
      case itk::ImageIOBase::LONG:
        return DoIt( argc, argv, static_cast<long>(0));
        break;
      case itk::ImageIOBase::FLOAT:
        return DoIt( argc, argv, static_cast<float>(0));
        break;
      case itk::ImageIOBase::DOUBLE:
        return DoIt( argc, argv, static_cast<double>(0));
        break;
      case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
      default:
        std::cout << "unknown component type" << std::endl;
        break;
      }
    }
  catch( itk::ExceptionObject &excep)
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
