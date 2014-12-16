CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project (libdvid-utils)

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

    include (jsoncpp)
    include (libdvidcpp)
    include (libpng)

    set (boost_LIBS  ${BUILDEM_LIB_DIR}/libboost_thread.${BUILDEM_PLATFORM_DYLIB_EXTENSION}
                    ${BUILDEM_LIB_DIR}/libboost_system.${BUILDEM_PLATFORM_DYLIB_EXTENSION} )

    set (libdvid_LIBS ${LIBDVIDCPP_LIBRARIES} ${CPPNETLIB_LIBRARIES} ${BUILDEM_LIB_DIR}/libssl.so ${BUILDEM_LIB_DIR}/libcrypto.so ${BUILDEM_LIB_DIR}/libpng.so)
else ()
    find_package ( cppnetlib 0.11.0 REQUIRED )
    find_package ( libdvidcpp 0.1.0 REQUIRED )
    
    # ensure the libjsoncpp.so is symbolically linked somewhere your lib path
    set (json_LIB jsoncpp)
    set (libdvid_LIBS ${LIBDVIDCPP_LIBRARIES} ${CPPNETLIB_LIBRARIES} ssl crypto)
    
    set (boost_LIBS boost_thread boost_system) 

endif (NOT ${BUILDEM_DIR} STREQUAL "None")

# Compile lib-dvid utils components
include_directories(${LIBDVIDCPP_INCLUDE_DIRS})

# Handle all sources and dependent code
add_executable (dvid_load_sparse dvid_load_sparse.cpp)
add_dependencies (dvid_load_sparse ${jsoncpp_NAME} ${libdvidcpp_NAME} ${libpng_NAME})

target_link_libraries (dvid_load_sparse ${json_LIB} ${libdvid_LIBS} ${boost_LIBS})

