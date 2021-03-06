/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkSlicerSliceLogic.cxx,v $
  Date:      $Date$
  Version:   $Revision$

=========================================================================auto=*/

#include "vtkSmartPointer.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkPlaneSource.h"
#include "vtkPoints.h"
#include "vtkImageBlend.h"
#include "vtkImageMathematics.h"
#include "vtkSmartPointer.h"

#include "vtkMRMLCrosshairNode.h"
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLVolumeNode.h"
#include "vtkMRMLDiffusionTensorVolumeNode.h"
#include "vtkMRMLDiffusionTensorVolumeSliceDisplayNode.h"
#include "vtkMRMLProceduralColorNode.h"

#include "vtkSlicerSliceLogic.h"

#include <sstream>
#include <iostream>

vtkCxxRevisionMacro(vtkSlicerSliceLogic, "$Revision$");
vtkStandardNewMacro(vtkSlicerSliceLogic);

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

const int vtkSlicerSliceLogic::SLICE_INDEX_ROTATED=-1;
const int vtkSlicerSliceLogic::SLICE_INDEX_OUT_OF_VOLUME=-2;
const int vtkSlicerSliceLogic::SLICE_INDEX_NO_VOLUME=-3;

//----------------------------------------------------------------------------
vtkSlicerSliceLogic::vtkSlicerSliceLogic()
{
  this->BackgroundLayer = NULL;
  this->ForegroundLayer = NULL;
  this->LabelLayer = NULL;
  this->SliceNode = NULL;
  this->SliceCompositeNode = NULL;
  this->ForegroundOpacity = 0.5; // Start by blending fg/bg
  this->LabelOpacity = 1.0;
  this->Blend = vtkImageBlend::New();
  this->ExtractModelTexture = vtkImageReslice::New();
  this->ExtractModelTexture->SetOutputDimensionality (2);
  this->PolyDataCollection = vtkPolyDataCollection::New();
  this->LookupTableCollection = vtkCollection::New();
  this->SetForegroundOpacity(this->ForegroundOpacity);
  this->SetLabelOpacity(this->LabelOpacity);
  this->SliceModelNode = NULL;
  this->SliceModelTransformNode = NULL;
  this->Name = NULL;
  this->SetName("");
  this->SliceModelDisplayNode = NULL;
  this->ImageData = NULL;
  this->SliceSpacing[0] = this->SliceSpacing[1] = this->SliceSpacing[2] = 1;
}

//----------------------------------------------------------------------------
vtkSlicerSliceLogic::~vtkSlicerSliceLogic()
{
  this->SetSliceNode(NULL);

  if (this->ImageData)
    {
    this->ImageData->Delete();
    }

  if ( this->Blend ) 
    {
    this->Blend->Delete();
    this->Blend = NULL;
    }
  if ( this->ExtractModelTexture ) 
    {
    this->ExtractModelTexture->Delete();
    this->ExtractModelTexture = NULL;
    }
  this->PolyDataCollection->Delete();
  this->LookupTableCollection->Delete();
 
  this->SetBackgroundLayer (NULL);
  this->SetForegroundLayer (NULL);
  this->SetLabelLayer (NULL);
 
  if ( this->SliceCompositeNode ) 
    {
    vtkSetAndObserveMRMLNodeMacro( this->SliceCompositeNode, NULL );
    }

  this->SetName(NULL);

  this->DeleteSliceModel();

}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::UpdateSliceNode()
{
  // find SliceNode in the scene
  vtkMRMLSliceNode *node= NULL;
  int nnodes = this->MRMLScene->GetNumberOfNodesByClass("vtkMRMLSliceNode");
  for (int n=0; n<nnodes; n++)
    {
    node = vtkMRMLSliceNode::SafeDownCast (
          this->MRMLScene->GetNthNodeByClass(n, "vtkMRMLSliceNode"));
    if (node->GetLayoutName() && !strcmp(node->GetLayoutName(), this->GetName()))
      {
      break;
      }
    node = NULL;
    }

  if ( this->SliceNode != NULL && node != NULL && 
    strcmp(this->SliceNode->GetID(), node->GetID()) != 0 )
    {
    // local SliceNode is out of sync with the scene
    this->SetSliceNode (NULL);
    }

  if ( this->SliceNode == NULL )
    {
    if ( node == NULL )
      {
      node = vtkMRMLSliceNode::New();
      node->SetName(this->GetName());
      node->SetLayoutName(this->GetName());
      node->SetSingletonTag(this->GetName());
      this->SetSliceNode (node);
      this->UpdateSliceNodeFromLayout();
      node->Delete();
      }
    else
      {

      this->SetSliceNode (node);
      }
    }

  if ( this->MRMLScene->GetNodeByID(this->SliceNode->GetID()) == NULL)
    {
    // local node not in the scene
    node = this->SliceNode;
    node->Register(this);
    this->SetSliceNode (NULL);
    this->MRMLScene->AddNodeNoNotify(node);
    this->SetSliceNode (node);
    node->UnRegister(this);
    }
}

//----------------------------------------------------------------------------

void vtkSlicerSliceLogic::UpdateSliceNodeFromLayout()
{
  if (this->SliceNode == NULL)
    {
    return;
    }

  if ( !strcmp( this->GetName(), "Red" ) )
    {
    this->SliceNode->SetOrientationToAxial();
    }
  if ( !strcmp( this->GetName(), "Yellow" ) )
    {
    this->SliceNode->SetOrientationToSagittal();
    }
  if ( !strcmp( this->GetName(), "Green" ) )
    {
    this->SliceNode->SetOrientationToCoronal();
    }
}

//----------------------------------------------------------------------------

