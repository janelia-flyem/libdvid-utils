CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Gui)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

set (SOURCES DVIDController.cpp DVIDPlaneView.cpp DVIDPlaneController.cpp DVIDUi.cpp)

QT4_WRAP_CPP(QTHEADERS DVIDUi.h DVIDController.h DVIDPlaneController.h)
QT4_WRAP_UI(QTFORMS dvid_viewer.ui)

add_library (dvidviewer_gui SHARED ${SOURCES} ${QTFORMS} ${QTHEADERS})
target_link_libraries (dvidviewer_gui dvidviewer_model ${vtk_LIBS} ${boost_LIBS} ${qt_LIBS} ${libdvid_LIBS} ${json_LIB})

install (TARGETS dvidviewer_gui DESTINATION lib${LIB_SUFFIX})
