CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (Gui)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
message("${CMAKE_CURRENT_BINARY_DIR}")

set (SOURCES DVIDController.cpp DVIDPlaneView.cpp DVIDPlaneController.cpp DVIDUi.cpp)
#set (SOURCES DVIDController.cpp DVIDUi.cpp)

if (QT_BUILT)
    QT4_WRAP_CPP(QTHEADERS DVIDUi.h DVIDController.h DVIDPlaneController.h)
    QT4_WRAP_UI(QTFORMS dvid_viewer.ui)
    add_library (Gui SHARED ${SOURCES} ${QTFORMS} ${QTHEADERS})
else()
    add_library (Gui SHARED ${SOURCES})
endif()


add_dependencies (Gui ${boost_NAME} ${vtk_NAME} RescanQt4)