void vtkSlicerSliceLogic::UpdateSliceCompositeNode()
{
  // find SliceCompositeNode in the scene
  vtkMRMLSliceCompositeNode *node= NULL;
  int nnodes = this->MRMLScene->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
  for (int n=0; n<nnodes; n++)
    {
    node = vtkMRMLSliceCompositeNode::SafeDownCast (
          this->MRMLScene->GetNthNodeByClass(n, "vtkMRMLSliceCompositeNode"));
    if (node->GetLayoutName() && !strcmp(node->GetLayoutName(), this->GetName()))
      {
      break;
      }
    node = NULL;
    }

  if ( this->SliceCompositeNode != NULL && node != NULL && 
        strcmp(this->SliceCompositeNode->GetID(), node->GetID()) != 0 )
    {
    // local SliceCompositeNode is out of sync with the scene
    this->SetSliceCompositeNode (NULL);
    }

  if ( this->SliceCompositeNode == NULL )
    {
    if ( node == NULL )
      {
      node = vtkMRMLSliceCompositeNode::New();
      node->SetLayoutName(this->GetName());
      node->SetSingletonTag(this->GetName());
      this->SetSliceCompositeNode (node);
      node->Delete();
      }
    else
      {
      this->SetSliceCompositeNode (node);
      }
    }

  if ( this->MRMLScene->GetNodeByID(this->SliceCompositeNode->GetID()) == NULL)
    {
    // local node not in the scene
    node = this->SliceCompositeNode;
    node->Register(this);
    this->SetSliceCompositeNode (NULL);
    this->MRMLScene->AddNodeNoNotify(node);
    this->SetSliceCompositeNode (node);
    node->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::ProcessMRMLEvents(vtkObject * caller, 
                                            unsigned long event, 
                                            void * callData )
{
 //
  // if you don't have a node yet, look in the scene to see if 
  // one exists for you to use.  If not, create one and add it to the scene
  //  
  if ( vtkMRMLScene::SafeDownCast(caller) == this->MRMLScene 
    && (event == vtkMRMLScene::NodeAddedEvent || event == vtkMRMLScene::NodeRemovedEvent ) )
    {
    vtkMRMLNode *node =  reinterpret_cast<vtkMRMLNode*> (callData);
    if (node == NULL || !(node->IsA("vtkMRMLSliceCompositeNode") || node->IsA("vtkMRMLSliceNode") || node->IsA("vtkMRMLVolumeNode")) )
      {
      return;
      }
    }

  if (event == vtkMRMLScene::SceneCloseEvent) 
    {
    this->UpdateSliceNodeFromLayout();
    this->DeleteSliceModel();

    return;
    }

  // Set up the nodes and models
  this->CreateSliceModel();
  this->UpdateSliceNode();
  this->UpdateSliceCompositeNode();

  //
  // check that our referenced nodes exist, and if not set to None
  //

  /** PROBABLY DONT NEED TO DO THAT And it causes load scene to override ID's
  if ( this->MRMLScene->GetNodeByID( this->SliceCompositeNode->GetForegroundVolumeID() ) == NULL )
    {
    this->SliceCompositeNode->SetForegroundVolumeID(NULL);
    }

  if ( this->MRMLScene->GetNodeByID( this->SliceCompositeNode->GetLabelVolumeID() ) == NULL )
    {
    this->SliceCompositeNode->SetLabelVolumeID(NULL);
    }

  if ( this->MRMLScene->GetNodeByID( this->SliceCompositeNode->GetBackgroundVolumeID() ) == NULL )
    {
    this->SliceCompositeNode->SetBackgroundVolumeID(NULL);
    }
   **/

  if (event != vtkMRMLScene::NewSceneEvent) 
    {
    this->UpdatePipeline();
    }

  //
  // On a new scene or restore, create the singleton for the default crosshair
  // for navigation or cursor if it doesn't already exist in scene
  //
  if ( vtkMRMLScene::SafeDownCast(caller) && 
        ( event == vtkMRMLScene::NewSceneEvent || event == vtkMRMLScene::SceneRestoredEvent ) )
    {
    vtkMRMLScene *scene =  vtkMRMLScene::SafeDownCast(caller);
    int n, numberOfCrosshairs = scene->GetNumberOfNodesByClass("vtkMRMLCrosshairNode");
    int foundDefault = 0;
    vtkMRMLCrosshairNode *crosshair;
    for (n = 0; n < numberOfCrosshairs; n++)
      {
      crosshair = vtkMRMLCrosshairNode::SafeDownCast( scene->GetNthNodeByClass( n, "vtkMRMLCrosshairNode" ) );
      if ( crosshair && !strcmp( "default", crosshair->GetCrosshairName() ) )
        {
        foundDefault = 1;
        }
      }

    if (!foundDefault)
      {
      crosshair = vtkMRMLCrosshairNode::New();
      crosshair->SetCrosshairName("default");
      crosshair->NavigationOn();
      scene->AddNode( crosshair );
      crosshair->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::ProcessLogicEvents()
{

  //
  // if we don't have layers yet, create them 
  //
  if ( this->BackgroundLayer == NULL )
    {
    vtkSlicerSliceLayerLogic *layer = vtkSlicerSliceLayerLogic::New();
    this->SetBackgroundLayer (layer);
    layer->Delete();
    }
  if ( this->ForegroundLayer == NULL )
    {
    vtkSlicerSliceLayerLogic *layer = vtkSlicerSliceLayerLogic::New();
    this->SetForegroundLayer (layer);
    layer->Delete();
    }
  if ( this->LabelLayer == NULL )
    {
    vtkSlicerSliceLayerLogic *layer = vtkSlicerSliceLayerLogic::New();
    // turn on using the label outline only in this layer
    layer->IsLabelLayerOn();
    this->SetLabelLayer (layer);
    layer->Delete();
    }
  // Update slice plane geometry
  if (this->SliceNode != NULL && this->GetSliceModelNode() != NULL 
      && this->MRMLScene->GetNodeByID( this->SliceModelNode->GetID() ) != NULL && 
        this->SliceModelNode->GetPolyData() != NULL )
    {


    vtkPoints *points = this->SliceModelNode->GetPolyData()->GetPoints();
    unsigned int *dims = this->SliceNode->GetDimensions();
    vtkMatrix4x4 *xyToRAS = this->SliceNode->GetXYToRAS();


#ifdef USE_IMAGE_ACTOR // not defined
    // set the transform for the slice model for use by an image actor in the viewer
    this->SliceModelTransformNode->GetMatrixTransformToParent()->DeepCopy( xyToRAS );

    // set the plane corner point for use in a model (deprecated)
    // TODO: remove this block
    double pt[3]={0,0,0};
    points->SetPoint(0, pt);
    pt[0] = dims[0];
    points->SetPoint(1, pt);
    pt[0] = 0;
    pt[1] = dims[1];
    points->SetPoint(2, pt);
    pt[0] = dims[0];
    pt[1] = dims[1];
    points->SetPoint(3, pt);
#else
    // set the plane corner point for use in a model
    double inPt[4]={0,0,0,1};
    double outPt[4];
    double *outPt3 = outPt;

    // set the z position to be the active slice (from the lightbox)
    inPt[2] = this->SliceNode->GetActiveSlice();

    xyToRAS->MultiplyPoint(inPt, outPt);
    points->SetPoint(0, outPt3);

    inPt[0] = dims[0];
    xyToRAS->MultiplyPoint(inPt, outPt);
    points->SetPoint(1, outPt3);

    inPt[0] = 0;
    inPt[1] = dims[1];
    xyToRAS->MultiplyPoint(inPt, outPt);
    points->SetPoint(2, outPt3);

    inPt[0] = dims[0];
    inPt[1] = dims[1];
    xyToRAS->MultiplyPoint(inPt, outPt);
    points->SetPoint(3, outPt3);
#endif

    this->UpdatePipeline();
    this->SliceModelNode->GetPolyData()->Modified();
    vtkMRMLModelDisplayNode *modelDisplayNode = this->SliceModelNode->GetModelDisplayNode();
    if ( modelDisplayNode )
      {
      modelDisplayNode->SetVisibility( this->SliceNode->GetSliceVisible() );
      if ( this->SliceCompositeNode != NULL )
        {
        modelDisplayNode->SetSliceIntersectionVisibility( this->SliceCompositeNode->GetSliceIntersectionVisibility() );
        }
      }
    }

  // This is called when a slice layer is modified, so pass it on
  // to anyone interested in changes to this sub-pipeline
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SetSliceNode(vtkMRMLSliceNode *sliceNode)
{
    // Don't directly observe the slice node -- the layers will observe it and
    // will notify us when things have changed.
    // This class takes care of passing the one slice node to each of the layers
    // so that users of this class only need to set the node one place.
  vtkSetMRMLNodeMacro( this->SliceNode, sliceNode );

  if (this->BackgroundLayer)
    {
    this->BackgroundLayer->SetSliceNode(sliceNode);
    }
  if (this->ForegroundLayer)
    {
    this->ForegroundLayer->SetSliceNode(sliceNode);
    }
  if (this->LabelLayer)
    {
    this->LabelLayer->SetSliceNode(sliceNode);
    }

  this->Modified();

}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SetSliceCompositeNode(vtkMRMLSliceCompositeNode *sliceCompositeNode)
{
    // Observe the composite node, since this holds the parameters for
    // this pipeline
  vtkSetAndObserveMRMLNodeMacro( this->SliceCompositeNode, sliceCompositeNode );
  this->UpdatePipeline();

}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SetBackgroundLayer(vtkSlicerSliceLayerLogic *backgroundLayer)
{
  if (this->BackgroundLayer)
    {
    this->BackgroundLayer->SetAndObserveMRMLScene( NULL );
    this->BackgroundLayer->Delete();
    }
  this->BackgroundLayer = backgroundLayer;

  if (this->BackgroundLayer)
    {
    this->BackgroundLayer->Register(this);

    vtkIntArray *events = vtkIntArray::New();
    events->InsertNextValue(vtkMRMLScene::NewSceneEvent);
    events->InsertNextValue(vtkMRMLScene::SceneCloseEvent);
    events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
    events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
    this->BackgroundLayer->SetAndObserveMRMLSceneEvents ( this->MRMLScene, events );
    events->Delete();

    this->BackgroundLayer->SetSliceNode(SliceNode);
    vtkEventBroker::GetInstance()->AddObservation(
        this->BackgroundLayer, vtkCommand::ModifiedEvent, this, this->LogicCallbackCommand );
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SetForegroundLayer(vtkSlicerSliceLayerLogic *foregroundLayer)
{
  if (this->ForegroundLayer)
    {
    this->ForegroundLayer->SetAndObserveMRMLScene( NULL );
    this->ForegroundLayer->Delete();
    }
  this->ForegroundLayer = foregroundLayer;

  if (this->ForegroundLayer)
    {
    this->ForegroundLayer->Register(this);

    vtkIntArray *events = vtkIntArray::New();
    events->InsertNextValue(vtkMRMLScene::NewSceneEvent);
    events->InsertNextValue(vtkMRMLScene::SceneCloseEvent);
    events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
    events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
    this->ForegroundLayer->SetAndObserveMRMLSceneEvents ( this->MRMLScene, events );
    events->Delete();

    this->ForegroundLayer->SetSliceNode(SliceNode);
    vtkEventBroker::GetInstance()->AddObservation(
        this->ForegroundLayer, vtkCommand::ModifiedEvent, this, this->LogicCallbackCommand );
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SetLabelLayer(vtkSlicerSliceLayerLogic *labelLayer)
{
  if (this->LabelLayer)
    {
    this->LabelLayer->SetAndObserveMRMLScene( NULL );
    this->LabelLayer->Delete();
    }
  this->LabelLayer = labelLayer;

  if (this->LabelLayer)
    {
    this->LabelLayer->Register(this);

    vtkIntArray *events = vtkIntArray::New();
    events->InsertNextValue(vtkMRMLScene::NewSceneEvent);
    events->InsertNextValue(vtkMRMLScene::SceneCloseEvent);
    events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
    events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
    this->LabelLayer->SetAndObserveMRMLSceneEvents ( this->MRMLScene, events );
    events->Delete();

    this->LabelLayer->SetSliceNode(SliceNode);
    vtkEventBroker::GetInstance()->AddObservation(
        this->LabelLayer, vtkCommand::ModifiedEvent, this, this->LogicCallbackCommand );
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SetForegroundOpacity(double foregroundOpacity)
{
  this->ForegroundOpacity = foregroundOpacity;

  if ( this->Blend->GetOpacity(1) != this->ForegroundOpacity )
    {
    this->Blend->SetOpacity(1, this->ForegroundOpacity);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SetLabelOpacity(double labelOpacity)
{
  this->LabelOpacity = labelOpacity;

  if ( this->Blend->GetOpacity(2) != this->LabelOpacity )
    {
    this->Blend->SetOpacity(2, this->LabelOpacity);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::UpdatePipeline()
{


  int modified = 0;

  if ( this->SliceCompositeNode )
    {
    // get the background and foreground image data from the layers
    // so we can use them as input to the image blend
    // TODO: change logic to use a volume node superclass rather than
    // a scalar volume node once the superclass is sorted out for vector/tensor Volumes

    const char *id;
    
    // Background
    id = this->SliceCompositeNode->GetBackgroundVolumeID();
    vtkMRMLVolumeNode *bgnode = NULL;
    if (id)
      {
      bgnode = vtkMRMLVolumeNode::SafeDownCast (this->MRMLScene->GetNodeByID(id));
      }
    
    if (this->BackgroundLayer)
      {
      if ( this->BackgroundLayer->GetVolumeNode() != bgnode ) 
        {
        this->BackgroundLayer->SetVolumeNode (bgnode);
        modified = 1;
        }
      }
    
    // Foreground
    id = this->SliceCompositeNode->GetForegroundVolumeID();
    vtkMRMLVolumeNode *fgnode = NULL;
    if (id)
      {
      fgnode = vtkMRMLVolumeNode::SafeDownCast (this->MRMLScene->GetNodeByID(id));
      }
    
    if (this->ForegroundLayer)
      {
      if ( this->ForegroundLayer->GetVolumeNode() != fgnode ) 
        {
        this->ForegroundLayer->SetVolumeNode (fgnode);
        modified = 1;
        }
      }
      
    // Label
    id = this->SliceCompositeNode->GetLabelVolumeID();
    vtkMRMLVolumeNode *lbnode = NULL;
    if (id)
      {
      lbnode = vtkMRMLVolumeNode::SafeDownCast (this->MRMLScene->GetNodeByID(id));
      }
    
    if (this->LabelLayer)
      {
      if ( this->LabelLayer->GetVolumeNode() != lbnode ) 
        {
        this->LabelLayer->SetVolumeNode (lbnode);
        modified = 1;
        }
      }

    // update the slice intersection visibility to track the composite node setting
    if ( this->SliceModelNode )
      {
      vtkMRMLModelDisplayNode *modelDisplayNode = this->SliceModelNode->GetModelDisplayNode();
      if ( modelDisplayNode )
        {
        if ( this->SliceCompositeNode != NULL )
          {
          modelDisplayNode->SetSliceIntersectionVisibility( this->SliceCompositeNode->GetSliceIntersectionVisibility() );
          }
        }
      }

    // Now update the image blend with the background and foreground and label
    // -- layer 0 opacity is ignored, but since not all inputs may be non-null, 
    //    we keep track so that someone could, for example, have a NULL background
    //    with a non-null foreground and label and everything will work with the 
    //    label opacity
    //
    // -- first make a temp blend instance and set it up according to the current 
    //    parameters.  Then check if this is the same as the current 'real' blend,
    //    and if not send a modified event
    //

    vtkImageMathematics *tempMath = vtkImageMathematics::New();
    vtkImageCast *tempCast = vtkImageCast::New();
    vtkImageBlend *tempBlend = vtkImageBlend::New();

    int sliceCompositing = this->SliceCompositeNode->GetCompositing();
    // alpha blend or reverse alpha blend
    bool alphaBlending = (sliceCompositing == vtkMRMLSliceCompositeNode::Alpha ||
                          sliceCompositing == vtkMRMLSliceCompositeNode::ReverseAlpha);
    
    if (sliceCompositing == vtkMRMLSliceCompositeNode::Add)
      {
      // add the foreground and background
      tempMath->SetOperationToAdd();
      tempMath->GetOutput()->SetScalarType(VTK_SHORT);
      tempCast->SetOutputScalarTypeToUnsignedChar();
      }
    else if (sliceCompositing == vtkMRMLSliceCompositeNode::Subtract)
      {
      // subtract the foreground and background
      tempMath->SetOperationToSubtract();
      tempMath->GetOutput()->SetScalarType(VTK_SHORT);
      tempCast->SetOutputScalarTypeToUnsignedChar();
      }

    if (!alphaBlending)
      {
      if (!(this->BackgroundLayer && this->BackgroundLayer->GetImageData())
          || !(this->ForegroundLayer && this->ForegroundLayer->GetImageData()))
        {
        // not enough inputs for add/subtract, so use alpha blending
        // pipeline
        alphaBlending = true;
        }
      }

    tempBlend->RemoveAllInputs ( );
    int layerIndex = 0;

    if (!alphaBlending)
      {
      tempMath->SetInput1( this->ForegroundLayer->GetImageData() );
      tempMath->SetInput2( this->BackgroundLayer->GetImageData() );
      tempCast->SetInput( tempMath->GetOutput() );
      
      tempBlend->AddInput( tempCast->GetOutput() );
      tempBlend->SetOpacity( layerIndex++, 1.0 );
      }
    else
      {
      if (sliceCompositing ==  vtkMRMLSliceCompositeNode::Alpha)
        {
        if ( this->BackgroundLayer && this->BackgroundLayer->GetImageData() )
          {
          tempBlend->AddInput( this->BackgroundLayer->GetImageData() );
          tempBlend->SetOpacity( layerIndex++, 1.0 );
          }
        if ( this->ForegroundLayer && this->ForegroundLayer->GetImageData() )
          {
          tempBlend->AddInput( this->ForegroundLayer->GetImageData() );
          tempBlend->SetOpacity( layerIndex++, this->SliceCompositeNode->GetForegroundOpacity() );
          }
        }
      else if (sliceCompositing == vtkMRMLSliceCompositeNode::ReverseAlpha)
        {
        if ( this->ForegroundLayer && this->ForegroundLayer->GetImageData() )
          {
          tempBlend->AddInput( this->ForegroundLayer->GetImageData() );
          tempBlend->SetOpacity( layerIndex++, 1.0 );
          }
        if ( this->BackgroundLayer && this->BackgroundLayer->GetImageData() )
          {
          tempBlend->AddInput( this->BackgroundLayer->GetImageData() );
          tempBlend->SetOpacity( layerIndex++, this->SliceCompositeNode->GetForegroundOpacity() );
          }
        
        }
      }

    // always blending the label layer
    if ( this->LabelLayer && this->LabelLayer->GetImageData() )
      {
      tempBlend->AddInput( this->LabelLayer->GetImageData() );
      tempBlend->SetOpacity( layerIndex++, this->SliceCompositeNode->GetLabelOpacity() );
      }

    if ( tempBlend->GetNumberOfInputs() != this->Blend->GetNumberOfInputs() )
      {
      this->Blend->RemoveAllInputs();
      }
    for (layerIndex = 0; layerIndex < tempBlend->GetNumberOfInputs(); layerIndex++)
      {
      if ( tempBlend->GetInput(layerIndex) != this->Blend->GetInput(layerIndex) )
        {
        this->Blend->SetInput(layerIndex, tempBlend->GetInput(layerIndex));
        modified = 1;
        }
      if ( tempBlend->GetOpacity(layerIndex) != this->Blend->GetOpacity(layerIndex) )
        {
        this->Blend->SetOpacity(layerIndex, tempBlend->GetOpacity(layerIndex));
        modified = 1;
        }
      }

    tempBlend->Delete();
    tempMath->Delete();  // Blend may still be holding a reference
    tempCast->Delete();  // Blend may still be holding a reference

    //Models

    this->UpdateImageData();

    if ( this->SliceModelNode && 
          this->SliceModelNode->GetModelDisplayNode() &&
            this->SliceNode ) 
      {
      if (this->SliceModelNode->GetModelDisplayNode()->GetVisibility() != this->SliceNode->GetSliceVisible() )
        {
        this->SliceModelNode->GetModelDisplayNode()->SetVisibility( this->SliceNode->GetSliceVisible() );
        }


      if (!((this->GetBackgroundLayer() != NULL && this->GetBackgroundLayer()->GetImageData() != NULL) ||
                 (this->GetForegroundLayer() != NULL && this->GetForegroundLayer()->GetImageData() != NULL) ||
                 (this->GetLabelLayer() != NULL && this->GetLabelLayer()->GetImageData() != NULL) )  )
        {
        this->SliceModelNode->GetModelDisplayNode()->SetAndObserveTextureImageData(NULL);
        }
      else if (this->SliceModelNode->GetModelDisplayNode()->GetTextureImageData() != this->ExtractModelTexture->GetOutput())
        {
        // upadte texture
        this->ExtractModelTexture->Update();
        this->SliceModelNode->GetModelDisplayNode()->SetAndObserveTextureImageData(this->ExtractModelTexture->GetOutput());
        }
       
      }

    if ( modified )
      {
      if (this->SliceModelNode && this->SliceModelNode->GetPolyData())
        {
        this->SliceModelNode->GetPolyData()->Modified();
        }
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkSlicerLogic::PrintSelf(os, indent);
  vtkIndent nextIndent;
  nextIndent = indent.GetNextIndent();

  os << indent << "SlicerSliceLogic:             " << this->GetClassName() << "\n";

  if (this->SliceNode)
    {
    os << indent << "SliceNode: ";
    os << (this->SliceNode->GetID() ? this->SliceNode->GetID() : "(null ID)") << "\n";
    this->SliceNode->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "SliceNode: (none)\n";
    }

  if (this->SliceCompositeNode)
    {
    os << indent << "SliceCompositeNode: ";
    os << (this->SliceCompositeNode->GetID() ? this->SliceCompositeNode->GetID() : "(null ID)") << "\n";
    this->SliceCompositeNode->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "SliceCompositeNode: (none)\n";
    }

  if (this->BackgroundLayer)
    {
    os << indent << "BackgroundLayer: ";
    this->BackgroundLayer->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "BackgroundLayer: (none)\n";
    }

  if (this->ForegroundLayer)
    {
    os << indent << "ForegroundLayer: ";
    this->ForegroundLayer->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "ForegroundLayer: (none)\n";
    }

  if (this->LabelLayer)
    {
    os << indent << "LabelLayer: ";
    this->LabelLayer->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "LabelLayer: (none)\n";
    }

  if (this->Blend)
    {
    os << indent << "Blend: ";
    this->Blend->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "Blend: (none)\n";
    }

  os << indent << "ForegroundOpacity: " << this->ForegroundOpacity << "\n";
  os << indent << "LabelOpacity: " << this->LabelOpacity << "\n";

}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::DeleteSliceModel()
{
  // Remove References
  if (this->SliceModelNode != NULL)
    {
    this->SliceModelNode->SetAndObserveDisplayNodeID(NULL);
    this->SliceModelNode->SetAndObserveTransformNodeID(NULL);
    this->SliceModelNode->SetAndObservePolyData(NULL);
    }
  if (this->SliceModelDisplayNode != NULL)
    {
    this->SliceModelDisplayNode->SetAndObserveTextureImageData(NULL);
    }

  // Remove Nodes 
  if (this->SliceModelNode != NULL)
    {
    if (this->MRMLScene)
      {
      this->MRMLScene->RemoveNode(this->SliceModelNode);
      }
    this->SliceModelNode->Delete();
    this->SliceModelNode = NULL;
    }
  if (this->SliceModelDisplayNode != NULL)
    {
    if (this->MRMLScene)
      {
      this->MRMLScene->RemoveNode(this->SliceModelDisplayNode);
      }
    this->SliceModelDisplayNode->Delete();
    this->SliceModelDisplayNode = NULL;
    }
  if (this->SliceModelTransformNode != NULL)
    {
    if (this->MRMLScene)
      {
      this->MRMLScene->RemoveNode(this->SliceModelTransformNode);
      }
    this->SliceModelTransformNode->Delete();
    this->SliceModelTransformNode = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::CreateSliceModel()
{

  if (this->SliceModelNode != NULL && this->MRMLScene->GetNodeByID( this->GetSliceModelNode()->GetID() ) == NULL )
    {
    this->DeleteSliceModel();
    }

  if ( this->SliceModelNode == NULL) 
    {
    this->SliceModelNode = vtkMRMLModelNode::New();
    this->SliceModelNode->SetScene(this->GetMRMLScene());
    this->SliceModelNode->SetDisableModifiedEvent(1);

    this->SliceModelNode->SetHideFromEditors(1);
    this->SliceModelNode->SetSelectable(0);
    this->SliceModelNode->SetSaveWithScene(0);

    // create plane slice
    vtkPlaneSource *planeSource;
    planeSource = vtkPlaneSource::New();
    planeSource->GetOutput()->Update();
    this->SliceModelNode->SetAndObservePolyData(planeSource->GetOutput());
    planeSource->Delete();
    this->SliceModelNode->SetDisableModifiedEvent(0);

    // create display node and set texture
    this->SliceModelDisplayNode = vtkMRMLModelDisplayNode::New();
    this->SliceModelDisplayNode->SetScene(this->GetMRMLScene());
    this->SliceModelDisplayNode->SetDisableModifiedEvent(1);

    this->SliceModelDisplayNode->SetPolyData(this->SliceModelNode->GetPolyData());
    this->SliceModelDisplayNode->SetVisibility(0);
    this->SliceModelDisplayNode->SetOpacity(1);
    this->SliceModelDisplayNode->SetColor(1,1,1);
    // try to auto-set the colors based on the slice name
    // cannot use the vtkSlicerColor class since it is in the GUI
    // TODO: need a better mapping than this
    // 
    if (this->Name != NULL)
      {
      if ( !strcmp(this->Name, "Red") )
        {
        this->SliceModelDisplayNode->SetColor(0.952941176471, 0.290196078431, 0.2);
        }
      if ( !strcmp(this->Name, "Green") )
        {
        this->SliceModelDisplayNode->SetColor(0.43137254902, 0.690196078431, 0.294117647059);
        }
      if ( !strcmp(this->Name, "Yellow") )
        {
        this->SliceModelDisplayNode->SetColor(0.929411764706, 0.835294117647, 0.298039215686);
        }
      }
    this->SliceModelDisplayNode->SetAmbient(1);
    this->SliceModelDisplayNode->SetBackfaceCulling(0);
    this->SliceModelDisplayNode->SetDiffuse(0);
    this->SliceModelDisplayNode->SetAndObserveTextureImageData(this->ExtractModelTexture->GetOutput());
    this->SliceModelDisplayNode->SetSaveWithScene(0);
    this->SliceModelDisplayNode->SetDisableModifiedEvent(0);

    // Turn slice intersection off by default - there is a higher level GUI control
    // in the SliceCompositeNode that tells if slices should be enabled for a given
    // slice viewer
    this->SliceModelDisplayNode->SetSliceIntersectionVisibility(0);

    std::string name = std::string(this->Name) + " Volume Slice";
    this->SliceModelNode->SetName (name.c_str());

    // make the xy to RAS transform
    this->SliceModelTransformNode = vtkMRMLLinearTransformNode::New();
    this->SliceModelTransformNode->SetScene(this->GetMRMLScene());
    this->SliceModelTransformNode->SetDisableModifiedEvent(1);

    this->SliceModelTransformNode->SetHideFromEditors(1);
    this->SliceModelTransformNode->SetSelectable(0);
    this->SliceModelTransformNode->SetSaveWithScene(0);
    // set the transform for the slice model for use by an image actor in the viewer
    this->SliceModelTransformNode->GetMatrixTransformToParent()->Identity();

    this->SliceModelTransformNode->SetDisableModifiedEvent(0);

    }

  if (this->SliceModelNode != NULL && this->MRMLScene->GetNodeByID( this->GetSliceModelNode()->GetID() ) == NULL )
    {
    this->MRMLScene->AddNodeNoNotify(this->SliceModelDisplayNode);
    this->MRMLScene->AddNodeNoNotify(this->SliceModelTransformNode);
    this->MRMLScene->AddNode(this->SliceModelNode);
    this->SliceModelNode->SetAndObserveDisplayNodeID(this->SliceModelDisplayNode->GetID());
    this->SliceModelDisplayNode->SetAndObserveTextureImageData(this->ExtractModelTexture->GetOutput());
    this->SliceModelNode->SetAndObserveTransformNodeID(this->SliceModelTransformNode->GetID());
    }

  // update the description to refer back to the slice and composite nodes
  // TODO: this doesn't need to be done unless the ID change, but it needs
  // to happen after they have been set, so do it every event for now
  if ( this->SliceModelNode != NULL ) 
    {
    char description[256];
    std::stringstream ssD;
    vtkMRMLSliceNode *sliceNode = this->GetSliceNode();
    if ( sliceNode && sliceNode->GetID() )
      {
      ssD << " SliceID " << sliceNode->GetID();
      }
    vtkMRMLSliceCompositeNode *compositeNode = this->GetSliceCompositeNode();
    if ( compositeNode && compositeNode->GetID() )
      {
      ssD << " CompositeID " << compositeNode->GetID();
      }

    ssD.getline(description,256);
    this->SliceModelNode->SetDescription(description);
    }
}


vtkMRMLVolumeNode *vtkSlicerSliceLogic::GetLayerVolumeNode(int layer)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();
  vtkMRMLSliceCompositeNode *compositeNode = this->GetSliceCompositeNode();

  if ( !sliceNode || !compositeNode )
    {
    return (NULL);
    }
  
  char *id = NULL;
  switch (layer)
    {
    case 0:
      {
      id = compositeNode->GetBackgroundVolumeID();
      break;
      }
    case 1:
      {
      id = compositeNode->GetForegroundVolumeID();
      break;
      }
    case 2:
      {
      id = compositeNode->GetLabelVolumeID();
      break;
      }
    }
  return ( vtkMRMLVolumeNode::SafeDownCast ( this->MRMLScene->GetNodeByID( id )) );
}

// Get the size of the volume, transformed to RAS space
void vtkSlicerSliceLogic::GetVolumeRASBox(vtkMRMLVolumeNode *volumeNode, double rasDimensions[3], double rasCenter[3])
{
  rasCenter[0] = rasDimensions[0] = 0.0;
  rasCenter[1] = rasDimensions[1] = 0.0;
  rasCenter[2] = rasDimensions[2] = 0.0;


  vtkImageData *volumeImage;
  if ( !volumeNode || ! (volumeImage = volumeNode->GetImageData()) )
    {
    return;
    }

  //
  // Get the size of the volume in RAS space
  // - map the size of the volume in IJK into RAS
  // - map the middle of the volume to RAS for the center
  //   (IJK space always has origin at first pixel)
  //
  vtkSmartPointer<vtkMatrix4x4> ijkToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASMatrix (ijkToRAS);
  vtkMRMLTransformNode *transformNode = volumeNode->GetParentTransformNode();
  if ( transformNode )
    {
    vtkSmartPointer<vtkMatrix4x4> rasToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
    transformNode->GetMatrixTransformToWorld(rasToRAS);
    vtkMatrix4x4::Multiply4x4 (rasToRAS, ijkToRAS, ijkToRAS);
    }

  int dimensions[3];
  int i,j,k;
  volumeImage->GetDimensions(dimensions);
  double doubleDimensions[4], rasHDimensions[4];
  double minBounds[3], maxBounds[3];

  for ( i=0; i<3; i++) 
    {
    minBounds[i] = 1.0e10;
    maxBounds[i] = -1.0e10;
    }
  for ( i=0; i<2; i++) 
    {
    for ( j=0; j<2; j++) 
      {
      for ( k=0; k<2; k++) 
        {
        doubleDimensions[0] = i*(dimensions[0] - 1);
        doubleDimensions[1] = j*(dimensions[1] - 1);
        doubleDimensions[2] = k*(dimensions[2] - 1);
        doubleDimensions[3] = 1;
        ijkToRAS->MultiplyPoint( doubleDimensions, rasHDimensions );
        for (int n=0; n<3; n++) {
          if (rasHDimensions[n] < minBounds[n])
            {
            minBounds[n] = rasHDimensions[n];
            }
          if (rasHDimensions[n] > maxBounds[n])
            {
            maxBounds[n] = rasHDimensions[n];
            }
          }
        }
      }
    }
  
   for ( i=0; i<3; i++) 
    {
    rasDimensions[i] = maxBounds[i] - minBounds[i];
    rasCenter[i] = 0.5*(maxBounds[i] + minBounds[i]);
    }
}

// Get the size of the volume, transformed to RAS space
void vtkSlicerSliceLogic::GetVolumeSliceDimensions(vtkMRMLVolumeNode *volumeNode, double sliceDimensions[3], double sliceCenter[3])
{
  sliceCenter[0] = sliceDimensions[0] = 0.0;
  sliceCenter[1] = sliceDimensions[1] = 0.0;
  sliceCenter[2] = sliceDimensions[2] = 0.0;

  double sliceBounds[6];

  this->GetVolumeSliceBounds(volumeNode, sliceBounds);

  for (int i=0; i<3; i++) 
    {
    sliceDimensions[i] = sliceBounds[2*i+1] - sliceBounds[2*i];
    sliceCenter[i] = 0.5*(sliceBounds[2*i+1] + sliceBounds[2*i]);
    }
}

void vtkSlicerSliceLogic::GetVolumeSliceBounds(vtkMRMLVolumeNode *volumeNode, double sliceBounds[6])
{
  sliceBounds[0] = sliceBounds[1] = 0.0;
  sliceBounds[2] = sliceBounds[3] = 0.0;
  sliceBounds[4] = sliceBounds[5] = 0.0;

  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  if ( !sliceNode )
    {
    return;
    }
  
  double rasDimensions[3], rasCenter[3];

  this->GetVolumeRASBox(volumeNode, rasDimensions, rasCenter);

  //
  // figure out how big that volume is on this particular slice plane
  //
  vtkSmartPointer<vtkMatrix4x4> rasToSlice = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToSlice->DeepCopy(sliceNode->GetSliceToRAS());
  rasToSlice->SetElement(0, 3, 0.0);
  rasToSlice->SetElement(1, 3, 0.0);
  rasToSlice->SetElement(2, 3, 0.0);
  rasToSlice->Invert();

  double minBounds[3], maxBounds[3];
  double rasCorner[4], sliceCorner[4];
  int i,j,k;

  for ( i=0; i<3; i++) 
    {
    minBounds[i] = 1.0e10;
    maxBounds[i] = -1.0e10;
    }
  for ( i=-1; i<=1; i=i+2) 
    {
    for ( j=-1; j<=1; j=j+2) 
      {
      for ( k=-1; k<=1; k=k+2) 
        {
        rasCorner[0] = rasCenter[0] + i * rasDimensions[0] / 2.;
        rasCorner[1] = rasCenter[1] + j * rasDimensions[1] / 2.;
        rasCorner[2] = rasCenter[2] + k * rasDimensions[2] / 2.;
        rasCorner[3] = 1.;

        rasToSlice->MultiplyPoint( rasCorner, sliceCorner );

        for (int n=0; n<3; n++) {
          if (sliceCorner[n] < minBounds[n])
            {
            minBounds[n] = sliceCorner[n];
            }
          if (sliceCorner[n] > maxBounds[n])
            {
            maxBounds[n] = sliceCorner[n];
            }
          }
        }
      }
    }

  // ignore homogeneous coordinate
  sliceBounds[0] = minBounds[0];
  sliceBounds[1] = maxBounds[0];
  sliceBounds[2] = minBounds[1];
  sliceBounds[3] = maxBounds[1];
  sliceBounds[4] = minBounds[2];
  sliceBounds[5] = maxBounds[2];
}

// Get the spacing of the volume, transformed to slice space
double *vtkSlicerSliceLogic::GetVolumeSliceSpacing(vtkMRMLVolumeNode *volumeNode)
{
  if ( !volumeNode )
    {
    return (this->SliceSpacing);
    }

  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  if ( !sliceNode )
    {
    return (this->SliceSpacing);
    }

  if (sliceNode->GetSliceSpacingMode() == vtkMRMLSliceNode::PrescribedSliceSpacingMode)
    {
    // jvm - should we cache the PrescribedSliceSpacing in SliceSpacing?
    double *pspacing = sliceNode->GetPrescribedSliceSpacing();
    this->SliceSpacing[0] = pspacing[0];
    this->SliceSpacing[1] = pspacing[1];
    this->SliceSpacing[2] = pspacing[2];
    return (pspacing);
    }
  
  vtkSmartPointer<vtkMatrix4x4> ijkToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkMatrix4x4> rasToSlice = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkMatrix4x4> ijkToSlice = vtkSmartPointer<vtkMatrix4x4>::New();

  volumeNode->GetIJKToRASMatrix(ijkToRAS);

  // Apply the transform, if it exists
  vtkMRMLTransformNode *transformNode = volumeNode->GetParentTransformNode();
  if ( transformNode != NULL ) 
    {
    if ( transformNode->IsTransformToWorldLinear() )
      {
      vtkSmartPointer<vtkMatrix4x4> rasToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
      transformNode->GetMatrixTransformToWorld( rasToRAS );
      rasToRAS->Invert();
      vtkMatrix4x4::Multiply4x4(rasToRAS, ijkToRAS, ijkToRAS); 
      }
    }

  rasToSlice->DeepCopy(sliceNode->GetSliceToRAS());
  rasToSlice->Invert();

  ijkToSlice->Multiply4x4(rasToSlice, ijkToRAS, ijkToSlice);

  double invector[4];
  invector[0] = invector[1] = invector[2] = 1.0;
  invector[3] = 0.0;
  double spacing[4];
  ijkToSlice->MultiplyPoint(invector, spacing);
  int i;
  for (i = 0; i < 3; i++)
    {
    this->SliceSpacing[i] = fabs(spacing[i]);
    }

  return (this->SliceSpacing);
}

// adjust the node's field of view to match the extent of current volume
void vtkSlicerSliceLogic::FitSliceToVolume(vtkMRMLVolumeNode *volumeNode, int width, int height)
{
  vtkImageData *volumeImage;
  if ( !volumeNode || ! (volumeImage = volumeNode->GetImageData()) )
    {
    return;
    }

  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  if ( !sliceNode )
    {
    return;
    }

  double rasDimensions[3], rasCenter[3];
  this->GetVolumeRASBox (volumeNode, rasDimensions, rasCenter);
  double sliceDimensions[3], sliceCenter[3];
  this->GetVolumeSliceDimensions (volumeNode, sliceDimensions, sliceCenter);

  double fitX, fitY, fitZ, displayX, displayY;
  displayX = fitX = fabs(sliceDimensions[0]);
  displayY = fitY = fabs(sliceDimensions[1]);
  fitZ = this->GetVolumeSliceSpacing(volumeNode)[2] * sliceNode->GetDimensions()[2];


  // fit fov to min dimension of window
  double pixelSize;
  if ( height > width )
    {
    pixelSize = fitX / (1.0 * width);
    fitY = pixelSize * height;
    }
  else
    {
    pixelSize = fitY / (1.0 * height);
    fitX = pixelSize * width;
    }

  // if volume is still too big, shrink some more
  if ( displayX > fitX )
    {
    fitY = fitY / ( fitX / (displayX * 1.0) );
    fitX = displayX;
    }
  if ( displayY > fitY )
    {
    fitX = fitX / ( fitY / (displayY * 1.0) );
    fitY = displayY;
    }

  sliceNode->SetFieldOfView(fitX, fitY, fitZ);

  //
  // set the origin to be the center of the volume in RAS
  //
  vtkSmartPointer<vtkMatrix4x4> sliceToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  sliceToRAS->DeepCopy(sliceNode->GetSliceToRAS());
  sliceToRAS->SetElement(0, 3, rasCenter[0]);
  sliceToRAS->SetElement(1, 3, rasCenter[1]);
  sliceToRAS->SetElement(2, 3, rasCenter[2]);
  sliceNode->GetSliceToRAS()->DeepCopy(sliceToRAS);
  sliceNode->UpdateMatrices( );
}


// Get the size of the volume, transformed to RAS space
void vtkSlicerSliceLogic::GetBackgroundRASBox(double rasDimensions[3], double rasCenter[3])
{
  vtkMRMLVolumeNode *backgroundNode = NULL;
  backgroundNode = this->GetLayerVolumeNode (0);
  this->GetVolumeRASBox( backgroundNode, rasDimensions, rasCenter );
}

// Get the size of the volume, transformed to RAS space
void vtkSlicerSliceLogic::GetBackgroundSliceDimensions(double sliceDimensions[3], double sliceCenter[3])
{
  vtkMRMLVolumeNode *backgroundNode = NULL;
  backgroundNode = this->GetLayerVolumeNode (0);
  this->GetVolumeSliceDimensions( backgroundNode, sliceDimensions, sliceCenter );
}

// Get the spacing of the volume, transformed to slice space
double *vtkSlicerSliceLogic::GetBackgroundSliceSpacing()
{
  vtkMRMLVolumeNode *backgroundNode = NULL;
  backgroundNode = this->GetLayerVolumeNode (0);
  return (this->GetVolumeSliceSpacing( backgroundNode ));
}

void vtkSlicerSliceLogic::GetBackgroundSliceBounds(double sliceBounds[6])
{
  vtkMRMLVolumeNode *backgroundNode = NULL;
  backgroundNode = this->GetLayerVolumeNode (0);
  this->GetVolumeSliceBounds(backgroundNode, sliceBounds);
}

// adjust the node's field of view to match the extent of current background volume
void vtkSlicerSliceLogic::FitSliceToBackground(int width, int height)
{
  vtkMRMLVolumeNode *backgroundNode = NULL;
  backgroundNode = this->GetLayerVolumeNode (0);
  this->FitSliceToVolume( backgroundNode, width, height );
}

// adjust the node's field of view to match the extent of all volume layers
void vtkSlicerSliceLogic::FitSliceToAll(int width, int height)
{
  vtkMRMLVolumeNode *volumeNode;
  for ( int layer=0; layer < 3; layer++ )
    {
    volumeNode = this->GetLayerVolumeNode (layer);
    if (volumeNode)
      {
      this->FitSliceToVolume( volumeNode, width, height );
      return;
      }
    }
}

vtkMRMLVolumeNode *vtkSlicerSliceLogic::GetLowestVolumeNode()
{
  vtkMRMLVolumeNode *volumeNode;
  for ( int layer=0; layer < 3; layer++ )
    {
    volumeNode = this->GetLayerVolumeNode (layer);
    if (volumeNode)
      {
      return volumeNode;
      }
    }
  return NULL;
}

double *vtkSlicerSliceLogic::GetLowestVolumeSliceSpacing()
{
  vtkMRMLVolumeNode *volumeNode;
  for ( int layer=0; layer < 3; layer++ )
    {
    volumeNode = this->GetLayerVolumeNode (layer);
    if (volumeNode)
      {
      return this->GetVolumeSliceSpacing( volumeNode );
      }
    }
  return (this->SliceSpacing);
}

void vtkSlicerSliceLogic::GetLowestVolumeSliceBounds(double sliceBounds[6])
{
  vtkMRMLVolumeNode *volumeNode;
  for ( int layer=0; layer < 3; layer++ )
    {
    volumeNode = this->GetLayerVolumeNode (layer);
    if (volumeNode)
      {
      return this->GetVolumeSliceBounds( volumeNode, sliceBounds );
      }
    }
  // return the default values
  return this->GetVolumeSliceBounds( NULL, sliceBounds );
}

// Get/Set the current distance from the origin to the slice plane

double vtkSlicerSliceLogic::GetSliceOffset()
{
  //
  // - get the current translation in RAS space and convert it to Slice space
  //   by transforming it by the inverse of the upper 3x3 of SliceToRAS
  // - pull out the Z translation part
  //

  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  if ( !sliceNode )
    {
    return 0.0;
    }

  vtkSmartPointer<vtkMatrix4x4> sliceToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  sliceToRAS->DeepCopy( this->SliceNode->GetSliceToRAS() );
  for (int i = 0; i < 3; i++)
    {
    sliceToRAS->SetElement( i, 3, 0.0 );  // Zero out the tranlation portion
    }
  sliceToRAS->Invert();
  double v1[4], v2[4];
  for (int i = 0; i < 4; i++)
    { // get the translation back as a vector
    v1[i] = sliceNode->GetSliceToRAS()->GetElement( i, 3 );
    }
  // bring the translation into slice space
  // and overwrite the z part
  sliceToRAS->MultiplyPoint(v1, v2);

  return ( v2[2] );

}


void vtkSlicerSliceLogic::CalculateSliceOffset(double offset, double *oldOffsetVector, double *newOffsetVector)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  if ( !sliceNode )
    {
    return;
    }

  vtkSmartPointer<vtkMatrix4x4> sliceToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  sliceToRAS->DeepCopy( this->SliceNode->GetSliceToRAS() );
  for (int i = 0; i < 3; i++)
    {
    sliceToRAS->SetElement( i, 3, 0.0 );  // Zero out the tranlation portion
    }
  vtkSmartPointer<vtkMatrix4x4> sliceToRASInverted = vtkSmartPointer<vtkMatrix4x4>::New(); // inverse sliceToRAS
  sliceToRASInverted->DeepCopy( sliceToRAS );
  sliceToRASInverted->Invert();
  double tempVector[4];
  for (int i = 0; i < 4; i++)
    { // get the translation back as a vector
    oldOffsetVector[i] = sliceNode->GetSliceToRAS()->GetElement( i, 3 );
    }
  // bring the translation into slice space
  // and overwrite the z part
  sliceToRASInverted->MultiplyPoint(oldOffsetVector, tempVector);

  tempVector[2] = offset;

  // Now bring the new translation vector back into RAS space
  sliceToRAS->MultiplyPoint(tempVector, newOffsetVector);
}

void vtkSlicerSliceLogic::SetSliceOffset(double offset)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  if ( !sliceNode )
    {
    return;
    }

  //
  // Set the Offset
  // - get the current translation in RAS space and convert it to Slice space
  //   by transforming it by the invers of the upper 3x3 of SliceToRAS
  // - replace the z value of the translation with the new value given by the slider
  // - this preserves whatever translation was already in place
  //

  double oldOffset = this->GetSliceOffset();
  if (fabs(offset - oldOffset) <= 1.0e-6)
    {
    return;
    }

  double oldOffsetVector[4];
  double newOffsetVector[4];

  this->CalculateSliceOffset(offset, oldOffsetVector, newOffsetVector);
 
  // if the translation has changed, update the rest of the matrices
  double eps=1.0e-6;
  if ( fabs(oldOffsetVector[0] - newOffsetVector[0]) > eps ||
       fabs(oldOffsetVector[1] - newOffsetVector[1]) > eps ||
       fabs(oldOffsetVector[2] - newOffsetVector[2]) > eps )
    {
    vtkSmartPointer<vtkMatrix4x4> sliceToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
    sliceToRAS->DeepCopy( this->SliceNode->GetSliceToRAS() );

    // copy new translation into sliceToRAS
    for (int i = 0; i < 4; i++)
      {
      sliceToRAS->SetElement( i, 3, newOffsetVector[i] );
      }
    sliceNode->GetSliceToRAS()->DeepCopy( sliceToRAS );
    sliceNode->UpdateMatrices();
    }
}

//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::SnapSliceOffsetToIJK()
{
  /* old way
  double offset, *spacing, bounds[6];
  double oldOffset = this->GetSliceOffset();
  spacing = this->GetLowestVolumeSliceSpacing();
  this->GetLowestVolumeSliceBounds( bounds );
  
  // number of slices along the offset dimension (depends on ijkToRAS and Transforms)
  double slice = (oldOffset - bounds[4]) / spacing[2];
  int intSlice = static_cast<int> (0.5 + slice);  
  offset = intSlice * spacing[2] + bounds[4];
  this->SetSliceOffset( offset );
  */

  vtkMRMLVolumeNode *volumeNode = this->GetLowestVolumeNode();
  if (volumeNode == NULL)
    {
    return;
    }

  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();
  if ( !sliceNode )
    {
    return;
    }

  double oldOffsetVector[4];
  double newOffsetVector[4];

  double oldOffset = this->GetSliceOffset();
  this->CalculateSliceOffset(oldOffset + 1, oldOffsetVector, newOffsetVector);

  // get the transform from RAS to IJK 
  vtkSmartPointer<vtkMatrix4x4> ijkToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASMatrix (ijkToRAS);
  vtkMRMLTransformNode *transformNode = volumeNode->GetParentTransformNode();
  if ( transformNode )
    {
    vtkSmartPointer<vtkMatrix4x4> rasToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
    transformNode->GetMatrixTransformToWorld(rasToRAS);
    vtkMatrix4x4::Multiply4x4 (rasToRAS, ijkToRAS, ijkToRAS);
    }

  vtkSmartPointer<vtkMatrix4x4> rasToIJK = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToIJK->DeepCopy(ijkToRAS);
  rasToIJK->Invert();
  double ijk[4];
  rasToIJK->MultiplyPoint(oldOffsetVector, ijk);
  for (int i = 0; i < 3; i++)
    {
    ijk[i] = floor( 0.5+ijk[i] );
    }
  ijkToRAS->MultiplyPoint(ijk, newOffsetVector);

  // copy new translation into sliceToRAS
  vtkSmartPointer<vtkMatrix4x4> sliceToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  sliceToRAS->DeepCopy( this->SliceNode->GetSliceToRAS() );
  for (int i = 0; i < 4; i++)
    {
    sliceToRAS->SetElement( i, 3, newOffsetVector[i] );
    }
  sliceNode->GetSliceToRAS()->DeepCopy( sliceToRAS );
  sliceNode->UpdateMatrices();
}



//----------------------------------------------------------------------------
void vtkSlicerSliceLogic::GetPolyDataAndLookUpTableCollections(vtkPolyDataCollection *polyDataCollection,
                                                               vtkCollection *lookupTableCollection)
{
  this->PolyDataCollection->RemoveAllItems();
  this->LookupTableCollection->RemoveAllItems();
  
  // Add glyphs. Get them from Background or Foreground slice layers.
  vtkSlicerSliceLayerLogic *layerLogic = this->GetBackgroundLayer();
  this->AddSliceGlyphs(layerLogic);
  
  layerLogic = this->GetForegroundLayer();
  this->AddSliceGlyphs(layerLogic);
  
  polyDataCollection = this->PolyDataCollection;
  lookupTableCollection = this->LookupTableCollection;
} 

void vtkSlicerSliceLogic::AddSliceGlyphs(vtkSlicerSliceLayerLogic *layerLogic)
{
 if (layerLogic && layerLogic->GetVolumeNode()) 
    {
    vtkMRMLVolumeNode *volumeNode = vtkMRMLVolumeNode::SafeDownCast (layerLogic->GetVolumeNode());
    vtkMRMLGlyphableVolumeDisplayNode *displayNode = vtkMRMLGlyphableVolumeDisplayNode::SafeDownCast( layerLogic->GetVolumeNode()->GetDisplayNode() );
    if (displayNode)
      {
      std::vector< vtkMRMLGlyphableVolumeSliceDisplayNode*> dnodes  = displayNode->GetSliceGlyphDisplayNodes(volumeNode);
      for (unsigned int i=0; i<dnodes.size(); i++)
        {
        vtkMRMLGlyphableVolumeSliceDisplayNode* dnode = dnodes[i];
        if (dnode->GetVisibility() && layerLogic->GetSliceNode() 
          && layerLogic->GetSliceNode()->GetLayoutName() 
          &&!strcmp(layerLogic->GetSliceNode()->GetLayoutName(), dnode->GetName()) )
          {
          vtkPolyData* poly = dnode->GetPolyDataTransformedToSlice();
          if (poly)
            {
            this->PolyDataCollection->AddItem(poly);
            if (dnode->GetColorNode())
              {
              if (dnode->GetColorNode()->GetLookupTable()) 
                {
                this->LookupTableCollection->AddItem(dnode->GetColorNode()->GetLookupTable());
                }
              else if (vtkMRMLProceduralColorNode::SafeDownCast(dnode->GetColorNode())->GetColorTransferFunction())
                {
                this->LookupTableCollection->AddItem((vtkScalarsToColors*)(vtkMRMLProceduralColorNode::SafeDownCast(dnode->GetColorNode())->GetColorTransferFunction()));
                }
              }
            }
            break;
          }
        }
      }//  if (volumeNode)
    }// if (layerLogic && layerLogic->GetVolumeNode()) 
    
}

std::vector< vtkMRMLDisplayNode*> vtkSlicerSliceLogic::GetPolyDataDisplayNodes()
{
  std::vector< vtkMRMLDisplayNode*> nodes;
  std::vector<vtkSlicerSliceLayerLogic *> layerLogics;
  layerLogics.push_back(this->GetBackgroundLayer());
  layerLogics.push_back(this->GetForegroundLayer());
  for (unsigned int i=0; i<layerLogics.size(); i++) 
    {
    vtkSlicerSliceLayerLogic *layerLogic = layerLogics[i];
    if (layerLogic && layerLogic->GetVolumeNode()) 
      {
      vtkMRMLVolumeNode *volumeNode = vtkMRMLVolumeNode::SafeDownCast (layerLogic->GetVolumeNode());
      vtkMRMLGlyphableVolumeDisplayNode *displayNode = vtkMRMLGlyphableVolumeDisplayNode::SafeDownCast( layerLogic->GetVolumeNode()->GetDisplayNode() );
      if (displayNode)
        {
        std::vector< vtkMRMLGlyphableVolumeSliceDisplayNode*> dnodes  = displayNode->GetSliceGlyphDisplayNodes(volumeNode);
        for (unsigned int n=0; n<dnodes.size(); n++)
          {
          vtkMRMLGlyphableVolumeSliceDisplayNode* dnode = dnodes[n];
          if (layerLogic->GetSliceNode() 
            && layerLogic->GetSliceNode()->GetLayoutName()
            && !strcmp(layerLogic->GetSliceNode()->GetLayoutName(), dnode->GetName()) )
            {
            nodes.push_back(dnode);
            }
          }
        }//  if (volumeNode)
      }// if (layerLogic && layerLogic->GetVolumeNode()) 
    }
  return nodes;
}

int vtkSlicerSliceLogic::GetSliceIndexFromOffset(double sliceOffset, vtkMRMLVolumeNode *volumeNode)
{
  if ( !volumeNode )
    {
    return SLICE_INDEX_NO_VOLUME;
    }
  vtkImageData *volumeImage=NULL;
  if ( !(volumeImage = volumeNode->GetImageData()) )
    {
    return SLICE_INDEX_NO_VOLUME;
    }
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();
  if ( !sliceNode )
    {
    return SLICE_INDEX_NO_VOLUME;
    }

  vtkSmartPointer<vtkMatrix4x4> ijkToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASMatrix (ijkToRAS);
  vtkMRMLTransformNode *transformNode = volumeNode->GetParentTransformNode();
  if ( transformNode )
    {
    vtkSmartPointer<vtkMatrix4x4> rasToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
    transformNode->GetMatrixTransformToWorld(rasToRAS);
    vtkMatrix4x4::Multiply4x4 (rasToRAS, ijkToRAS, ijkToRAS);
    }

  // Get the slice normal in RAS

  vtkSmartPointer<vtkMatrix4x4> rasToSlice = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToSlice->DeepCopy(sliceNode->GetSliceToRAS());
  rasToSlice->Invert();

  double sliceNormal_IJK[4]={0,0,1,0};  // slice normal vector in IJK coordinate system
  double sliceNormal_RAS[4]={0,0,0,0};  // slice normal vector in RAS coordinate system
  sliceNode->GetSliceToRAS()->MultiplyPoint(sliceNormal_IJK, sliceNormal_RAS);
  
  // Find an axis normal that has the same orientation as the slice normal
  double axisDirection_RAS[3]={0,0,0};  
  int axisIndex=0;  
  double volumeSpacing=1.0; // spacing along axisIndex
  for (axisIndex=0; axisIndex<3; axisIndex++)
    {
    axisDirection_RAS[0]=ijkToRAS->GetElement(0,axisIndex);
    axisDirection_RAS[1]=ijkToRAS->GetElement(1,axisIndex);
    axisDirection_RAS[2]=ijkToRAS->GetElement(2,axisIndex);
    volumeSpacing=vtkMath::Norm(axisDirection_RAS); // spacing along axisIndex
    vtkMath::Normalize(sliceNormal_RAS);
    vtkMath::Normalize(axisDirection_RAS);
    double dotProd=vtkMath::Dot(sliceNormal_RAS, axisDirection_RAS);
    // Due to numerical inaccuracies the dot product of two normalized vectors
    // can be slightly bigger than 1 (and acos cannot be computed) - fix that.
    if (dotProd>1.0)
      {
      dotProd=1.0;
      }
    else if (dotProd<-1.0)
      {
      dotProd=-1.0;
      }
    double axisMisalignmentDegrees=acos(dotProd)*180.0/vtkMath::Pi();
    if (fabs(axisMisalignmentDegrees)<0.1)
      {
      // found an axis that is aligned to the slice normal
      break;
      }
    if (fabs(axisMisalignmentDegrees-180)<0.1 || fabs(axisMisalignmentDegrees+180)<0.1)
      {
      // found an axis that is aligned to the slice normal, just points to the opposite direction
      volumeSpacing*=-1.0;
      break;
      }
    }

  if (axisIndex>=3)
    {
    // no aligned axis is found
    return SLICE_INDEX_ROTATED;
    }
  
  // Determine slice index
  double originPos_RAS[4]={
    ijkToRAS->GetElement( 0, 3 ),
    ijkToRAS->GetElement( 1, 3 ),
    ijkToRAS->GetElement( 2, 3 ),
    0};
  double originPos_Slice[4]={0,0,0,0};
  rasToSlice->MultiplyPoint(originPos_RAS, originPos_Slice);
  double volumeOriginOffset=originPos_Slice[2];
  double sliceShift=sliceOffset-volumeOriginOffset;
  double normalizedSliceShift=sliceShift/volumeSpacing;
  int sliceIndex=vtkMath::Round(normalizedSliceShift)+1; // +0.5 because the slice plane is displayed in the center of the slice
  
  // Check if slice index is within the volume
  int sliceCount=volumeImage->GetDimensions()[axisIndex];
  if (sliceIndex<1 || sliceIndex>sliceCount)
    {
    sliceIndex=SLICE_INDEX_OUT_OF_VOLUME;
    }

  return sliceIndex;
}

// sliceIndex: DICOM slice index, 1-based
int vtkSlicerSliceLogic::GetSliceIndexFromOffset(double sliceOffset)
{
  vtkMRMLVolumeNode *volumeNode;
  for (int layer=0; layer < 3; layer++ )
    {
    volumeNode = this->GetLayerVolumeNode (layer);
    if (volumeNode)
      {
      int sliceIndex=this->GetSliceIndexFromOffset( sliceOffset, volumeNode );
      // return the result for the first available layer
      return sliceIndex;
      }
    }
  // slice is not aligned to any of the layers or out of the volume
  return SLICE_INDEX_NO_VOLUME;
}
