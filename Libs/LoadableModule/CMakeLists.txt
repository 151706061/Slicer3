project(LoadableModule)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)

 
## ITK is required for expat.h

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

if(LoadableModule_USE_PYTHON)
  find_package(PythonLibs)
endif(LoadableModule_USE_PYTHON)

if(NOT DEFINED BUILD_SHARED_LIBS)
  option(BUILD_SHARED_LIBS "Build with shared libraries." ON)
endif(NOT DEFINED BUILD_SHARED_LIBS)
 
# --------------------------------------------------------------------------
# Include dirs

set(include_dirs
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

if(LoadableModule_USE_PYTHON)
  # Python requires a pointer to the Slicer Application
  set(include_dirs ${include_dirs} ${PYTHON_INCLUDE_PATH})
  if(WIN32)
    set(include_dirs ${include_dirs} ${PYTHON_INCLUDE_PATH}/../PC)
  endif(WIN32)
endif(LoadableModule_USE_PYTHON)

include_directories(${include_dirs})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadableModuleConfigure.h.in 
  ${CMAKE_CURRENT_BINARY_DIR}/LoadableModuleConfigure.h
  )

file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES 
  ${headers} 
  "${CMAKE_CURRENT_BINARY_DIR}/LoadableModuleConfigure.h"
  DESTINATION include/${PROJECT_NAME} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Sources

set(LoadableModule_SRCS
  LoadableModuleDescription.cxx
  LoadableModuleDescriptionParser.cxx
  LoadableModuleFactory.cxx
  )

if(USE_BFD)
  if(NOT WIN32)
    include(CheckIncludeFile)
    check_include_file(bfd.h HAVE_BFD_HEADER)

    if(HAVE_BFD_HEADER)
      # make sure we can build with libbfd
      message(STATUS "Testing libbfd")
      try_compile(HAVE_BFD
        ${CMAKE_CURRENT_BINARY_DIR}/CMake
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake
        TestBFD
        CMAKE_FLAGS 
        -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        OUTPUT_VARIABLE OUTPUT)
      message(${OUTPUT})
      if(HAVE_BFD)
        message(STATUS "Testing libbfd - ok. ModuleFactory will look for global symbols in plugin executables.")
      else(HAVE_BFD)
        message(STATUS "Testing libbfd - error.  ModuleFactory will not look for global symbols in plugin executables.")
      endif(HAVE_BFD)
    endif(HAVE_BFD_HEADER)
    
    if(HAVE_BFD)
      set(LoadableModule_SRCS 
        ${LoadableModule_SRCS} 
        BinaryFileDescriptor.cxx)
    endif(HAVE_BFD)
  endif(NOT WIN32)
endif(USE_BFD)

# --------------------------------------------------------------------------
# Build and install the library

set(lib_name LoadableModule)
add_library(${lib_name} 
  ${LoadableModule_SRCS}
  )

set(link_libs
  ITKEXPAT
  itksys
  )
if(LoadableModule_USE_PYTHON)
  set(link_libs ${link_libs} ${PYTHON_LIBRARIES})
endif(LoadableModule_USE_PYTHON)
if(NOT WIN32 AND NOT APPLE AND NOT UNIX)
  set(link_libs ${link_libs} util)
endif(NOT WIN32 AND NOT APPLE AND NOT UNIX)
if(NOT WIN32 AND HAVE_BFD)
  set(link_libs ${link_libs} bfd iberty)
endif(NOT WIN32 AND HAVE_BFD)

target_link_libraries(${lib_name} ${link_libs})

# Apply user-defined properties to the library target.
IF(Slicer3_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(${lib_name} PROPERTIES
     ${Slicer3_LIBRARY_PROPERTIES}
  )
ENDIF(Slicer3_LIBRARY_PROPERTIES)

install(TARGETS ${lib_name} 
  RUNTIME DESTINATION bin COMPONENT RuntimeLibraries
  LIBRARY DESTINATION lib/${PROJECT_NAME} COMPONENT RuntimeLibraries
  ARCHIVE DESTINATION lib/${PROJECT_NAME} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Testing

if(BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
endif(BUILD_TESTING)

# --------------------------------------------------------------------------
# Install support files

include(GenerateLoadableModuleConfig.cmake) 

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/UseLoadableModule.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/UseLoadableModule.cmake COPYONLY IMMEDIATE)

install(FILES 
  ${CMAKE_CURRENT_BINARY_DIR}/UseLoadableModule.cmake 
  ${CMAKE_CURRENT_BINARY_DIR}/install/LoadableModuleConfig.cmake 
  DESTINATION lib/${PROJECT_NAME} COMPONENT Development
  )
