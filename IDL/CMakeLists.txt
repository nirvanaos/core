add_subdirectory (ORB)

set (core_idl ${core_idl}
	${CMAKE_CURRENT_SOURCE_DIR}/CoreDomains.idl
	PARENT_SCOPE
)
