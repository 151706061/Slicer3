#include "qSlicerTractographyFiducialSeedingModuleWidget.h"
#include "ui_qSlicerTractographyFiducialSeedingModule.h"

#include "vtkSlicerTractographyFiducialSeedingLogic.h"
#include "vtkMRMLFiberBundleNode.h"
#include "vtkMRMLFiducialListNode.h"
#include "vtkMRMLTractographyFiducialSeedingNode.h"


//-----------------------------------------------------------------------------
class qSlicerTractographyFiducialSeedingModuleWidgetPrivate: 
  public qCTKPrivate<qSlicerTractographyFiducialSeedingModuleWidget>,
  public Ui_qSlicerTractographyFiducialSeedingModule
{
};

//-----------------------------------------------------------------------------
qSlicerTractographyFiducialSeedingModuleWidget::qSlicerTractographyFiducialSeedingModuleWidget(QWidget *parent)
{
  this->TractographyFiducialSeedingNode = NULL;
  this->TransformableNode = NULL;
  this->DiffusionTensorVolumeNode = NULL;

}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  qSlicerWidget::setMRMLScene(scene);

  // find parameters node or create it if there is no one in the scene
  if (this->TractographyFiducialSeedingNode == NULL)
  {
    vtkMRMLTractographyFiducialSeedingNode *tnode = NULL;
    vtkMRMLNode *node = scene->GetNthNodeByClass(0, "vtkMRMLTractographyFiducialSeedingNode");
    if (node == NULL)
    {
      tnode = vtkMRMLTractographyFiducialSeedingNode::New();
      scene->AddNode(tnode);
      tnode->Delete();
    }
    else {
      tnode = vtkMRMLTractographyFiducialSeedingNode::SafeDownCast(node);
    }
    this->setTractographyFiducialSeedingNode(tnode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setup()
{
  QCTK_D(qSlicerTractographyFiducialSeedingModuleWidget);
  d->setupUi(this);


  QObject::connect(d->ParameterNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, 
                                             SLOT(setTractographyFiducialSeedingNode(vtkMRMLNode*)));

  QObject::connect(d->DTINodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, 
                                       SLOT(setDiffusionTensorVolumeNode(vtkMRMLNode*)));

  QObject::connect(d->FiducialNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, 
                                            SLOT(setTransformableNode(vtkMRMLNode*)));

  QObject::connect(d->FiberNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, 
                                         SLOT(setFiberBundleNode(vtkMRMLNode*)));

  //QObject::connect(d->StoppingCurvatureSpinBoxLabel,
  //                 SIGNAL(sliderMoved(double)),
  //                 SLOT(onParameterChanged(double)));

  QObject::connect(d->StoppingCurvatureSpinBoxLabel, 
                SIGNAL(valueChanged(double)),
                SLOT(setStoppingCurvature(double)));

  QObject::connect(d->StoppingCriteriaComboBox, 
                SIGNAL(currentIndexChanged(int)),
                SLOT(setStoppingCriteria(int)));

  QObject::connect(d->StoppingValueSpinBoxLabel, 
                SIGNAL(valueChanged(double)),
                SLOT(setStoppingValue(double)));

  QObject::connect(d->IntegrationStepSpinBoxLabel, 
                SIGNAL(valueChanged(double)),
                SLOT(setIntegrationStep(double)));

  QObject::connect(d->MinimumPathSpinBoxLabel, 
                SIGNAL(valueChanged(double)),
                SLOT(setMinimumPath(double)));

  QObject::connect(d->FiducialRegionSpinBoxLabel, 
                SIGNAL(valueChanged(double)),
                SLOT(setFiducialRegion(double)));

  QObject::connect(d->FiducialStepSpinBoxLabel, 
                SIGNAL(valueChanged(double)),
                SLOT(setFiducialRegionStep(double)));

  QObject::connect(d->DisplayTracksComboBox, 
                SIGNAL(currentIndexChanged(int)),
                SLOT(setTrackDisplayMode(int)));

  QObject::connect(d->SeedSelectedCheckBox, 
                SIGNAL(stateChanged(int)),
                SLOT(setSeedSelectedFiducials(int)));

  QObject::connect(d->EnableSeedingCheckBox, 
                SIGNAL(stateChanged(int)),
                SLOT(setEnableSeeding(int)));

  QObject::connect(d->MaxNumberSeedsNumericInput, 
                SIGNAL(valueChanged(int)),
                SLOT(setMaxNumberSeeds(int)));
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setTractographyFiducialSeedingNode(vtkMRMLNode *node)
{
  vtkMRMLTractographyFiducialSeedingNode *paramNode = vtkMRMLTractographyFiducialSeedingNode::SafeDownCast(node);

  // each time the node is modified, the logic creates tracks
  vtkSlicerTractographyFiducialSeedingLogic *seedingLogic = 
        vtkSlicerTractographyFiducialSeedingLogic::SafeDownCast(this->logic());
  if (seedingLogic && this->mrmlScene())
  {
    seedingLogic->SetAndObserveTractographyFiducialSeedingNode(paramNode);
  }

  // each time the node is modified, the qt widgets are updated
  this->qvtkReconnect(this->TractographyFiducialSeedingNode, paramNode, 
                       vtkCommand::ModifiedEvent, this, SLOT(updateWidgetfromMRML()));

  this->TractographyFiducialSeedingNode = paramNode;
  this->updateWidgetfromMRML();
}


//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setTransformableNode(vtkMRMLNode *node)
{
  vtkMRMLTransformableNode *transformableNode = vtkMRMLTransformableNode::SafeDownCast(node);

  this->qvtkReconnect(this->TransformableNode, transformableNode,
                vtkMRMLTransformableNode::TransformModifiedEvent, this, SLOT(onParameterChanged()));
  this->qvtkReconnect(this->TransformableNode, transformableNode,
                vtkMRMLModelNode::PolyDataModifiedEvent, this, SLOT(onParameterChanged()));
  this->qvtkReconnect(this->TransformableNode, transformableNode,
                vtkMRMLFiducialListNode::FiducialModifiedEvent, this, SLOT(onParameterChanged()));

  this->TransformableNode = transformableNode;
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetInputFiducialRef(this->TransformableNode ? 
                                                                this->TransformableNode->GetID() : "" );
  }
}
//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setDiffusionTensorVolumeNode(vtkMRMLNode *node)
{
  vtkMRMLDiffusionTensorVolumeNode *diffusionTensorVolumeNode = vtkMRMLDiffusionTensorVolumeNode::SafeDownCast(node);

  this->qvtkReconnect(this->DiffusionTensorVolumeNode, diffusionTensorVolumeNode,
                      vtkCommand::ModifiedEvent, this, SLOT(onParameterChanged()));

  this->DiffusionTensorVolumeNode = diffusionTensorVolumeNode;

  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetInputVolumeRef(this->DiffusionTensorVolumeNode ? 
                                                              this->DiffusionTensorVolumeNode->GetID() : "" );
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setFiberBundleNode(vtkMRMLNode *node)
{
  this->FiberBundleNode = vtkMRMLFiberBundleNode::SafeDownCast(node);
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetOutputFiberRef(this->FiberBundleNode ?
                                                              this->FiberBundleNode->GetID() : "" );
  }
}



//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::onParameterChanged()
{
  vtkSlicerTractographyFiducialSeedingLogic *seedingLogic = 
        vtkSlicerTractographyFiducialSeedingLogic::SafeDownCast(this->logic());
  if (seedingLogic && this->mrmlScene() && this->TractographyFiducialSeedingNode)
  {
    seedingLogic->ProcessMRMLEvents (NULL, 
                                     vtkCommand::ModifiedEvent,
                                     this->TractographyFiducialSeedingNode );
  }
}


//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::updateWidgetfromMRML()
{
  QCTK_D(qSlicerTractographyFiducialSeedingModuleWidget);
  
  vtkMRMLTractographyFiducialSeedingNode *paramNode = this->TractographyFiducialSeedingNode;

  if (paramNode && this->mrmlScene())
    {

    d->IntegrationStepSpinBoxLabel->setValue(paramNode->GetIntegrationStep());
    d->MaxNumberSeedsNumericInput->setValue(paramNode->GetMaxNumberOfSeeds());
    d->MinimumPathSpinBoxLabel->setValue(paramNode->GetMinimumPathLength());
    d->FiducialRegionSpinBoxLabel->setValue(paramNode->GetSeedingRegionSize());
    d->FiducialStepSpinBoxLabel->setValue(paramNode->GetSeedingRegionStep());
    d->SeedSelectedCheckBox->setChecked(paramNode->GetSeedSelectedFiducials()==1);
    d->StoppingCurvatureSpinBoxLabel->setValue(paramNode->GetStoppingCurvature());
    d->StoppingCriteriaComboBox->setCurrentIndex(paramNode->GetStoppingMode());
    d->StoppingValueSpinBoxLabel->setValue(paramNode->GetStoppingValue());
    d->DisplayTracksComboBox->setCurrentIndex(paramNode->GetDisplayMode());

    d->ParameterNodeSelector->setCurrentNode(
      this->mrmlScene()->GetNodeByID(paramNode->GetID()));
    d->FiberNodeSelector->setCurrentNode(
      this->mrmlScene()->GetNodeByID(paramNode->GetOutputFiberRef()));
    d->FiducialNodeSelector->setCurrentNode(
      this->mrmlScene()->GetNodeByID(paramNode->GetInputFiducialRef()));
    d->DTINodeSelector->setCurrentNode(
      this->mrmlScene()->GetNodeByID(paramNode->GetInputVolumeRef()));

    }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setStoppingCurvature(double value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetStoppingCurvature(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setStoppingValue(double value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetStoppingValue(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setIntegrationStep(double value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetIntegrationStep(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setMinimumPath(double value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetMinimumPathLength(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setFiducialRegion(double value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetSeedingRegionSize(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setFiducialRegionStep(double value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetSeedingRegionStep(value);
  }
}


//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setStoppingCriteria(int value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetStoppingMode(value);
  }
}
//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setTrackDisplayMode(int value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetDisplayMode(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setSeedSelectedFiducials(int value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetSeedSelectedFiducials(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setMaxNumberSeeds(int value)
{
  if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetMaxNumberOfSeeds(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerTractographyFiducialSeedingModuleWidget::setEnableSeeding(int value)
{
 if (this->TractographyFiducialSeedingNode)
  {
    this->TractographyFiducialSeedingNode->SetEnableSeeding(value);
  }
}

