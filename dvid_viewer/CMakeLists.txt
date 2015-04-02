CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project (dvid-viewer)

include (ExternalProject)


set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set (CMAKE_CXX_LINK_FLAGS "-O3")
set (CMAKE_DEBUG_POSTFIX "-g")


################################################################################
# Check if BUILDEM_DIR has already been assigned.  If not, create a default.
set (BUILDEM_DIR "None" CACHE TYPE STRING)

if (${BUILDEM_DIR} STREQUAL "None")
    message ("WARNING: To use Buildem, Buildem directory (for all downloads & builds) should be specified via -DBUILDEM_DIR=<path> on cmake command line.")
    message ("Builds will be placed here: ${CMAKE_SOURCE_DIR}/bin")
else()
    message ("FlyEM downloads and builds will be placed here: ${BUILDEM_DIR}")
endif ()
###############################################################################

if (NOT ${BUILDEM_DIR} STREQUAL "None")
    ###############################################################################
    # Download and install buildem, if it isn't already in BUILDEM_DIR.
    set (BUILDEM_REPO_DIR ${BUILDEM_DIR}/src/buildem)
    if (NOT EXISTS ${BUILDEM_REPO_DIR}/python.cmake)
        message ("Installing buildem repo...")
        ExternalProject_Add(buildem
            PREFIX ${BUILDEM_DIR}
            GIT_REPOSITORY http://github.com/janelia-flyem/buildem.git
            UPDATE_COMMAND ""
            PATCH_COMMAND ""
            CONFIGURE_COMMAND "" 
            BUILD_COMMAND ""
            BUILD_IN_SOURCE 1
            INSTALL_COMMAND ""
        )
        message ("\n**********************************************************\n")
        message ("\nAfter running make, you must re-run the cmake command once")
        message ("buildem has been downloaded!\n")
        message ("\n***********************************************************\n")
        return()
    endif() 
    ###############################################################################


    # Use modules from the downloaded buildem
    set (CMAKE_MODULE_PATH ${BUILDEM_REPO_DIR})
    message("Using cmake modules from ${BUILDEM_REPO_DIR}")
    set (LIBDVID_VERSION master)

    set (QT_QMAKE_EXECUTABLE ${BUILDEM_DIR}/bin/qmake) 
    set (QT_BUILT FALSE)
    if (EXISTS ${QT_QMAKE_EXECUTABLE})
        SET (QT_BUILT TRUE)
    endif()

    if (NOT QT_BUILT)
        include (qt4)
        add_custom_target(RescanQt4 ALL ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR} DEPENDS ${qt4_NAME})
    else()
        add_custom_target(RescanQt4)
        FIND_PACKAGE(Qt4 REQUIRED) 
        include(${QT_USE_FILE}) 
    endif()

    include (libdvidcpp)
    include (boost)
    include (qt4)
    include (vtk)
    include (jsoncpp)

    include_directories (BEFORE ${PYTHON_INCLUDE_PATH})
    
    add_subdirectory (Model)
    add_subdirectory (Gui)
    
    #    set (boost_LIBS  ${BUILDEM_LIB_DIR}/libboost_thread.${BUILDEM_PLATFORM_DYLIB_EXTENSION}
    #               ${BUILDEM_LIB_DIR}/libboost_system.${BUILDEM_PLATFORM_DYLIB_EXTENSION} )

    set (vtk_LIBS 
        ${vtk_LIBPATH}/libQVTK.so
        ${vtk_LIBPATH}/libvtkHybrid.so
        ${vtk_LIBPATH}/libvtkCommon.so
        ${vtk_LIBPATH}/libvtkRendering.so
        ${vtk_LIBPATH}/libvtkVolumeRendering.so
        ${vtk_LIBPATH}/libvtkWidgets.so)
   
    set (qt_LIBS
        ${BUILDEM_DIR}/lib/libQtCore.so
        ${BUILDEM_DIR}/lib/libQtGui.so
    )

    # Compile lib-dvid utils components
    include_directories(${LIBDVIDCPP_INCLUDE_DIRS})

    # Handle all sources and dependent code
    add_executable (dvid_viewer dvid_viewer.cpp)
    add_dependencies (dvid_viewer ${libdvidcpp_NAME} ${jsoncpp_NAME})
    
    #${BUILDEM_DIR}/lib/libdvidcpp.a
    message("!!! ${libdvid_LIBS}") 
    target_link_libraries (dvid_viewer  Model Gui ${vtk_LIBS} ${PYTHON_LIBRARY_FILE} ${boost_LIBS} ${qt_LIBS} ${LIBDVIDCPP_LIBRARIES} ${json_LIB} ${BUILDEM_LIB_DIR}/libpng.so ${BUILDEM_LIB_DIR}/libjpeg.so ${BUILDEM_LIB_DIR}/liblz4.so ${BUILDEM_LIB_DIR}/libcurl.so)
endif (NOT ${BUILDEM_DIR} STREQUAL "None")


