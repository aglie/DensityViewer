cmake_minimum_required(VERSION 3.25)
project(pdfviewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type is set to Release" FORCE)
    add_definitions(-DCMAKE_BUILD_TYPE=Release)
endif ()

find_package(Qt5 COMPONENTS
        Core Gui Widgets
        REQUIRED)

add_executable(PDFViewer main.cpp
        densityviewer.cpp
        densityviewerwindow.cpp
        densitydata.cpp
        colormap.cpp)

target_include_directories(PDFViewer PUBLIC ${HDF5_INSTALL_INCLUDE_DIR})

target_link_libraries(PDFViewer
        Qt5::Core Qt5::Gui Qt5::Widgets
        )


if (DEVELOPMENT_BUILD)
    set(HDF5_PREFIX /Users/asimonov/ag/C++/DensityViewer/libs/hdf5-build)
    set(HDF5_INSTALL_LIB_DIR ${HDF5_PREFIX}/lib)
    set(HDF5_INSTALL_INCLUDE_DIR ${HDF5_PREFIX}/include)

    set(lib_hdf5_name libhdf5.a)
    set(lib_hdf5_name_cpp libhdf5_cpp.a)

    add_library(hdf5_local STATIC IMPORTED)
    set_target_properties(hdf5_local PROPERTIES IMPORTED_LOCATION ${HDF5_INSTALL_LIB_DIR}/${lib_hdf5_name})
    add_library(hdf5_local_cpp STATIC IMPORTED)
    set_target_properties(hdf5_local_cpp PROPERTIES IMPORTED_LOCATION ${HDF5_INSTALL_LIB_DIR}/${lib_hdf5_name_cpp})

    target_link_libraries(PDFViewer hdf5_local_cpp hdf5_local)
    target_include_directories(PDFViewer PUBLIC ${HDF5_INSTALL_INCLUDE_DIR})
else ()
    include(cmake/hdf5.cmake)

    DownloadHDF5For(PDFViewer)
endif ()

install(TARGETS PDFViewer
        CONFIGURATIONS Release
        RUNTIME DESTINATION Release/bin)
