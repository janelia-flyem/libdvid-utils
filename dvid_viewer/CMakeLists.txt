CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project (dvid-viewer)

include (ExternalProject)

add_definitions(-std=c++11)
set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set (CMAKE_CXX_LINK_FLAGS "-O3")
set (CMAKE_DEBUG_POSTFIX "-g")



FIND_PACKAGE(VTK REQUIRED)
FIND_PACKAGE(Qt4 REQUIRED) 
FIND_PACKAGE(PythonLibs)
FIND_PACKAGE(Boost)
find_package ( libdvidcpp)
include(${QT_USE_FILE}) 

include_directories (${Boost_INCLUDE_DIR})
include_directories(${LIBDVIDCPP_INCLUDE_DIRS})
include_directories (AFTER ${CMAKE_SOURCE_DIR}/src/external_packages)

INCLUDE_DIRECTORIES(
            ${QT_INCLUDE_DIR}
            ${QT_QTGUI_INCLUDE_DIR}
            ${QT_QTCORE_INCLUDE_DIR}
        )
INCLUDE(${VTK_USE_FILE})

        
set (vtk_LIBS vtkHybrid vtkRendering vtkVolumeRendering vtkWidgets vtkCommon QVTK)
set (qt_LIBS  ${QT_LIBRARIES})
set (boost_LIBS boost_thread boost_system boost_program_options boost_unit_test_framework boost_filesystem)
set (json_LIB jsoncpp)
set (libdvid_LIBS dvidcpp.a  png curl jpeg lz4)

add_subdirectory (Model)
add_subdirectory (Gui)

# Handle all sources and dependent code
add_executable (dvid_viewer dvid_viewer.cpp)

#${BUILDEM_DIR}/lib/libdvidcpp.a
target_link_libraries (dvid_viewer  dvidviewer_model dvidviewer_gui ${vtk_LIBS} ${boost_LIBS} ${qt_LIBS} ${libdvid_LIBS} ${json_LIB} /Users/plazas/miniconda/envs/dvidviewer/lib/liblowtis.dylib)

install (TARGETS dvid_viewer DESTINATION bin)
