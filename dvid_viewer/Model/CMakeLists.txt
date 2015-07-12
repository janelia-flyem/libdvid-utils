CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Model)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_LINK_FLAGS "-O3")
SET(CMAKE_DEBUG_POSTFIX "-g")

set (SOURCES Model.cpp)

add_library (dvidviewer_model SHARED ${SOURCES})

target_link_libraries (dvidviewer_model ${vtk_LIBS} ${boost_LIBS} ${libdvid_LIBS} ${json_LIB} ${qt_LIBS})

install (TARGETS dvidviewer_model DESTINATION lib${LIB_SUFFIX})
