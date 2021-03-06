/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkSlicerFiducialsLogic.h,v $
  Date:      $Date$
  Version:   $Revision$

=========================================================================auto=*/

///  vtkSlicerFiducialsLogic - slicer logic class for volumes manipulation
/// 
/// This class manages the logic associated with reading, saving,
/// and changing propertied of the volumes


#ifndef __vtkSlicerFiducialsLogic_h
#define __vtkSlicerFiducialsLogic_h

#include <stdlib.h>

#include "vtkSlicerBaseLogic.h"
#include "vtkSlicerLogic.h"

#include "vtkMRML.h"
#include "vtkMRMLFiducial.h"
#include "vtkMRMLFiducialListNode.h"

class VTK_SLICER_BASE_LOGIC_EXPORT vtkSlicerFiducialsLogic : public vtkSlicerLogic 
{
  public:
  
  /// The Usual vtk class functions
  static vtkSlicerFiducialsLogic *New();
  vtkTypeRevisionMacro(vtkSlicerFiducialsLogic,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  ///
  /// Get the currently selected list from the scene. Returns NULL if no
  /// selection node or no active fiducial list id is set on the selection
  /// node.
  vtkMRMLFiducialListNode *GetSelectedList();

  /// 
  /// Create new mrml node for a full list, make it the selected list, and clear up local pointers
  void AddFiducialListSelected();
  
  /// 
  /// Create new mrml node and associated display node for a full list,
  /// return the node
  vtkMRMLFiducialListNode * AddFiducialList();

  /// 
  /// Add a fiducial to the currently selected list, as kept in the
  /// vtkMRMLSelectionNode
  /// Returns the index of the new fiducial in the list, -1 on failure
  /// AddFiducialSelected includes a selected flag option, AddFiducial calls
  /// AddFiducialSelected with selected set to false.
  int AddFiducial(float x, float y, float z);
  int AddFiducialSelected (float x, float y, float z, int selected);

  ///
  /// Add a fiducial, but transform it first by the inverse of any
  /// transformation node on the list. Called by Pick methods. Calls
  /// AddFiducialSelected with the transformed x,y,z and same selected flag
  /// (defaults to 0).
  int AddFiducialPicked(float x, float y, float z, int selected = 0);
  
  /// 
  /// Load a fiducial list from file, returns NULL on failure
  vtkMRMLFiducialListNode *LoadFiducialList(const char *path);

  /// 
  /// Update logic state when MRML scene changes
  void ProcessMRMLEvents();
  //BTX
  using vtkSlicerLogic::ProcessMRMLEvents;
  //ETX
    
protected:
  vtkSlicerFiducialsLogic();
  ~vtkSlicerFiducialsLogic();
  vtkSlicerFiducialsLogic(const vtkSlicerFiducialsLogic&);
  void operator=(const vtkSlicerFiducialsLogic&);

};

#endif

