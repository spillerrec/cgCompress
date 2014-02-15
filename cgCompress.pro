TEMPLATE = app
TARGET = cgCompress
INCLUDEPATH += .
CONFIG += console

LIBS += -lz

# Input
HEADERS += src/Image.hpp src/Frame.hpp src/MultiImage.hpp src/Converter.hpp src/OraSaver.hpp src/FileUtils.hpp src/Format.hpp
SOURCES += src/Image.cpp src/Frame.cpp src/MultiImage.cpp src/Converter.cpp src/OraSaver.cpp src/FileUtils.cpp src/main.cpp

# minizip
SOURCES += src/minizip/ioapi.cpp src/minizip/zip.cpp

# C++11 support
QMAKE_CXXFLAGS += -std=c++11
