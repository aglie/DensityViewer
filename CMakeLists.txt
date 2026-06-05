cmake_minimum_required(VERSION 3.25)
project(pdfviewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type is set to Release" FORCE)
    add_definitions(-DCMAKE_BUILD_TYPE=Release)
endif ()

# Static build option
option(BUILD_STATIC "Build with static libraries" OFF)
if (BUILD_STATIC)
    set(CMAKE_TOTAL_STATIC ON)
    set(HDF5_USE_STATIC_LIBRARIES ON)
    
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(MINGW OR (UNIX AND NOT APPLE))
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -Wl,-Bstatic")
    endif()
endif()

find_package(Qt5 COMPONENTS Core Gui Widgets Test REQUIRED)

add_executable(PDFViewer MACOSX_BUNDLE main.cpp
        densityviewer.cpp
        densityviewerwindow.cpp
        densitydata.cpp
        colormap.cpp)

add_executable(PDFViewerTest densityviewertest.cpp
        densitydata.cpp
        densityviewer.cpp
        colormap.cpp)

# HDF5 Logic
if (DEVELOPMENT_BUILD)
    # Local developer paths
    set(HDF5_PREFIX /Users/asimonov/ag/C++/DensityViewer/libs/hdf5-build)
    set(HDF5_INSTALL_LIB_DIR ${HDF5_PREFIX}/lib)
    set(HDF5_INSTALL_INCLUDE_DIR ${HDF5_PREFIX}/include)

    set(lib_hdf5_name libhdf5.a)
    set(lib_hdf5_name_cpp libhdf5_cpp.a)

    add_library(hdf5_local STATIC IMPORTED)
    set_target_properties(hdf5_local PROPERTIES IMPORTED_LOCATION ${HDF5_INSTALL_LIB_DIR}/${lib_hdf5_name})
    add_library(hdf5_local_cpp STATIC IMPORTED)
    set_target_properties(hdf5_local_cpp PROPERTIES IMPORTED_LOCATION ${HDF5_INSTALL_LIB_DIR}/${lib_hdf5_name_cpp})

    target_link_libraries(PDFViewer PUBLIC hdf5_local_cpp hdf5_local)
    target_include_directories(PDFViewer PUBLIC ${HDF5_INSTALL_INCLUDE_DIR})

    target_link_libraries(PDFViewerTest PUBLIC hdf5_local_cpp hdf5_local)
    target_include_directories(PDFViewerTest PUBLIC ${HDF5_INSTALL_INCLUDE_DIR})
else ()
    # Generic build / CI logic
    include(cmake/hdf5.cmake)
    
    DownloadHDF5For(PDFViewer)
    
    # Ensure Test target has the same dependencies
    add_dependencies(PDFViewerTest download_hdf5)
    target_include_directories(PDFViewerTest PUBLIC ${HDF5_INSTALL_INCLUDE_DIR})
    target_link_libraries(PDFViewerTest PUBLIC hdf5_local hdf5_local_cpp bitshuffle)
endif ()

target_link_libraries(PDFViewer
        PUBLIC
        Qt5::Core Qt5::Gui Qt5::Widgets
        )

target_link_libraries(PDFViewerTest
        PUBLIC
        Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Test
        )

# For Linux/MinGW static builds, we might need to ensure pthread is linked dynamically at the end
if(BUILD_STATIC AND NOT APPLE AND NOT MSVC)
    set(CMAKE_THREAD_LIBS_INIT "-Wl,-Bdynamic -lpthread -Wl,-Bstatic")
endif()

install(TARGETS PDFViewer
        CONFIGURATIONS Release
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION bin)
