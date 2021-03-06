project (DicomToNrrdConverter)

#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.0)
#-----------------------------------------------------------------------------


enable_testing()

if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND CMAKE_POLICY)

SET(RUN_EXTENDED_LOCAL_TESTING OFF CACHE BOOL "Run extended tests? Only set to true if running at UIowa.")

if (RUN_EXTENDED_LOCAL_TESTING)
  ADD_SUBDIRECTORY(IowaTesting)
endif (RUN_EXTENDED_LOCAL_TESTING)

FIND_PACKAGE(ITK REQUIRED)

if (NOT ITK_USE_REVIEW)
  MESSAGE(FATAL_ERROR "ITK version was not built with ITK_USE_REVIEW true")
endif (NOT ITK_USE_REVIEW)

if (NOT ITK_USE_REVIEW_STATISTICS)
  MESSAGE(FATAL_ERROR "ITK version was not built with ITK_USE_REVIEW_STATISTICS true")
endif (NOT ITK_USE_REVIEW_STATISTICS)

if (NOT ITK_USE_OPTIMIZED_REGISTRATION_METHODS)
  MESSAGE(FATAL_ERROR "ITK version was not built with ITK_USE_OPTIMIZED_REGISTRATION_METHODS true")
endif (NOT ITK_USE_OPTIMIZED_REGISTRATION_METHODS)

if (Slicer3_SOURCE_DIR)
  ##There's limited testing from within Slicer3.  There's extensive testing when run 
  ##at UIowa but the testing data isn't publicly accessible yet.  Other packages that 
  ##use this tool will not run either set of tests
  ADD_SUBDIRECTORY(Testing)
endif (Slicer3_SOURCE_DIR)

set (CLP DicomToNrrdConverter)
set ( ${CLP}_SOURCE ${CLP}.cxx)

if (Slicer3_SOURCE_DIR)
  generateclp(${CLP}_SOURCE ${CLP}.xml ${Slicer3_SOURCE_DIR}/Resources/NAMICLogo.h)
else (Slicer3_SOURCE_DIR)
  find_package(GenerateCLP NO_MODULE REQUIRED)
  include(${GenerateCLP_USE_FILE})
  generateclp(${CLP}_SOURCE ${CLP}.xml)
endif (Slicer3_SOURCE_DIR)
add_executable(${CLP} ${${CLP}_SOURCE})
target_link_libraries (${CLP} ${ITK_LIBRARIES})

if (Slicer3_SOURCE_DIR)
  slicer3_set_plugins_output_path(${CLP})

  add_library(${CLP}Lib SHARED ${${CLP}_SOURCE})
  slicer3_set_plugins_output_path(${CLP}Lib)
  set_target_properties (${CLP}Lib PROPERTIES COMPILE_FLAGS "-Dmain=ModuleEntryPoint")
  target_link_libraries (${CLP}Lib ${ITK_LIBRARIES})

  set(TARGETS ${CLP}Lib ${CLP})
  slicer3_install_plugins(${TARGETS})
else (Slicer3_SOURCE_DIR)

install(TARGETS
  DicomToNrrdConverter
                 RUNTIME DESTINATION bin
                 LIBRARY DESTINATION lib
                 ARCHIVE DESTINATION lib/static)

endif (Slicer3_SOURCE_DIR)
