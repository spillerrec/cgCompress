TEMPLATE = app
TARGET = cgCompress
INCLUDEPATH += .
CONFIG += console
QT += concurrent

LIBS += -larchive -lz -llz4 -llzma -lwebp -lpugixml
win32{
	LIBS += -liconv
}

# Input
HEADERS += src/images/ImageView.hpp src/images/Image.hpp src/images/Rgba.hpp src/images/Blend.hpp
HEADERS += src/decoder/OraHandler.hpp
SOURCES += src/decoder/OraHandler.cpp
HEADERS += src/Compression.hpp src/CsvWriter.hpp src/Image.hpp src/Frame.hpp src/ImageSimilarities.hpp src/MultiImage.hpp src/Converter.hpp src/OraSaver.hpp src/FileUtils.hpp src/Format.hpp src/FileSizeEval.hpp src/ImageOptim.hpp src/formats/FormatWebP.hpp src/ProgressBar.hpp
SOURCES += src/Compression.cpp src/CsvWriter.cpp src/Image.cpp src/Frame.cpp src/ImageSimilarities.cpp src/MultiImage.cpp src/Converter.cpp src/OraSaver.cpp src/FileUtils.cpp src/Format.cpp src/FileSizeEval.cpp src/ImageOptim.cpp src/formats/FormatWebP.cpp src/main.cpp

# minizip
SOURCES += src/minizip/ioapi.cpp src/minizip/zip.cpp

# C++11 support
CONFIG += c++14


# Generate both debug and release on Linux
CONFIG += debug_and_release

#QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_DEBUG += -O3

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