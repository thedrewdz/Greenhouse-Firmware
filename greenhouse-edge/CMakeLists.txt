cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(greenhouse-edge)
idf_component_get_property(bt_component bt COMPONENT_LIB)
target_compile_options(
	${bt_component}
	PRIVATE
	"$<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>"
)
