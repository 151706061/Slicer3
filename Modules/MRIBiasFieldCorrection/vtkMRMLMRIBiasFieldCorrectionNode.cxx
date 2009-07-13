/*=auto==============================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All
  Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLMRIBiasFieldCorrectionNode.cxx,v $
  Date:      $Date: 2006/03/17 15:10:10 $
  Version:   $Revision: 1.2 $
  Author:    $Nicolas Rannou (BWH), Sylvain Jaume (MIT)$

==============================================================auto=*/

#include <iostream>
#include <sstream>

#include "vtkMRMLMRIBiasFieldCorrectionNode.h"
#include "vtkMRMLScene.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkMRMLMRIBiasFieldCorrectionNode* vtkMRMLMRIBiasFieldCorrectionNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance(
      "vtkMRMLMRIBiasFieldCorrectionNode");

  if (ret)
    {
    return (vtkMRMLMRIBiasFieldCorrectionNode*)ret;
    }

  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLMRIBiasFieldCorrectionNode;
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLMRIBiasFieldCorrectionNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance(
      "vtkMRMLMRIBiasFieldCorrectionNode");

  if (ret)
    {
    return (vtkMRMLMRIBiasFieldCorrectionNode*)ret;
    }

  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLMRIBiasFieldCorrectionNode;
}

//---------------------------------------------------------------------------
vtkMRMLMRIBiasFieldCorrectionNode::vtkMRMLMRIBiasFieldCorrectionNode()
{
  this->MaskThreshold         = 0.0;
  this->ShrinkFactor          = 3.0;
  this->NumberOfFittingLevels = 50;
  this->NumberOfIterations    = 4;
  this->WienerFilterNoise     = 0.1;
  this->BiasField             = 0.15;
  this->ConvergenceThreshold  = 0.001;

  this->InputVolumeRef        = NULL;
  this->OutputVolumeRef       = NULL;
  this->StorageVolumeRef      = NULL;
  this->MaskVolumeRef         = NULL;

  this->HideFromEditors       = true;
}

//---------------------------------------------------------------------------
vtkMRMLMRIBiasFieldCorrectionNode::~vtkMRMLMRIBiasFieldCorrectionNode()
{
  this->SetInputVolumeRef(NULL);
  this->SetOutputVolumeRef(NULL);
  this->SetStorageVolumeRef(NULL);
  this->SetMaskVolumeRef(NULL);
}

