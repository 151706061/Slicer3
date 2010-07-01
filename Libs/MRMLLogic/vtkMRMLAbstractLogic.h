/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLAbstractLogic.h,v $
  Date:      $Date: 2010-06-19 12:48:04 -0400 (Sat, 19 Jun 2010) $
  Version:   $Revision: 13859 $

=========================================================================auto=*/
///  vtkMRMLAbstractLogic - Superclass for MRML logic classes
/// 
/// Superclass for all MRML logic classes.
/// There must be a corresponding UI class that handles all UI interaction

#ifndef __vtkMRMLAbstractLogic_h
#define __vtkMRMLAbstractLogic_h

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkObserverManager.h>

// VTK includes
#include <vtkCommand.h>
#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkIntArray.h>
#include <vtkUnsignedLongArray.h>

#include "vtkMRMLLogicWin32Header.h"

//BTX

//----------------------------------------------------------------------------
// Convenient macros

//----------------------------------------------------------------------------
#ifndef vtkSetMRMLNodeMacro
#define vtkSetMRMLNodeMacro(node,value)  {                                         \
  vtkObject *oldNode = (node);                                                     \
  this->GetMRMLObserverManager()->SetObject(vtkObjectPointer(&(node)), (value));   \
  if (oldNode != (node))                                                           \
    {                                                                              \
    this->InvokeEvent(vtkCommand::ModifiedEvent);                                  \
    }                                                                              \
};
#endif

//----------------------------------------------------------------------------
#ifndef vtkSetAndObserveMRMLNodeMacro
#define vtkSetAndObserveMRMLNodeMacro(node,value) {                                        \
  vtkObject *oldNode = (node);                                                             \
  this->GetMRMLObserverManager()->SetAndObserveObject(vtkObjectPointer(&(node)), (value)); \
  if (oldNode != (node))                                                                   \
    {                                                                                      \
    this->InvokeEvent(vtkCommand::ModifiedEvent);                                          \
    }                                                                                      \
};
#endif

//----------------------------------------------------------------------------
#ifndef vtkSetAndObserveNoModifyMRMLNodeMacro
#define vtkSetAndObserveNoModifyMRMLNodeMacro(node,value) {                                 \
  this->GetMRMLObserverManager()->SetAndObserveObject(vtkObjectPointer(&(node)), (value));  \
};
#endif

//----------------------------------------------------------------------------
#ifndef vtkSetAndObserveMRMLNodeEventsMacro
#define vtkSetAndObserveMRMLNodeEventsMacro(node,value,events) {                        \
  vtkObject *oldNode = (node);                                                          \
  this->GetMRMLObserverManager()->SetAndObserveObjectEvents(                            \
     vtkObjectPointer(&(node)), (value), (events));                                     \
  if (oldNode != (node))                                                                \
    {                                                                                   \
    this->InvokeEvent(vtkCommand::ModifiedEvent);                                       \
    }                                                                                   \
};
#endif

//ETX

class VTK_MRML_LOGIC_EXPORT vtkMRMLAbstractLogic : public vtkObject 
{
public:
  
  static vtkMRMLAbstractLogic *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkMRMLAbstractLogic, vtkObject);

  ///
  /// Return a reference to the current MRML scene
  vtkMRMLScene * GetMRMLScene();

  ///
  /// Set and/or observe the MRMLScene
  void SetMRMLScene(vtkMRMLScene * newScene);
  void SetAndObserveMRMLScene(vtkMRMLScene * newScene);
  void SetAndObserveMRMLSceneEvents(vtkMRMLScene * newScene, vtkIntArray * events);

  virtual void ProcessMRMLEvents(vtkObject * /*caller*/, unsigned long /*event*/,
                                 void * /*callData*/){ };

  virtual void ProcessLogicEvents(vtkObject * /*caller*/, unsigned long /*event*/,
                                  void * /*callData*/){ };

  /// Get MRML CallbackCommand
  vtkCallbackCommand * GetMRMLCallbackCommand();

  /// Get MRML ObserverManager
  vtkObserverManager * GetMRMLObserverManager();

  /// Set / Get flags to avoid event loops
  void SetInMRMLCallbackFlag(int flag);
  int GetInMRMLCallbackFlag();

  /// Get Logic CallbackCommand
  vtkCallbackCommand * GetLogicCallbackCommand();

  /// Set / Get flags to avoid event loops
  void SetInLogicCallbackFlag(int flag);
  int GetInLogicCallbackFlag();

protected:

  vtkMRMLAbstractLogic();
  virtual ~vtkMRMLAbstractLogic();
  
  /// Register node classes into the MRML scene. Called each time a new scene
  /// is set. Do nothing by default. Can be reimplemented in derivated classes.
  virtual void RegisterNodes(){}

  //BTX
  /// MRMLCallback is a static function to relay modified events from the MRML Scene
  /// In subclass, MRMLCallback can also be used to relay event from observe MRML node(s)
  static void MRMLCallback(vtkObject *caller, unsigned long eid, void *clientData, void *callData);

  /// LogicCallback is a static function to relay modified events from the Logic
  static void LogicCallback(vtkObject *caller, unsigned long eid, void *clientData, void *callData);
  //ETX
  
private:

  vtkMRMLAbstractLogic(const vtkMRMLAbstractLogic&); // Not implemented
  void operator=(const vtkMRMLAbstractLogic&);       // Not implemented

  //BTX
  class vtkInternal;
  vtkInternal * Internal;
  //ETX

};

#endif

