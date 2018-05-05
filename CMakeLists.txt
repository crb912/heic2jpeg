cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

set(HEIF2JPEG_EXE heic2jpeg)
set(HEIF2JPEG_SRCS heif2jpeg.cpp)

INCLUDE_DIRECTORIES(./heif/srcs/api/reader) 
INCLUDE_DIRECTORIES(./heif/srcs/api/common) 
LINK_DIRECTORIES(./heif/build/lib)
add_executable(${HEIF2JPEG_EXE} ${HEIF2JPEG_SRCS})

target_link_libraries(${HEIF2JPEG_EXE} heif_static)
target_link_libraries(${HEIF2JPEG_EXE} avcodec avutil swscale)


if(UNIX)
  INCLUDE_DIRECTORIES(/usr/include/glib-2.0)                         # glib-object.h
  INCLUDE_DIRECTORIES(/usr/lib/x86_64-linux-gnu/glib-2.0/include/)   # glibconfig.h
  target_link_libraries(${HEIF2JPEG_EXE} vips-cpp vips gobject-2.0 glib-2.0 jpeg png)
else(MSYS OR MINGW)
  INCLUDE_DIRECTORIES(C:/msys64/mingw64/include/glib-2.0)
  target_link_libraries(${HEIF2JPEG_EXE} vips-cpp vips gobject-2.0 glib-2.0 gmodule-2.0 intl expat jpeg png)
endif()


set_property(TARGET ${HEIF2JPEG_EXE} PROPERTY CXX_STANDARD 11)
