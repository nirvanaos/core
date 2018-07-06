// Testing Windows API memory management functions

#include <windows.h>
#include <gtest/gtest.h>

#define PAGE_SIZE 4096
#define ALLOCATION_GRANULARITY (16 * PAGE_SIZE)

namespace unittests {

BOOL handles_equal (HANDLE h0, HANDLE h1)
{
	return CompareObjectHandles (h0, h1);
}

class TestAPI :
	public ::testing::Test
{
protected:
	TestAPI ()
	{}

	virtual ~TestAPI ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

	// Create new mapping
	static HANDLE new_mapping ()
	{
		return CreateFileMappingW (0, 0, PAGE_EXECUTE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
	}

};

TEST_F (TestAPI, MappingHandle)
{
	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	HANDLE mh1;
	HANDLE process = GetCurrentProcess ();
	EXPECT_TRUE (DuplicateHandle (process, mh, process, &mh1, 0, FALSE, DUPLICATE_SAME_ACCESS));
	EXPECT_TRUE (handles_equal (mh, mh1));
	EXPECT_TRUE (CloseHandle (mh1));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST_F (TestAPI, Allocate)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	EXPECT_EQ (si.dwAllocationGranularity, ALLOCATION_GRANULARITY);
	EXPECT_EQ (((size_t)si.lpMaximumApplicationAddress + 1) % ALLOCATION_GRANULARITY, 0);

	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	char* p = (char*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (p);

	// Commit 2 pages.
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE * 2, MEM_COMMIT, PAGE_READWRITE));

	EXPECT_FALSE (VirtualAlloc (p, ALLOCATION_GRANULARITY, MEM_RESERVE, PAGE_READWRITE));

	DWORD err = GetLastError ();
	EXPECT_EQ (err, ERROR_INVALID_ADDRESS);

	// Write to 2 pages.
	EXPECT_NO_THROW (p [0] = 1);
	EXPECT_NO_THROW (p [PAGE_SIZE] = 2);

	// Decommit first page. We can't use VirtualFree with mapped memory.

	DWORD old;
	EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_NOACCESS, &old));
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_RESET, PAGE_NOACCESS));

	char x = 0;
	EXPECT_ANY_THROW (x = p [0]);
	EXPECT_EQ (x, 0); // Prevent optimization.

	// Recommit first page
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	EXPECT_NO_THROW (x = p [0]);

	EXPECT_TRUE (UnmapViewOfFile (p));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST_F (TestAPI, Sharing)
{
	MEMORY_BASIC_INFORMATION mbi0, mbi1;

	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	char* p = (char*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (p);
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));

	EXPECT_TRUE (VirtualQuery (p, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (p + PAGE_SIZE, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.AllocationBase, p);
	EXPECT_EQ (mbi1.AllocationBase, p);
	EXPECT_EQ (mbi0.Type, MEM_MAPPED);
	EXPECT_EQ (mbi1.Type, MEM_MAPPED);
	EXPECT_EQ (mbi0.AllocationProtect, PAGE_READWRITE);
	EXPECT_EQ (mbi1.AllocationProtect, PAGE_READWRITE);

	EXPECT_EQ (mbi0.State, MEM_COMMIT);
	EXPECT_EQ (mbi1.State, MEM_RESERVE);
	EXPECT_EQ (mbi0.Protect, PAGE_READWRITE);
	EXPECT_EQ (mbi1.Protect, 0);

	strcpy (p, "test");

	HANDLE process = GetCurrentProcess ();

	char* copies [10];
	HANDLE handles [10];

	memset (copies, 0, sizeof (copies));
	memset (handles, 0, sizeof (copies));

	DWORD old;
	EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_WRITECOPY, &old));

	EXPECT_TRUE (VirtualQuery (p, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (p + PAGE_SIZE, &mbi1, sizeof (mbi1)));

	for (int i = 0; i < _countof (copies); ++i) {
		HANDLE mh1;
		ASSERT_TRUE (DuplicateHandle (process, mh, process, &mh1, 0, FALSE, DUPLICATE_SAME_ACCESS));
		handles [i] = mh1;
		char* p = (char*)MapViewOfFile (mh1, FILE_MAP_READ, 0, 0, ALLOCATION_GRANULARITY);
		EXPECT_TRUE (p);
		copies [i] = p;
		if (p) {
//			EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_READONLY, &old));
			EXPECT_STREQ (p, "test");
			EXPECT_ANY_THROW (strcpy (p, "test1"));
		}
	}

	for (int i = 0; i < _countof (copies); ++i) {
		char* p = copies [i];
		copies [i] = 0;
		if (p)
			EXPECT_TRUE (UnmapViewOfFile (p));
		HANDLE h = handles [i];
		handles [i] = 0;
		if (h)
			EXPECT_TRUE (CloseHandle (h));
	}

	for (int i = 0; i < _countof (copies); ++i) {
		HANDLE mh1;
		ASSERT_TRUE (DuplicateHandle (process, mh, process, &mh1, 0, FALSE, DUPLICATE_SAME_ACCESS));
		handles [i] = mh1;
		char* p = (char*)MapViewOfFile (mh1, FILE_MAP_COPY, 0, 0, ALLOCATION_GRANULARITY);
		EXPECT_TRUE (p);
		copies [i] = p;
		if (p) {
//			EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_WRITECOPY, &old));
			EXPECT_STREQ (p, "test");

			char buf [16];
			_itoa (i, buf, 10);
			EXPECT_NO_THROW (strcpy (p + 4, buf));
		}
	}

	for (int i = 0; i < _countof (copies); ++i) {
		char* p = copies [i];
		char buf [16] = "test";
		_itoa (i, buf + 4, 10);
		EXPECT_STREQ (p, buf);
	}

	for (int i = 0; i < _countof (copies); ++i) {
		char* p = copies [i];
		copies [i] = 0;
		if (p)
			EXPECT_TRUE (UnmapViewOfFile (p));
		HANDLE h = handles [i];
		handles [i] = 0;
		if (h)
			EXPECT_TRUE (CloseHandle (h));
	}

	EXPECT_STREQ (p, "test");
	EXPECT_TRUE (UnmapViewOfFile (p));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST_F (TestAPI, Protection)
{
	MEMORY_BASIC_INFORMATION mbi0, mbi1;
	DWORD old;

	// We use "execute" protection to distinct mapped pages from write-copied pages.
	// Page states:
	// 0 - Not committed.
	// PAGE_NOACCESS - Decommitted (mapped, but not accessible).
	// PAGE_EXECUTE_READWRITE: The page is mapped and never was shared.
	// PAGE_WRITECOPY: The page is mapped and was shared.
	// PAGE_READWRITE: The page is write-copyed (private, disconnected from mapping).
	// PAGE_EXECUTE_READONLY: The read-only mapped page never was shared.
	// PAGE_EXECUTE: The read-only mapped page was shared.
	// PAGE_READONLY: The page is not mapped. Page was write-copyed, than access was changed from PAGE_READWRITE to PAGE_READONLY.
	// Note: "Page was shared" means that page has been shared at least once. Currently, page may be still shared or already not.

	// Page state changes.
	// Prepare to share:
	//   0 (not committed)->PAGE_NOACCESS (commit+decommit)
	//   PAGE_EXECUTE_READWRITE->PAGE_WRITECOPY
	//   PAGE_EXECUTE_READONLY, PAGE_WRITECOPY, PAGE_EXECUTE, PAGE_NOACCESS unchanged.
	//   PAGE_READWRITE, PAGE_READONLY - we need to remap the block.
	// Remap:
	//   PAGE_READWRITE->PAGE_EXECUTE_READWRITE
	//   PAGE_READONLY->PAGE_EXECUTE_READONLY
	// Write-protect:
	//   PAGE_EXECUTE_READWRITE<->PAGE_EXECUTE_READONLY
	//	 PAGE_WRITECOPY<->PAGE_EXECUTE
	//   PAGE_READWRITE<->PAGE_READONLY

	// Create source block
	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	BYTE* src = (BYTE*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (src);

	// Commit 2 pages
	EXPECT_TRUE (VirtualAlloc (src, PAGE_SIZE * 2, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	src [0] = 1;
	src [PAGE_SIZE] = 2;

	EXPECT_TRUE (VirtualQuery (src, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (src + PAGE_SIZE * 2, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.AllocationBase, src);
	EXPECT_EQ (mbi1.AllocationBase, src);
	EXPECT_EQ (mbi0.Type, MEM_MAPPED);
	EXPECT_EQ (mbi1.Type, MEM_MAPPED);
	EXPECT_EQ (mbi0.AllocationProtect, PAGE_EXECUTE_READWRITE);
	EXPECT_EQ (mbi1.AllocationProtect, PAGE_EXECUTE_READWRITE);

	EXPECT_EQ (mbi0.State, MEM_COMMIT);
	EXPECT_EQ (mbi1.State, MEM_RESERVE);
	EXPECT_EQ (mbi0.Protect, PAGE_EXECUTE_READWRITE);
	EXPECT_EQ (mbi1.Protect, 0);

	// Virtual copy first page to target block
	
	// 1. Change committed data protection to write-copy
	EXPECT_TRUE (VirtualProtect (src, PAGE_SIZE, PAGE_WRITECOPY, &old));

	// 2. Map to destination
	BYTE* dst = (BYTE*)MapViewOfFile (mh, FILE_MAP_COPY, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (dst);

	// 3. Protect non-copyed data
	EXPECT_TRUE (VirtualProtect (dst + PAGE_SIZE, PAGE_SIZE, PAGE_NOACCESS, &old));

	EXPECT_TRUE (VirtualQuery (dst, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (dst + PAGE_SIZE, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.AllocationBase, dst);
	EXPECT_EQ (mbi1.AllocationBase, dst);
	EXPECT_EQ (mbi0.Type, MEM_MAPPED);
	EXPECT_EQ (mbi1.Type, MEM_MAPPED);
	EXPECT_EQ (mbi0.AllocationProtect, PAGE_WRITECOPY);
	EXPECT_EQ (mbi1.AllocationProtect, PAGE_WRITECOPY);

	EXPECT_EQ (mbi0.State, MEM_COMMIT);
	EXPECT_EQ (mbi1.State, MEM_COMMIT);
	EXPECT_EQ (mbi0.Protect, PAGE_WRITECOPY);
	EXPECT_EQ (mbi1.Protect, PAGE_NOACCESS);

	EXPECT_EQ (dst [0], src [0]);
	// Write to destination
	dst [0] = 3;
	EXPECT_EQ (src [0], 1);	// Source not changed

	EXPECT_TRUE (VirtualQuery (dst, &mbi0, sizeof (mbi0)));
	EXPECT_EQ (mbi0.Protect, PAGE_READWRITE); // Private write-copyed

	// Copy source again
	//
	// PAGE_REVERT_TO_FILE_MAP can be combined with other protection
	// values to specify to VirtualProtect that the argument range
	// should be reverted to point back to the backing file.  This
	// means the contents of any private (copy on write) pages in the
	// range will be discarded.  Any reverted pages that were locked
	// into the working set are unlocked as well.
	//
	EXPECT_TRUE (VirtualProtect (dst, PAGE_SIZE, PAGE_WRITECOPY | PAGE_REVERT_TO_FILE_MAP, &old));
	EXPECT_EQ (dst [0], src [0]);

	// Write to source (copy-on-write)
	src [1] = 4;

	// Check source state
	EXPECT_TRUE (VirtualQuery (src, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (src + PAGE_SIZE, &mbi1, sizeof (mbi1)));

	EXPECT_EQ (mbi0.Protect, PAGE_READWRITE); // Private write-copyed
	EXPECT_EQ (mbi1.Protect, PAGE_EXECUTE_READWRITE); // Shared read-write

	EXPECT_EQ (dst [0], 1);	// Destination not changed

	BYTE x = 0;

	// Decommit source page
	EXPECT_TRUE (VirtualProtect (src, PAGE_SIZE, PAGE_NOACCESS | PAGE_REVERT_TO_FILE_MAP, &old));
	EXPECT_ANY_THROW (x = src [0]);
	EXPECT_EQ (x, 0); // Prevent optimization.

	EXPECT_EQ (dst [0], 1);	// Destination not changed

	// Decommit second source page
	EXPECT_TRUE (VirtualProtect (src + PAGE_SIZE, PAGE_SIZE, PAGE_NOACCESS, &old));
	// For not shared pages call VirtualAlloc with MEM_RESET to inform system that page content is not more interested.
	EXPECT_TRUE (VirtualAlloc (src + PAGE_SIZE, PAGE_SIZE, MEM_RESET, PAGE_NOACCESS));
	EXPECT_ANY_THROW (x = src [PAGE_SIZE]);
	EXPECT_EQ (x, 0); // Prevent optimization.

	EXPECT_TRUE (UnmapViewOfFile (dst));
	EXPECT_TRUE (UnmapViewOfFile (src));
	EXPECT_TRUE (CloseHandle (mh));
}

// SparseMapping on 64 bit system over 10 times faster than SharedMapping
TEST_F (TestAPI, SparseMapping)
{
	WCHAR dir [MAX_PATH + 1];
	ASSERT_TRUE (GetCurrentDirectoryW (_countof (dir), dir));

	HANDLE file = CreateFileW (L"mapping.tmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
														 CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
	ASSERT_NE (file, INVALID_HANDLE_VALUE);

	{
		FILE_SET_SPARSE_BUFFER sb;
		sb.SetSparse = TRUE;
		DWORD cb;
		EXPECT_TRUE (DeviceIoControl (file, FSCTL_SET_SPARSE, &sb, sizeof (sb), 0, 0, &cb, 0));
	}

	HANDLE mapping;
	HANDLE* table;
	{
		SYSTEM_INFO si;
		GetSystemInfo (&si);
		size_t size = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY * sizeof (HANDLE);

		FILE_ZERO_DATA_INFORMATION zdi;
		zdi.FileOffset.QuadPart = 0;
		zdi.BeyondFinalZero.QuadPart = size;
		DWORD cb;
		EXPECT_TRUE (DeviceIoControl (file, FSCTL_SET_ZERO_DATA, &zdi, sizeof (zdi), 0, 0, &cb, 0));
		mapping = CreateFileMapping (file, 0, PAGE_READWRITE, zdi.BeyondFinalZero.HighPart, zdi.BeyondFinalZero.LowPart, 0);
		EXPECT_TRUE (mapping);

		table = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		EXPECT_TRUE (table);
	}

	HANDLE sfile = CreateFileW (L"mapping.tmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
															OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
	EXPECT_NE (sfile, INVALID_HANDLE_VALUE);

	LARGE_INTEGER fsize;
	EXPECT_TRUE (GetFileSizeEx (sfile, &fsize));

	HANDLE smapping = CreateFileMapping (sfile, 0, PAGE_READWRITE, fsize.HighPart, fsize.LowPart, 0);
	EXPECT_TRUE (smapping);
	HANDLE* stable = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	table [ALLOCATION_GRANULARITY / sizeof (HANDLE)] = INVALID_HANDLE_VALUE;
	EXPECT_EQ (stable [ALLOCATION_GRANULARITY / sizeof (HANDLE)], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (stable));
	EXPECT_TRUE (CloseHandle (smapping));
	EXPECT_TRUE (CloseHandle (sfile));

	EXPECT_TRUE (UnmapViewOfFile (table));
	EXPECT_TRUE (CloseHandle (mapping));
	EXPECT_TRUE (CloseHandle (file));
}

// Best directory implementation for 32-bit system.
TEST_F (TestAPI, SharedMapping)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	LARGE_INTEGER size;
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY * sizeof (HANDLE);

	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, L"NirvanaMapping");
	ASSERT_TRUE (mapping);
	HANDLE* table = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, (SIZE_T)size.QuadPart);
	EXPECT_TRUE (table);

	HANDLE smapping = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, L"NirvanaMapping");
	ASSERT_TRUE (smapping);

	HANDLE* stable = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, 0, 0, (SIZE_T)size.QuadPart);
	EXPECT_TRUE (stable);

	EXPECT_TRUE (VirtualAlloc (table + ALLOCATION_GRANULARITY / sizeof (HANDLE), PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	table [ALLOCATION_GRANULARITY / sizeof (HANDLE)] = INVALID_HANDLE_VALUE;

	EXPECT_TRUE (VirtualAlloc (stable + ALLOCATION_GRANULARITY / sizeof (HANDLE), PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	EXPECT_EQ (stable [ALLOCATION_GRANULARITY / sizeof (HANDLE)], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (stable));
	EXPECT_TRUE (CloseHandle (smapping));

	EXPECT_TRUE (UnmapViewOfFile (table));
	EXPECT_TRUE (CloseHandle (mapping));
}

// Best directory implementation for 64-bit system.
TEST_F (TestAPI, SharedMapping2)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	LARGE_INTEGER size;
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY * sizeof (HANDLE);

	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, L"NirvanaMapping");
	ASSERT_TRUE (mapping);

	static const size_t SECOND_LEVEL_SIZE = ALLOCATION_GRANULARITY / sizeof (HANDLE);

	size_t root_size = ((size_t)size.QuadPart + ALLOCATION_GRANULARITY - 1) / ALLOCATION_GRANULARITY;

	HANDLE** root_dir = (HANDLE**)VirtualAlloc (0, root_size, MEM_RESERVE, PAGE_READWRITE);
	ASSERT_TRUE (root_dir);

	size_t idx = ALLOCATION_GRANULARITY / sizeof (HANDLE);
	size_t i1 = idx / SECOND_LEVEL_SIZE;
	size_t i2 = idx % SECOND_LEVEL_SIZE;

	ASSERT_TRUE (VirtualAlloc (root_dir + i1, sizeof (HANDLE**), MEM_COMMIT, PAGE_READWRITE));

	LARGE_INTEGER offset;
	offset.QuadPart = ALLOCATION_GRANULARITY * i1;
	EXPECT_TRUE (root_dir [i1] = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY));
	ASSERT_TRUE (VirtualAlloc (root_dir[i1] + i2, sizeof (HANDLE), MEM_COMMIT, PAGE_READWRITE));

	HANDLE smapping = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, L"NirvanaMapping");
	ASSERT_TRUE (smapping);

	HANDLE** sroot_dir = (HANDLE**)VirtualAlloc (0, root_size, MEM_RESERVE, PAGE_READWRITE);
	ASSERT_TRUE (sroot_dir);

	ASSERT_TRUE (VirtualAlloc (sroot_dir + i1, sizeof (HANDLE**), MEM_COMMIT, PAGE_READWRITE));

	EXPECT_TRUE (sroot_dir [i1] = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY));
	ASSERT_TRUE (VirtualAlloc (sroot_dir [i1] + i2, sizeof (HANDLE), MEM_COMMIT, PAGE_READWRITE));

	root_dir [i1][i2] = INVALID_HANDLE_VALUE;
	EXPECT_EQ (sroot_dir [i1][i2], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (sroot_dir [i1]));
	EXPECT_TRUE (CloseHandle (smapping));

	EXPECT_TRUE (UnmapViewOfFile (root_dir [i1]));
	EXPECT_TRUE (CloseHandle (mapping));
}

TEST_F (TestAPI, Commit)
{
	// Create source block
	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	BYTE* src = (BYTE*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (src);

	// Commit 2 pages overlapped
	EXPECT_TRUE (VirtualAlloc (src, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READ));
	EXPECT_TRUE (VirtualAlloc (src + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE));

	MEMORY_BASIC_INFORMATION mbi0, mbi1;
	EXPECT_TRUE (VirtualQuery (src, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (src + PAGE_SIZE, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.Protect, PAGE_EXECUTE_READ);
	EXPECT_EQ (mbi1.Protect, PAGE_EXECUTE_READWRITE);

	EXPECT_TRUE (UnmapViewOfFile (src));
	EXPECT_TRUE (CloseHandle (mh));
}

PTOP_LEVEL_EXCEPTION_FILTER g_exception_filter;

class MyException
{
public:
	MyException ()
	{}

};

}
