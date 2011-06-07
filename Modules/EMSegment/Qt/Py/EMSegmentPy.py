from __main__ import vtk, qt, ctk, slicer

import EMSegmentPyWizard
from EMSegmentPyWizard import Helper

class EMSegmentPy:
  def __init__(self, parent):
    parent.title = "EMSegmentPy"
    parent.category = ""
    parent.contributor = "--"
    parent.helpText = """dsfdsf"""
    parent.acknowledgementText = """sdfsdfdsf"""
    self.parent = parent

class EMSegmentPyWidget:
  def __init__(self, parent=None):
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)      
    else:
      self.parent = parent
    self.layout = self.parent.layout()

    # this flag is 1 if there is an update in progress
    self.__updating = 1
    
    # the pointer to the logic and the mrmlManager
    self.__mrmlManager = None
    self.__logic = None

    if not parent:
      self.setup()

      # after setup, be ready for events
      self.__updating = 0      
      
      self.parent.show()
      
    # register default slots
    #self.parent.connect('mrmlSceneChanged(vtkMRMLScene*)', self.onMRMLSceneChanged)      
    

  def logic(self):
    if not self.__logic:
        self.__logic = slicer.modulelogic.vtkEMSegmentLogic()
        self.__logic.SetModuleName("EMSegment")
        self.__logic.SetMRMLScene(slicer.mrmlScene)
        self.__logic.RegisterNodes()
        self.__logic.InitializeEventListeners()   
    
    return self.__logic

  def mrmlManager(self):
    if not self.__mrmlManager:
        self.__mrmlManager = slicer.modulelogic.vtkEMSegmentLogic().GetMRMLManager()
        self.__mrmlManager.SetMRMLScene(slicer.mrmlScene)
    
    return self.__mrmlManager

    
  def setup(self):
    '''
    Create and start the EMSegment workflow.
    '''
    self.workflow = ctk.ctkWorkflow()
    
    workflowWidget = ctk.ctkWorkflowStackedWidget()
    workflowWidget.setWorkflow(self.workflow)
    
    workflowWidget.buttonBoxWidget().nextButtonDefaultText = ""
    workflowWidget.buttonBoxWidget().backButtonDefaultText = ""
           
    # create all wizard steps
    selectTaskStep = EMSegmentPyWizard.EMSegmentPyStepOne( Helper.GetNthStepId(1) )
    defineInputChannelsSimpleStep = EMSegmentPyWizard.EMSegmentPyStepTwo( Helper.GetNthStepId(2) + 'simple' ) # simple branch
    defineInputChannelsAdvancedStep = EMSegmentPyWizard.EMSegmentPyStepTwo( Helper.GetNthStepId(2) + 'advanced' ) # advanced branch
    defineAnatomicalTreeStep = EMSegmentPyWizard.EMSegmentPyStepThree( Helper.GetNthStepId(3) )
    defineAtlasStep = EMSegmentPyWizard.EMSegmentPyDummyStep( Helper.GetNthStepId(4) )
    editRegistrationParametersStep = EMSegmentPyWizard.EMSegmentPyDummyStep( Helper.GetNthStepId(5) )
    definePreprocessingStep = EMSegmentPyWizard.EMSegmentPyDummyStep( Helper.GetNthStepId(6) )
    specifyIntensityDistributionStep = EMSegmentPyWizard.EMSegmentPyDummyStep( Helper.GetNthStepId(7) ) 
    editNodeBasedParametersStep = EMSegmentPyWizard.EMSegmentPyDummyStep( Helper.GetNthStepId(8) )
    miscStep = EMSegmentPyWizard.EMSegmentPyDummyStep( Helper.GetNthStepId(9) )
    segmentStep = EMSegmentPyWizard.EMSegmentPyDummyStep( Helper.GetNthStepId(10) )
    
    # add the wizard steps to an array for convenience
    allSteps = []
    
    allSteps.append(selectTaskStep)
    allSteps.append(defineInputChannelsSimpleStep)
    allSteps.append(defineInputChannelsAdvancedStep)
    allSteps.append(defineAnatomicalTreeStep)
    allSteps.append(defineAtlasStep)
    allSteps.append(editRegistrationParametersStep)
    allSteps.append(definePreprocessingStep)
    allSteps.append(specifyIntensityDistributionStep)
    allSteps.append(editNodeBasedParametersStep)
    allSteps.append(miscStep)
    allSteps.append(segmentStep)
    
    # Add transition for the first step which let's the user choose between simple and advanced mode
    self.workflow.addTransition(selectTaskStep, defineInputChannelsSimpleStep, 'SimpleMode')
    self.workflow.addTransition(selectTaskStep, defineInputChannelsAdvancedStep, 'AdvancedMode')
    
    # Add transitions associated to the simple mode
    self.workflow.addTransition(defineInputChannelsSimpleStep,segmentStep)
    
    # Add transitions associated to the advanced mode
    self.workflow.addTransition(defineInputChannelsAdvancedStep,defineAnatomicalTreeStep)    
    
    # .. add transitions for the rest of the advanced mode steps
    for i in range(2,len(allSteps) - 1):
      self.workflow.addTransition(allSteps[i], allSteps[i + 1])
    
    # Propagate the workflow, the logic and the MRML Manager to the steps
    for s in allSteps:
        s.setWorkflow(self.workflow)
        s.setLogic(self.logic())
        s.setMRMLManager(self.mrmlManager())
    
    # start the workflow and show the widget
    self.workflow.start()
    workflowWidget.visible = True
    self.layout.addWidget(workflowWidget)    
    
    # compress the layout
    #self.layout.addStretch(1)        
      
      
