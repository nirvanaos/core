add_library (Core STATIC)

target_include_directories (Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_definitions (Core PUBLIC NIRVANA_HOST_PLATFORM=${NIRVANA_HOST_PLATFORM})

if (${NIRVANA_HOST_TYPE} STREQUAL "Windows")
	target_include_directories (Core PUBLIC ${CMAKE_SOURCE_DIR}/Port/Windows)
	target_compile_options (Core PUBLIC -fms-extensions)
endif ()

add_subdirectory (Source)

find_package(boost_preprocessor CONFIG REQUIRED)
target_link_libraries(Core PRIVATE Boost::preprocessor)

set (core_idl)
add_subdirectory (IDL)
nirvana_module_idl (Core IDL_FILES ${core_idl} OPTIONS -no_proxy)
nirvana_module_idl (Core IDL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/PacMan/factory.idl OPTIONS -client)
