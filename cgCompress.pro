TEMPLATE = app
TARGET = cgCompress
INCLUDEPATH += .
CONFIG += console

# Input
HEADERS += src/Image.hpp
SOURCES += src/Image.cpp src/main.cpp

# C++11 support
QMAKE_CXXFLAGS += -std=c++11
