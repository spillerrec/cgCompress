cmake_minimum_required(VERSION 3.0)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)

set(SOURCES_MISC
	decoder/CgImage.cpp
	decoder/OraHandler.cpp
	formats/FormatWebP.cpp
	)

set(SOURCES_OTHERS
	minizip/ioapi.cpp
	minizip/zip.cpp
	)

set(SOURCES_MAIN
	images/Rgba.cpp
	Compression.cpp
	Converter.cpp
	CsvWriter.cpp
	FileSizeEval.cpp
	FileUtils.cpp
	Format.cpp
	Frame.cpp
	Image.cpp
	ImageOptim.cpp
	ImageSimilarities.cpp
	MultiImage.cpp
	main.cpp
	)
	

add_executable(cgCompress
	${SOURCES_MISC}
	${SOURCES_OTHERS}
	${SOURCES_MAIN}
	)

if(WIN32)
	set(LIBS_WIN -liconv)
endif()

target_link_libraries(cgCompress Qt5::Widgets Qt5::Concurrent  -larchive -lz -llz4 -llzma -lwebp -lpugixml)

# C++14
set_property(TARGET cgCompress PROPERTY CXX_STANDARD 14)
set_property(TARGET cgCompress PROPERTY CXX_STANDARD_REQUIRED ON)
