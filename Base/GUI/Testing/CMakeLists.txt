SET(KIT BaseGUI)
SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  vtkSlicerModelsGUITest1.cxx
  vtkSlicerModuleGUITest1.cxx
  vtkSlicerComponentGUITest1.cxx
  EXTRA_INCLUDE TestingMacros.h
  )

SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

SET(LIBRARY_NAME ${PROJECT_NAME})

# --------------------------------------------------------------------------
# Testing

SET(SHORT_TEST_TIMEOUT 300)  ## Sometimes when the GUI crashes, it won't exit properly, and then the tests have to wait until they reach 1500 seconds before they time out

ADD_TEST(Slicer3GUITest1 ${Slicer3_BINARY_DIR}/Slicer3 --test-mode --script ${Slicer3_SOURCE_DIR}/Base/GUI/Testing/TestSliceController.tcl)
ADD_TEST(Slicer3GUITest2 ${Slicer3_BINARY_DIR}/Slicer3 --test-mode --script ${Slicer3_SOURCE_DIR}/Base/GUI/Testing/TestProgressDialog.tcl)
ADD_TEST(Slicer3FiducialsTest1 ${Slicer3_BINARY_DIR}/Slicer3 --test-mode --script ${Slicer3_SOURCE_DIR}/Base/GUI/Testing/TestFiducials.tcl)
ADD_TEST(Slicer3FiducialsTest2 ${Slicer3_BINARY_DIR}/Slicer3 --test-mode --script ${Slicer3_SOURCE_DIR}/Base/GUI/Testing/TestFiducialsAdd.tcl)
ADD_TEST(Slicer3FiducialsTestGlyphs ${Slicer3_BINARY_DIR}/Slicer3 --test-mode --script ${Slicer3_SOURCE_DIR}/Base/GUI/Testing/TestFiducialsGlyphs.tcl)
ADD_TEST(Slicer3FiducialsTestChangeGlyphs ${Slicer3_BINARY_DIR}/Slicer3 --test-mode --script ${Slicer3_SOURCE_DIR}/Base/GUI/Testing/TestFiducialsAddSymbolScale.tcl)
ADD_TEST(Slicer3RestoreSceneSnapshotCompareViewTest1 ${Slicer3_BINARY_DIR}/Slicer3 --test-mode --script ${Slicer3_SOURCE_DIR}/Base/GUI/Testing/TestRestoreSceneSnapshotCompareView.tcl)

SET_TESTS_PROPERTIES( Slicer3FiducialsTest1 PROPERTIES TIMEOUT ${SHORT_TEST_TIMEOUT} )
SET_TESTS_PROPERTIES( Slicer3FiducialsTest2 PROPERTIES TIMEOUT ${SHORT_TEST_TIMEOUT} )
SET_TESTS_PROPERTIES( Slicer3RestoreSceneSnapshotCompareViewTest1 PROPERTIES TIMEOUT ${SHORT_TEST_TIMEOUT} )

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests 
  ${lib_name} 
  SlicerBaseGUI
  )

SET( BaseGUI_TESTS ${CXX_TEST_PATH}/${KIT}CxxTests)

IF(WIN32)
  SET(BaseGUI_TESTS ${CXX_TEST_PATH}/${CMAKE_BUILD_TYPE}/${KIT}CxxTests)
ENDIF(WIN32)

MACRO( SIMPLE_TEST  TESTNAME )
  ADD_TEST( ${TESTNAME} ${LAUNCH_EXE} ${BaseGUI_TESTS} ${TESTNAME} 
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SlicerQT
    )
ENDMACRO( SIMPLE_TEST  )

SIMPLE_TEST( vtkSlicerModelsGUITest1 )
SIMPLE_TEST( vtkSlicerModuleGUITest1 )
SIMPLE_TEST( vtkSlicerComponentGUITest1 )
