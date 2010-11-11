/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkVolumeRenderingGUI.h,v $
Date:      $Date: 2006/03/19 17:12:29 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkVolumeRenderingGUI_h
#define __vtkVolumeRenderingGUI_h

#include "vtkSlicerModuleGUI.h"
#include "vtkVolumeRendering.h"
#include "vtkVolumeRenderingLogic.h"

#include "vtkSlicerNodeSelectorWidget.h"

#include "vtkKWLabel.h"
#include "vtkKWHistogram.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWTkUtilities.h"
#include "vtkMRMLVolumeRenderingParametersNode.h"
#include "vtkMRMLVolumeRenderingScenarioNode.h"
#include "vtkMRMLVolumePropertyNode.h"

#include <string>

class vtkKWMenu;
class vtkKWCheckButtonWithLabel;
class vtkSlicerVolumeRenderingHelper;
class vtkSlicerVolumePropertyWidget;

class VTK_SLICERVOLUMERENDERING_EXPORT vtkVolumeRenderingGUI :public vtkSlicerModuleGUI
{
public:

    static vtkVolumeRenderingGUI *New();
    vtkTypeMacro(vtkVolumeRenderingGUI,vtkSlicerModuleGUI);

    void PrintSelf(ostream& os, vtkIndent indent);

    // Description: Get/Set module logic
    vtkGetObjectMacro (Logic, vtkVolumeRenderingLogic);
    virtual void SetLogic(vtkVolumeRenderingLogic *log)
    {
        this->Logic=log;
        this->Logic->RegisterNodes();
    }

    // Description: Implements vtkSlicerModuleGUI::SetModuleLogic for this GUI
    virtual void SetModuleLogic(vtkSlicerLogic *logic)
    {
      this->SetLogic( dynamic_cast<vtkVolumeRenderingLogic*> (logic) );
    }

    //vtkSetObjectMacro (Logic, vtkVolumeRenderingLogic);
    // Description:
    // Create widgets
    virtual void BuildGUI ( );
    //BTX
    using vtkSlicerComponentGUI::BuildGUI;
    //ETX

    // Description:
    // This method releases references and key-bindings,
    // and optionally removes observers.
    virtual void TearDownGUI ( );

    // Description:
    // Central spots to manage volume actors/mappers and render widgets
    virtual void AddVolumeToViewers ( );
    virtual void RemoveVolumeFromViewers ( );

    // Description:
    // Methods for adding module-specific key bindings and
    // removing them.
    virtual void CreateModuleEventBindings ( );
    virtual void ReleaseModuleEventBindings ( );

    // Description:
    // Add obsereves to GUI widgets
    virtual void AddGUIObservers ( );
    virtual void AddMRMLObservers ( );

    // Description:
    // Remove obsereves to GUI widgets
    virtual void RemoveGUIObservers ( );
    virtual void RemoveMRMLObservers ( );
    virtual void RemoveLogicObservers ( );

    // Description:
    // Process events generated by Logic
    virtual void ProcessLogicEvents(vtkObject *vtkNotUsed(caller),
                                    unsigned long vtkNotUsed(event),
                                    void *vtkNotUsed(callData)){};

    // Description:
    // Process events generated by GUI widgets
    virtual void ProcessGUIEvents ( vtkObject *caller, unsigned long event,
        void *callData );

