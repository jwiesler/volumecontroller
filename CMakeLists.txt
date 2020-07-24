cmake_minimum_required(VERSION 3.5)

project(VolumeController LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(Qt5 COMPONENTS Widgets LinguistTools WinExtras REQUIRED)

set(TS_FILES VolumeController_de_DE.ts)

add_executable(VolumeController
    WIN32
    src/volumecontroller/runguard.cpp
    src/volumecontroller/runguard.h
    src/volumecontroller/main.cpp
    src/volumecontroller/comptr.h
    src/volumecontroller/hresulterrors.h
    src/volumecontroller/info/programminformation.cpp
    src/volumecontroller/info/programminformation.h
    src/volumecontroller/info/processdata.h
    src/volumecontroller/info/processdata.cpp
    src/volumecontroller/audio/audiosessions.h
    src/volumecontroller/audio/audiosessions.cpp
    src/volumecontroller/audio/audiodevicemanager.h
    src/volumecontroller/audio/audiodevicemanager.cpp
    src/volumecontroller/ui/gridlayout.cpp
    src/volumecontroller/ui/gridlayout.h
    src/volumecontroller/ui/volumecontrollist.cpp
    src/volumecontroller/ui/volumecontrollist.h
    src/volumecontroller/ui/animations.cpp
    src/volumecontroller/ui/animations.h
    src/volumecontroller/ui/volumeicons.cpp
    src/volumecontroller/ui/volumeicons.h
    src/volumecontroller/ui/devicevolumecontroller.cpp
    src/volumecontroller/ui/devicevolumecontroller.h
    src/volumecontroller/ui/volumecontroller.cpp
    src/volumecontroller/ui/volumecontroller.h
    src/volumecontroller/ui/volumecontroller.ui
    src/volumecontroller/ui/volumelistitem.h
    src/volumecontroller/ui/volumelistitem.cpp
    src/volumecontroller/collections.h
    src/volumecontroller/joiner.h
    src/volumecontroller/ui/theme.h
    src/volumecontroller/ui/customstyle.cpp
    src/volumecontroller/ui/customstyle.h
    ${TS_FILES}
)

target_include_directories(VolumeController PUBLIC src)
target_compile_definitions(VolumeController PUBLIC ROTATE_LOG_FILE)
target_link_libraries(VolumeController PRIVATE Qt5::Widgets Qt5::WinExtras Version.lib)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
