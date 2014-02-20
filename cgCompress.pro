TEMPLATE = app
TARGET = cgCompress
INCLUDEPATH += .
CONFIG += console

LIBS += -lz

# Input
HEADERS += src/Image.hpp src/Frame.hpp src/MultiImage.hpp src/Converter.hpp src/OraSaver.hpp src/FileUtils.hpp src/Format.hpp
SOURCES += src/Image.cpp src/Frame.cpp src/MultiImage.cpp src/Converter.cpp src/OraSaver.cpp src/FileUtils.cpp src/Format.cpp src/main.cpp

# minizip
SOURCES += src/minizip/ioapi.cpp src/minizip/zip.cpp

# C++11 support
QMAKE_CXXFLAGS += -std=c++11


# Generate both debug and release on Linux
CONFIG += debug_and_release

# Position of binaries and build files
Release:DESTDIR = release
Release:UI_DIR = release/.ui
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.qrc

Debug:DESTDIR = debug
Debug:UI_DIR = debug/.ui
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.qrc