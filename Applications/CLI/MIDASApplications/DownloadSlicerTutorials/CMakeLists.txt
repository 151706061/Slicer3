project (DownloadSlicerTutorials)

#
# Disable MSVC 8 warnings
if(WIN32)
  option(DISABLE_MSVC8_DEPRECATED_WARNINGS 
    "Disable Visual Studio 8 deprecated warnings" ON)
  mark_as_advanced(FORCE DISABLE_MSVC8_DEPRECATED_WARNINGS)  
  if(DISABLE_MSVC8_DEPRECATED_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  endif(DISABLE_MSVC8_DEPRECATED_WARNINGS)
endif(WIN32)

#
# Find VTK
find_package(VTK REQUIRED)
include_directories(${VTK_SOURCE_DIR})

#
# Find SLICERLIBCURL (slicercurl)
find_package(SLICERLIBCURL REQUIRED)
include(${SLICERLIBCURL_USE_FILE})

#
# Find GenerateCLP
find_package(GenerateCLP REQUIRED)
if(GenerateCLP_FOUND)
  include(${GenerateCLP_USE_FILE})
endif(GenerateCLP_FOUND)

#
# Set and generate source
set(DownloadSlicerTutorials_SOURCE DownloadSlicerTutorials.cxx)

generateclp(DownloadSlicerTutorials_SOURCE DownloadSlicerTutorials.xml)

#
# Generate shared loadable slicer module
add_library(DownloadSlicerTutorialsModule SHARED
  ${DownloadSlicerTutorials_SOURCE})
set_target_properties (DownloadSlicerTutorialsModule PROPERTIES
  COMPILE_FLAGS "-Dmain=ModuleEntryPoint")
slicer3_set_plugins_output_path(DownloadSlicerTutorialsModule)
target_link_libraries(DownloadSlicerTutorialsModule 
  ${SLICERLIBCURL_LIBRARIES} ITKIO MRMLIDIO itkzlib)

#
# Generate executable
add_executable(DownloadSlicerTutorials ${CLI_SOURCE_DIR}/Templates/CommandLineSharedLibraryWrapper.cxx)
slicer3_set_plugins_output_path(DownloadSlicerTutorials)
target_link_libraries(DownloadSlicerTutorials 
  DownloadSlicerTutorialsModule)

#
# Move exec and module to where Slicer can find them
if (Slicer3_SOURCE_DIR)

  set(TARGETS DownloadSlicerTutorials DownloadSlicerTutorialsModule)
  slicer3_install_plugins(${TARGETS})

endif (Slicer3_SOURCE_DIR)
