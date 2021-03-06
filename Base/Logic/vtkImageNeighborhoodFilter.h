/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkImageNeighborhoodFilter.h,v $
  Date:      $Date$
  Version:   $Revision$

=========================================================================auto=*/
///  vtkImageNeighborhoodFilter -  Augments vtkImageSpatialFilter with
/// a mask to allow neighborhoods of any shape.
/// 
/// 4 or 8 neighbor connectivity, or subclasses may define their own neighborhood
/// shapes. The mask is basically a binary (0/1) filter kernel to define
/// neighbors of interest.
//
#ifndef __vtkImageNeighborhoodFilter_h
#define __vtkImageNeighborhoodFilter_h

#include "vtkImageSpatialFilter.h"
#include "vtkSlicerBaseLogic.h"

class VTK_SLICER_BASE_LOGIC_EXPORT vtkImageNeighborhoodFilter : public vtkImageSpatialFilter
{
public:
  static vtkImageNeighborhoodFilter *New();
  vtkTypeRevisionMacro(vtkImageNeighborhoodFilter,vtkImageSpatialFilter);

  /// 
  /// Mask that defines area of interest in the neighborhood.
  /// Value is 1 for voxels of interest, 0 otherwise.
  /// This should be used when looping through neighborhood.
  unsigned char *GetMaskPointer() {return Mask;}

  /// 
  /// Use 4 or 8 neighbor connectivity in neighborhood around pixel
  /// This is in 3D
  void SetNeighborTo8();
  void SetNeighborTo4();

  /// 
  /// Get the neighborhood size
  /// (for erode class)
  vtkGetMacro(Neighbor, int);

  /// 
  /// Get the neighborhood extent, where the max/min numbers are
  /// offsets from the current voxel.  (So these numbers may be negative.)
  /// For use when looping through the neighborhood.
  void GetRelativeHoodExtent(int &hoodMin0, int &hoodMax0, int &hoodMin1,
                 int &hoodMax1, int &hoodMin2, int &hoodMax2);

  /// 
  /// Get the increments to use when looping through the mask.
  void GetMaskIncrements(vtkIdType &maskInc0, vtkIdType &maskInc1, vtkIdType &maskInc2);

  /// 
  /// set size of neighborhood of each pixel
  void SetKernelSize(int size0, int size1, int size2);

protected:
  vtkImageNeighborhoodFilter();
  ~vtkImageNeighborhoodFilter();

  int Neighbor;
  unsigned char *Mask;
};

#endif

