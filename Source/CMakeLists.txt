add_library (Core STATIC
	append_path.cpp
	AtExit.cpp
	BackOff.cpp
	Binary.cpp
	Binder.cpp
	BinderMemory.cpp
	BitmapOps.cpp
	ClassLibrary.cpp
	Console.cpp
	CoreModule.cpp
	CurrentDirContext.cpp
	deactivate_servant.cpp
	Event.cpp
	EventSync.cpp
	EventSyncTimeout.cpp
	ExecContext.cpp
	ExecDomain.cpp
	Executable.cpp
	FileAccessBuf.cpp
	FileAccessChar.cpp
	FileAccessCharProxy.cpp
	FileAccessDirect.cpp
	FileAccessDirectProxy.cpp
	FileDescriptors.cpp
	FileDescriptorsContext.cpp
	FileLockQueue.cpp
	FileLockRanges.cpp
	Heap.cpp
	HeapCustom.cpp
	HeapDirectory.cpp
	initterm.cpp
	LocaleImpl.cpp
	LockablePtr.cpp
	MemContext.cpp
	Module.cpp
	ObjectPool.cpp
	open_binary.cpp
	ProtDomain.cpp
	RandomGen.cpp
	Runnable.cpp
	Scheduler.cpp
	Security.cpp
	Signals.cpp
	Singleton.cpp
	SkipList.cpp
	Startup.cpp
	StartupProt.cpp
	StartupSys.cpp
	SyncContext.cpp
	SyncDomain.cpp
	Synchronized.cpp
	SysDomain.cpp
	SysManager.cpp
	Thread.cpp
	ThreadBackground.cpp
	Timeout.cpp
	TimerAsyncCall.cpp
	TimerEvent.cpp
	TLS.cpp
	virtual_copy.cpp
	WaitableRef.cpp
	WaitList.cpp
)

set (internal_idl
	${CMAKE_CURRENT_SOURCE_DIR}/CoreDomains.idl
)

add_subdirectory (ORB)
add_subdirectory (NameService)

nirvana_module_idl (Core IDL_FILES ${internal_idl} OPTIONS -no_proxy)

target_include_directories (Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions (Core PUBLIC NIRVANA_HOST_PLATFORM=${NIRVANA_HOST_PLATFORM})

if (${NIRVANA_HOST_TYPE} STREQUAL "Windows")
	target_include_directories (Core PUBLIC ${CMAKE_SOURCE_DIR}/Port/Windows)
	target_compile_options (Core PUBLIC -fms-extensions)
endif ()
