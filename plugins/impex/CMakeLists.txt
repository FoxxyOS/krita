project(kritafilters)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_definitions( -DCPU_32_BITS )
endif()

if(JPEG_FOUND AND HAVE_LCMS2)
	add_subdirectory(jpeg)
endif()

if(TIFF_FOUND)
	add_subdirectory(tiff)
endif()

if(PNG_FOUND)
    add_subdirectory(png)
    add_subdirectory(csv)
endif()

if(OPENEXR_FOUND)
    add_subdirectory(exr)
endif()

if(POPPLER_FOUND)
    add_subdirectory(pdf)
endif()

if(LIBRAW_FOUND)
    add_subdirectory(raw)
endif()

add_subdirectory(bmp)
add_subdirectory(ora)
add_subdirectory(ppm)
add_subdirectory(xcf)
add_subdirectory(psd)
add_subdirectory(odg)
add_subdirectory(qml)
add_subdirectory(tga)
add_subdirectory(heightmap)
add_subdirectory(brush)
add_subdirectory(spriter)
add_subdirectory(video)