//---------------------------------------------------------------------------
void vtkMRMLMRIBiasFieldCorrectionNode::WriteXML(ostream& of, int nIndent)
{
  // Write all MRML node attributes into output stream
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);

  {
  std::stringstream ss;
  ss << this->MaskThreshold;
  of << indent << " MaskThreshold=\"" << ss.str() << "\"";
  }

  {
  std::stringstream ss;
  ss << this->ShrinkFactor;
  of << indent << " ShrinkFactor=\"" << ss.str() << "\"";
  }

  {
  std::stringstream ss;
  ss << this->NumberOfIterations;
  of << indent << " NumberOfIterations=\"" << ss.str() << "\"";
  }

  {
  std::stringstream ss;
  ss << this->NumberOfFittingLevels;
  of << indent << " NumberOfFittingLevels=\"" << ss.str() << "\"";
  }

  {
  std::stringstream ss;
  ss << this->ConvergenceThreshold;
  of << indent << " ConvergenceThreshold=\"" << ss.str() << "\"";
  }

  {
  std::stringstream ss;
  ss << this->BiasField;
  of << indent << " BiasField=\"" << ss.str() << "\"";
  }

  {
  std::stringstream ss;
  ss << this->WienerFilterNoise;
  of << indent << " WienerFilterNoise=\"" << ss.str() << "\"";
  }

  {
  std::stringstream ss;
  if (this->InputVolumeRef)
    {
    ss << this->InputVolumeRef;
    of << indent << " InputVolumeRef=\"" << ss.str() << "\"";
    }
  }

  {
  std::stringstream ss;
  if (this->OutputVolumeRef)
    {
    ss << this->OutputVolumeRef;
    of << indent << " OutputVolumeRef=\"" << ss.str() << "\"";
    }
  }

  {
  std::stringstream ss;
  if (this->StorageVolumeRef)
    {
    ss << this->StorageVolumeRef;
    of << indent << " StorageVolumeRef=\"" << ss.str() << "\"";
    }
  }

  {
  std::stringstream ss;
  if (this->MaskVolumeRef)
    {
    ss << this->MaskVolumeRef;
    of << indent << " MaskVolumeRef=\"" << ss.str() << "\"";
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLMRIBiasFieldCorrectionNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL)
    {
    attName  = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName,"ConvergenceThreshold"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->ConvergenceThreshold;
      }

    if (!strcmp(attName,"BiasField"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->BiasField;
      }

    if (!strcmp(attName,"WienerFilterNoise"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->WienerFilterNoise;
      }

    if (!strcmp(attName,"MaskThreshold"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->MaskThreshold;
      }

    if (!strcmp(attName,"ShrinkFactor"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->ShrinkFactor;
      }

    if (!strcmp(attName,"NumberOfIterations"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->NumberOfIterations;
      }

    if (!strcmp(attName,"NumberOfFittingLevels"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->NumberOfFittingLevels;
      }

    if (!strcmp(attName,"InputVolumeRef"))
      {
      this->SetInputVolumeRef(attValue);
      this->Scene->AddReferencedNodeID(this->InputVolumeRef,this);
      }

    if (!strcmp(attName,"OutputVolumeRef"))
      {
      this->SetOutputVolumeRef(attValue);
      this->Scene->AddReferencedNodeID(this->OutputVolumeRef,this);
      }

    if (!strcmp(attName,"StorageVolumeRef"))
      {
      this->SetStorageVolumeRef(attValue);
      this->Scene->AddReferencedNodeID(this->StorageVolumeRef,this);
      }

     if (!strcmp(attName,"MaskVolumeRef"))
      {
      this->SetMaskVolumeRef(attValue);
      this->Scene->AddReferencedNodeID(this->MaskVolumeRef,this);
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLMRIBiasFieldCorrectionNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLMRIBiasFieldCorrectionNode *node =
    (vtkMRMLMRIBiasFieldCorrectionNode *) anode;

  this->SetMaskThreshold(node->MaskThreshold);
  this->SetNumberOfIterations(node->NumberOfIterations);
  this->SetNumberOfFittingLevels(node->NumberOfFittingLevels);
  this->SetShrinkFactor(node->ShrinkFactor);
  this->SetConvergenceThreshold(node->ConvergenceThreshold);
  this->SetBiasField(node->BiasField);
  this->SetWienerFilterNoise(node->WienerFilterNoise);

  this->SetInputVolumeRef(node->InputVolumeRef);
  this->SetOutputVolumeRef(node->OutputVolumeRef);
  this->SetStorageVolumeRef(node->StorageVolumeRef);
  this->SetMaskVolumeRef(node->MaskVolumeRef);
}

//----------------------------------------------------------------------------
void vtkMRMLMRIBiasFieldCorrectionNode::PrintSelf(ostream& os, vtkIndent
    indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "MaskThreshold:         " << this->MaskThreshold     <<"\n";
  os << indent << "ShrinkFactor:          " << this->ShrinkFactor      <<"\n";
  os << indent << "BiasField:             " << this->BiasField         <<"\n";
  os << indent << "WienerFilterNoise:     " << this->WienerFilterNoise <<"\n";

  os << indent << "NumberOfIterations:    " << this->NumberOfIterations    <<
    "\n";
  os << indent << "NumberOfFittingLevels: " << this->NumberOfFittingLevels <<
    "\n";
  os << indent << "ConvergenceThreshold:  " << this->ConvergenceThreshold  <<
    "\n";

  os << indent << "InputVolumeRef:   " <<
   (this->InputVolumeRef ? this->InputVolumeRef     : "(none)") << "\n";
  os << indent << "OutputVolumeRef:   " <<
   (this->OutputVolumeRef ? this->OutputVolumeRef   : "(none)") << "\n";
  os << indent << "StorageVolumeRef:   " <<
   (this->StorageVolumeRef ? this->StorageVolumeRef : "(none)") << "\n";
   os << indent << "MaskVolumeRef:   " <<
   (this->MaskVolumeRef ? this->MaskVolumeRef       : "(none)") << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLMRIBiasFieldCorrectionNode::UpdateReferenceID(const char *oldID,
    const char *newID)
{
  if (strcmp(oldID, this->InputVolumeRef) == 0)
    {
    this->SetInputVolumeRef(newID);
    }
  if (strcmp(oldID, this->OutputVolumeRef) == 0)
    {
    this->SetOutputVolumeRef(newID);
    }
  if (strcmp(oldID, this->StorageVolumeRef) == 0)
    {
    this->SetStorageVolumeRef(newID);
    }
  if (strcmp(oldID, this->MaskVolumeRef) == 0)
    {
    this->SetMaskVolumeRef(newID);
    }
}

