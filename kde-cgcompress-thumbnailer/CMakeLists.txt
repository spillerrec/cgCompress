cmake_minimum_required(VERSION 3.2)
project(cgthumbnailer)

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

find_package(KF5 REQUIRED COMPONENTS CoreAddons Solid)

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.30")
find_package(SharedMimeInfo REQUIRED)

find_package(KF5KIO REQUIRED)
find_package(Qt5Gui REQUIRED)

#sources
set(cgcreator_SRCS cgcreator.cpp)
add_library(cgthumbnailer ${cgcreator_SRCS})


## includes and linking
target_link_libraries(cgthumbnailer Qt5::Gui KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM -larchive)

## install the plugin
install(TARGETS cgthumbnailer DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES cgcompressthumbnailer.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES cgcompress.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})



feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
