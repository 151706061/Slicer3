PROJECT(PETCTFusion)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)


INCLUDE(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
TEST_BIG_ENDIAN(CMAKE_WORDS_BIGENDIAN)

# --------------------------------------------------------------------------
# Find Slicer3

if(NOT Slicer3_SOURCE_DIR)
  find_package(Slicer3 REQUIRED)
  include(${Slicer3_USE_FILE})
  slicer3_set_default_install_prefix_for_external_projects()
endif(NOT Slicer3_SOURCE_DIR)

# --------------------------------------------------------------------------
# Include dirs

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Slicer3_Libs_INCLUDE_DIRS}
  ${Slicer3_Base_INCLUDE_DIRS}
  )

# --------------------------------------------------------------------------
# Configure include file

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkPETCTFusionConfigure.h.in 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPETCTFusionConfigure.h
  )

file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES 
  ${headers} 
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPETCTFusionConfigure.h"
  DESTINATION ${Slicer3_INSTALL_MODULES_INCLUDE_DIR}/${PROJECT_NAME} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Sources

set(PETCTFusion_SRCS
  vtkMRMLPETCTFusionNode.cxx        
  vtkPETCTFusionLogic.cxx
  vtkPETCTFusionGUI.cxx
  vtkPETCTFusionPlots.cxx
  )

# --------------------------------------------------------------------------
# Wrapping

include("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
vtk_wrap_tcl3(PETCTFusion PETCTFusion_TCL_SRCS "${PETCTFusion_SRCS}" "")


# --------------------------------------------------------------------------
# Add Loadable Module support

generatelm(PETCTFusion_SRCS PETCTFusion.txt)

# --------------------------------------------------------------------------
# Build and install the library

set(lib_name PETCTFusion)
add_library(${lib_name} 
  ${PETCTFusion_SRCS}
  ${PETCTFusion_TCL_SRCS}
  )
slicer3_set_modules_output_path(${lib_name})

target_link_libraries(${lib_name}
  ${Slicer3_Libs_LIBRARIES}
  ${Slicer3_Base_LIBRARIES}
  ${KWWidgets_LIBRARIES}
  ITKIO
  )

slicer3_install_modules(${lib_name})