    // Description:
    // Process events generated by MRML
    virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event,
        void *callData);


    // Description:
    // Methods describe behavior at module enter and exit.
    virtual void Enter ( );
    //BTX
    using vtkSlicerComponentGUI::Enter; 
    //ETX
    virtual void Exit ( );

    void RequestRender();

    void PauseRenderInteraction();
    void ResumeRenderInteraction();

    // Description:
    // Get/Set the main slicer viewer widget, for picking
    vtkGetObjectMacro(ViewerWidget, vtkSlicerViewerWidget);
    virtual void SetViewerWidget(vtkSlicerViewerWidget *viewerWidget);

    // Description:
    // Get/Set the slicer interactorstyle, for picking
    vtkGetObjectMacro(InteractorStyle, vtkSlicerViewerInteractorStyle);
    virtual void SetInteractorStyle(vtkSlicerViewerInteractorStyle *interactorStyle);

    // Description:
    // Get methods on class members ( no Set methods required. )
    vtkGetObjectMacro (NS_ImageData,vtkSlicerNodeSelectorWidget);
    vtkGetObjectMacro (NS_ImageDataFg,vtkSlicerNodeSelectorWidget);
    vtkGetObjectMacro (NS_ImageDataLabelmap,vtkSlicerNodeSelectorWidget);

    vtkGetObjectMacro (NS_VolumePropertyPresets,vtkSlicerNodeSelectorWidget);
    vtkGetObjectMacro (NS_VolumeProperty,vtkSlicerNodeSelectorWidget);
    vtkGetObjectMacro (NS_VolumePropertyPresetsFg,vtkSlicerNodeSelectorWidget);
    vtkGetObjectMacro (NS_VolumePropertyFg,vtkSlicerNodeSelectorWidget);

    vtkGetObjectMacro (RenderingFrame, vtkSlicerModuleCollapsibleFrame);
    vtkGetObjectMacro (Presets, vtkMRMLScene);
    vtkGetObjectMacro (Helper, vtkSlicerVolumeRenderingHelper);

    vtkMRMLVolumeRenderingParametersNode* GetCurrentParametersNode();
    vtkMRMLVolumePropertyNode* GetVolumePropertyNode();
    vtkMRMLVolumePropertyNode* GetFgVolumePropertyNode();

protected:
    vtkVolumeRenderingGUI();
    ~vtkVolumeRenderingGUI();
    vtkVolumeRenderingGUI(const vtkVolumeRenderingGUI&);//not implemented
    void operator=(const vtkVolumeRenderingGUI&);//not implemented

    // Description:
    // Updates GUI widgets based on parameters values in MRML node
    void UpdateGUI();

    void DisplayMessageDialog(const char *message);

    // Description:
    // Updates parameters values in MRML node based on GUI widgets
//    void UpdateMRML();

    // Description:
    // GUI elements

    // Description:
    // Pointer to the module's logic class
    vtkVolumeRenderingLogic *Logic;

    vtkMRMLVolumeRenderingScenarioNode *ScenarioNode;

    // Description:
    // A pointer back to the viewer widget, useful for picking
    vtkSlicerViewerWidget *ViewerWidget;

    // Description:
    // A poitner to the interactor style, useful for picking
    vtkSlicerViewerInteractorStyle *InteractorStyle;

    void InitializePipelineNewVolumeProperty();
    void InitializePipelineNewVolumePropertyFg();

    void InitializePipelineFromImageData();
    void InitializePipelineFromImageDataFg();

    void InitializePipelineFromParametersNode();

    void UpdatePipelineByVolumeProperty();
    void UpdatePipelineByFgVolumeProperty();

    void UpdatePipelineByDisplayNode();
    
    void UpdatePipelineByROI();

    void LoadPresets();
    void PopulatePresetIcons(vtkKWMenu *menu);

    int ValidateParametersNode(vtkMRMLVolumeRenderingParametersNode* vspNode);

    //OWN GUI Elements

    //Frame input
    vtkKWCheckButtonWithLabel   *CB_VolumeRenderingOnOff;
    vtkSlicerNodeSelectorWidget *NS_ParametersSet;

    vtkSlicerNodeSelectorWidget *NS_ImageData;
    vtkSlicerNodeSelectorWidget *NS_ImageDataFg;
    vtkSlicerNodeSelectorWidget *NS_ImageDataLabelmap;

    //ETX
    vtkSlicerNodeSelectorWidget *NS_VolumePropertyPresets;
    vtkSlicerNodeSelectorWidget *NS_VolumeProperty;

    vtkSlicerNodeSelectorWidget *NS_VolumePropertyPresetsFg;
    vtkSlicerNodeSelectorWidget *NS_VolumePropertyFg;

    vtkSlicerNodeSelectorWidget *NS_ROI;

    //Frame Details
    vtkSlicerModuleCollapsibleFrame *RenderingFrame;

    //Other members
    vtkMRMLScene *Presets;

    void CreateRenderingFrame(void);
    void DeleteRenderingFrame(void);

    vtkSlicerVolumeRenderingHelper *Helper;

    int UpdatingGUI;

    int ProcessingGUIEvents;
    int ProcessingMRMLEvents;

    // check abort event
    void CheckAbort(void);

    int NewVolumePropertyAddedFlag;
    int NewFgVolumePropertyAddedFlag;

    int NewParametersNodeForNewInputFlag;
    int NewParametersNodeFromSceneLoadingFlag;
};

#endif
