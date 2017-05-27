TEMPLATE = app
TARGET = parallel-decode-test
CONFIG += console
QT += concurrent

#LIBS += -lz

# Input
SOURCES += main.cpp

# C++11 support
CONFIG += c++14

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