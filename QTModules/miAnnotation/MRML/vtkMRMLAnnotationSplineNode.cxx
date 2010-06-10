#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "vtkMRMLAnnotationSplineNode.h"
#include "vtkMatrix4x4.h"
#include "vtkAbstractTransform.h"
#include "vtkMRMLAnnotationTextDisplayNode.h"
#include "vtkMRMLAnnotationPointDisplayNode.h"
#include "vtkMRMLAnnotationLineDisplayNode.h"
#include "vtkMRMLAnnotationRulerStorageNode.h"
#include "vtkMath.h"

//------------------------------------------------------------------------------
vtkMRMLAnnotationSplineNode* vtkMRMLAnnotationSplineNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLAnnotationSplineNode");
  if(ret)
    {
    return (vtkMRMLAnnotationSplineNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLAnnotationSplineNode;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLAnnotationSplineNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLAnnotationSplineNode");
  if(ret)
    {
    return (vtkMRMLAnnotationSplineNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLAnnotationSplineNode;
}


//----------------------------------------------------------------------------
vtkMRMLAnnotationSplineNode::vtkMRMLAnnotationSplineNode()
{
  this->HideFromEditors = false;
  this->DistanceAnnotationFormat = NULL;
  this->SetDistanceAnnotationFormat("%.0f mm");
  this->Resolution = 5;
}
//----------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::Initialize(vtkMRMLScene* mrmlScene)
{
    if (!mrmlScene)
    {
        vtkErrorMacro("Scene was null!");
        return;
    }

    mrmlScene->AddNode(this);

    this->CreateAnnotationTextDisplayNode();
    this->CreateAnnotationPointDisplayNode();
    this->CreateAnnotationLineDisplayNode();
    
    this->AddText(" ",1,1);

  // Default bounds to get started
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };

    this->InvokeEvent(vtkMRMLAnnotationSplineNode::SplineNodeAddedEvent);
}

//----------------------------------------------------------------------------
vtkMRMLAnnotationSplineNode::~vtkMRMLAnnotationSplineNode()
{
  vtkDebugMacro("Destructing...." << (this->GetID() != NULL ? this->GetID() : "null id"));
  if (this->DistanceAnnotationFormat)
    {
      delete [] this->DistanceAnnotationFormat;
      this->DistanceAnnotationFormat = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  
  vtkIndent indent(nIndent);

  of << indent << " rulerDistanceAnnotationFormat=\"";
  if (this->DistanceAnnotationFormat) 
    {
      of << this->DistanceAnnotationFormat << "\"";
    }
  else 
    {
      of << "\"";
    }
  of << indent << " rulerResolution=\""<< this->Resolution << "\"";


}


//----------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::ReadXMLAttributes(const char** atts)
{
  // cout << "vtkMRMLAnnotationRulerNode::ReadXMLAttributes start"<< endl;

  int disabledModify = this->StartModify();

  this->ResetAnnotations();

  Superclass::ReadXMLAttributes(atts);

  
  while (*atts != NULL) 
    {
    const char* attName = *(atts++);
    std::string attValue(*(atts++));


    if (!strcmp(attName, "rulerResolution"))       
      {

    std::stringstream ss;
        ss << attValue;
        ss >> this->Resolution;
      }
    else if (!strcmp(attName, "rulerDistanceAnnotationFormat"))
      {
    this->SetDistanceAnnotationFormat(attValue.c_str());
      }

    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLAnnotationSplineNode *node = (vtkMRMLAnnotationSplineNode *) anode;
}

//-----------------------------------------------------------
void vtkMRMLAnnotationSplineNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);

  // Nothing to do at this point  bc vtkMRMLAnnotationDisplayNode is subclass of vtkMRMLModelDisplayNode 
  // => will be taken care of by vtkMRMLModelDisplayNode  

}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event, 
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  // Not necessary bc vtkMRMLAnnotationDisplayNode is subclass of vtkMRMLModelDisplayNode 
  // => will be taken care of  in vtkMRMLModelNode
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::PrintAnnotationInfo(ostream& os, vtkIndent indent, int titleFlag)
{
  //cout << "vtkMRMLAnnotationRulerNode::PrintAnnotationInfo" << endl;
  if (titleFlag) 
    {
      
      os <<indent << "vtkMRMLAnnotationRulerNode: Annotation Summary";
      if (this->GetName()) 
    {
      os << " of " << this->GetName();
    }
      os << endl;
    }

  Superclass::PrintAnnotationInfo(os, indent, 0);

  os << indent << "rulerDistanceAnnotationFormat: ";
  if (this->DistanceAnnotationFormat) 
    {
      os  << this->DistanceAnnotationFormat << "\n";
    }
  else 
    {
      os  << "(None)" << "\n";
    }
  os << indent << "rulerResolution: " << this->Resolution << "\n";
}

//---------------------------------------------------------------------------
int vtkMRMLAnnotationSplineNode::AddControlPoint(double newControl[3],int selectedFlag, int visibleFlag)
{
  if (this->GetNumberOfControlPoints() > 1) {
    vtkErrorMacro("AnnotationRuler: "<< this->GetName() << " cannot have more than 3 control points !");
    return -1;
  }
  return Superclass::AddControlPoint(newControl,selectedFlag,visibleFlag);
}

//---------------------------------------------------------------------------
double vtkMRMLAnnotationSplineNode::GetDistanceAnnotationScale()
{
  vtkMRMLAnnotationTextDisplayNode *node = this->GetAnnotationTextDisplayNode();
  if (!node)
    {
      return 0;
    }
  return node->GetTextScale();
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::SetDistanceAnnotationScale(double init)
{
  vtkMRMLAnnotationTextDisplayNode *node = this->GetAnnotationTextDisplayNode();
  
  if (!node)
    {
      vtkErrorMacro("AnnotationRuler: "<< this->GetName() << " cannot get AnnotationTextDisplayNode");
      return;
    }
  node->SetTextScale(init);
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::SetDistanceAnnotationVisibility(int flag)
{
  this->SetAnnotationAttribute(0,vtkMRMLAnnotationNode::TEXT_VISIBLE,flag);

}

//---------------------------------------------------------------------------
int vtkMRMLAnnotationSplineNode::GetDistanceAnnotationVisibility()
{
  return this->GetAnnotationAttribute(0, vtkMRMLAnnotationNode::TEXT_VISIBLE);
}

//---------------------------------------------------------------------------
int vtkMRMLAnnotationSplineNode::SetRuler(vtkIdType line1Id, int sel, int vis)
{
  vtkIdType line1IDPoints[2];
  this->GetEndPointsId(line1Id,line1IDPoints);

  //Change this later
  if (line1IDPoints[0]!= 0 || line1IDPoints[1] != 1)
    {
      vtkErrorMacro("Not valid line definition!");
      return -1;
    }
  this->SetSelected(sel); 
  this->SetVisible(vis); 

  return 1;
}

//---------------------------------------------------------------------------
int vtkMRMLAnnotationSplineNode::SetControlPoint(int id, double newControl[3])
{
  if (id < 0 || id > 1) {
    return 0;
  }

  int flag = Superclass::SetControlPoint(id, newControl,1,1);
  if (!flag) 
    {
      return 0;
    }
  if (this->GetNumberOfControlPoints() < 2) 
    {
      return 1;
    }

  this->AddLine(0,1,1,1);
  return 1;
}

//---------------------------------------------------------------------------
double* vtkMRMLAnnotationSplineNode::GetPointColour()
{
  vtkMRMLAnnotationPointDisplayNode *node = this->GetAnnotationPointDisplayNode();
  if (!node)
    {
      return 0;
    }
  return node->GetSelectedColor();
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::SetPointColour(double initColor[3])
{
  vtkMRMLAnnotationPointDisplayNode *node = this->GetAnnotationPointDisplayNode();
  if (!node)
    {
      vtkErrorMacro("AnnotationRuler: "<< this->GetName() << " cannot get AnnotationPointDisplayNode");
      return;
    }
  node->SetSelectedColor(initColor);
}

//---------------------------------------------------------------------------
double* vtkMRMLAnnotationSplineNode::GetDistanceAnnotationTextColour()
{
  vtkMRMLAnnotationTextDisplayNode *node = this->GetAnnotationTextDisplayNode();
  if (!node)
    {
      return 0;
    }
  return node->GetSelectedColor();
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::SetDistanceAnnotationTextColour(double initColor[3])
{
  vtkMRMLAnnotationTextDisplayNode *node = this->GetAnnotationTextDisplayNode();
  if (!node)
    {
      vtkErrorMacro("AnnotationRuler: "<< this->GetName() << " cannot get AnnotationPointDisplayNode");
      return;
    }
  node->SetSelectedColor(initColor);
}

//---------------------------------------------------------------------------
double* vtkMRMLAnnotationSplineNode::GetLineColour()
{
  vtkMRMLAnnotationLineDisplayNode *node = this->GetAnnotationLineDisplayNode();
  if (!node)
    {
      return 0;
    }
  return node->GetSelectedColor();
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::SetLineColour(double initColor[3])
{
  vtkMRMLAnnotationLineDisplayNode *node = this->GetAnnotationLineDisplayNode();
  if (!node)
    {
      vtkErrorMacro("AnnotationRuler: "<< this->GetName() << " cannot get AnnotationPointDisplayNode");
      return;
    }
  node->SetSelectedColor(initColor);
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::ApplyTransform(vtkMatrix4x4* transformMatrix)
{
  double (*matrix)[4] = transformMatrix->Element;
  double xyzIn[3];
  double xyzOut[3];
  double *p = NULL;

  // first point
  p = this->GetPosition1();
  if (p)
    {
    xyzIn[0] = p[0];
    xyzIn[1] = p[1];
    xyzIn[2] = p[2];
  
    xyzOut[0] = matrix[0][0]*xyzIn[0] + matrix[0][1]*xyzIn[1] + matrix[0][2]*xyzIn[2] + matrix[0][3];
    xyzOut[1] = matrix[1][0]*xyzIn[0] + matrix[1][1]*xyzIn[1] + matrix[1][2]*xyzIn[2] + matrix[1][3];
    xyzOut[2] = matrix[2][0]*xyzIn[0] + matrix[2][1]*xyzIn[1] + matrix[2][2]*xyzIn[2] + matrix[2][3];
    this->SetPosition1(xyzOut);
    }

  // second point
  p = this->GetPosition2();
  if (p)
    {
    xyzIn[0] = p[0];
    xyzIn[1] = p[1];
    xyzIn[2] = p[2];

    xyzOut[0] = matrix[0][0]*xyzIn[0] + matrix[0][1]*xyzIn[1] + matrix[0][2]*xyzIn[2] + matrix[0][3];
    xyzOut[1] = matrix[1][0]*xyzIn[0] + matrix[1][1]*xyzIn[1] + matrix[1][2]*xyzIn[2] + matrix[1][3];
    xyzOut[2] = matrix[2][0]*xyzIn[0] + matrix[2][1]*xyzIn[1] + matrix[2][2]*xyzIn[2] + matrix[2][3];
    this->SetPosition2(xyzOut);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationSplineNode::ApplyTransform(vtkAbstractTransform* transform)
{
  double xyzIn[3];
  double xyzOut[3];
  double *p;

  // first point
  p = this->GetPosition1();
  if (p)
    {
    xyzIn[0] = p[0];
    xyzIn[1] = p[1];
    xyzIn[2] = p[2];
    
    transform->TransformPoint(xyzIn,xyzOut);
    this->SetPosition1(xyzOut);
    }
  
  // second point
  p = this->GetPosition2();
  if (p)
    {
    xyzIn[0] = p[0];
    xyzIn[1] = p[1];
    xyzIn[2] = p[2];
    
    transform->TransformPoint(xyzIn,xyzOut);
    this->SetPosition2(xyzOut);
    }
}
