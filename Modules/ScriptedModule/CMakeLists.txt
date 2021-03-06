project(ScriptedModule)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)

 
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

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkScriptedModuleConfigure.h.in 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkScriptedModuleConfigure.h
  )

file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES 
  ${headers} 
  "${CMAKE_CURRENT_BINARY_DIR}/vtkScriptedModuleConfigure.h"
  DESTINATION ${Slicer3_INSTALL_MODULES_INCLUDE_DIR}/${PROJECT_NAME} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Sources

set(ScriptedModule_SRCS
  vtkScriptedModuleLogic.cxx
  vtkScriptedModuleGUI.cxx
  vtkMRMLScriptedModuleNode.cxx
  vtkOpenGLScriptedActor.cxx
  )

# --------------------------------------------------------------------------
# Wrapping

include("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
vtk_wrap_tcl3(ScriptedModule ScriptedModule_TCL_SRCS "${ScriptedModule_SRCS}" "")

# --------------------------------------------------------------------------
# Build and install the library

set(lib_name ScriptedModule)
add_library(${lib_name}
  ${ScriptedModule_SRCS}
  ${ScriptedModule_TCL_SRCS}
  )
slicer3_set_modules_output_path(${lib_name})

target_link_libraries(${lib_name}
  ${Slicer3_Libs_LIBRARIES}
  ${Slicer3_Base_LIBRARIES}
  ${KWWidgets_LIBRARIES}
  ${ITK_LIBRARIES}
  )

# Python settings

if(Slicer3_USE_PYTHON)
  find_package(PythonLibs REQUIRED)
  include_directories(${PYTHON_INCLUDE_PATH})
  if(WIN32)
    include_directories(${PYTHON_INCLUDE_PATH}/../PC)
  endif(WIN32)
  if(WIN32 AND PYTHON_DEBUG_LIBRARIES)
    target_link_libraries(${lib_name}
      debug ${PYTHON_DEBUG_LIBRARIES}
      optimized ${PYTHON_LIBRARIES})
  else(WIN32 AND PYTHON_DEBUG_LIBRARIES)
    target_link_libraries(${lib_name} ${PYTHON_LIBRARIES})
  endif(WIN32 AND PYTHON_DEBUG_LIBRARIES)
  if(NOT WIN32)
    if(APPLE OR UNIX)
      target_link_libraries(${lib_name} ${PYTHON_LIBRARIES})
    else(APPLE OR UNIX)
      target_link_libraries(${lib_name} ${PYTHON_LIBRARIES} util)
    endif(APPLE OR UNIX)
  endif(NOT WIN32)
endif(Slicer3_USE_PYTHON)

# Apply user-defined properties to the library target.
IF(Slicer3_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(${lib_name} PROPERTIES
     ${Slicer3_LIBRARY_PROPERTIES}
  )
ENDIF(Slicer3_LIBRARY_PROPERTIES)

slicer3_install_modules(${lib_name})

# --------------------------------------------------------------------------
# Testing (requires some of the examples)

if(BUILD_TESTING)
  #  ADD_SUBDIRECTORY(Testing)
endif(BUILD_TESTING)
