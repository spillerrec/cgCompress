TEMPLATE = app
TARGET = cgCompress
INCLUDEPATH += .
CONFIG += console

# Input
HEADERS += src/Image.hpp src/Frame.hpp src/MultiImage.hpp
SOURCES += src/Image.cpp src/Frame.cpp src/MultiImage.cpp src/main.cpp

# C++11 support
QMAKE_CXXFLAGS += -std=c++11
