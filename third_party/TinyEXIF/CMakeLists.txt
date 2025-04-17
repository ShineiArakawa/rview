project(TinyEXIF)

add_library(TinyEXIFstatic STATIC TinyEXIF.cpp TinyEXIF.h)

target_link_libraries(TinyEXIFstatic tinyxml2::tinyxml2)
set_target_properties(
	TinyEXIFstatic
	PROPERTIES
	OUTPUT_NAME TinyEXIF
	VERSION "${GENERIC_LIB_VERSION}"
	SOVERSION "${GENERIC_LIB_SOVERSION}"
)

target_include_directories(
	TinyEXIFstatic
	PUBLIC 
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

if(MSVC)
	target_compile_definitions(TinyEXIFstatic PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()
