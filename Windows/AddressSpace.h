// Nirvana project.
// Windows implementation.
// Protection domain (process) address space.

#ifndef NIRVANA_WINDOWS_ADDRESSSPACE_H_
#define NIRVANA_WINDOWS_ADDRESSSPACE_H_

#include "Win32.h"

namespace Nirvana {
namespace Windows {

///	<summary>
///	Page states for mapped block.
///	</summary>
///	<remarks>
///	<para>
///	We use "execute" protection to distinct mapped pages from write-copied pages.
///	Page states:
///	<list>
///		<item><term><c>0</c></term><description>Page not committed (entire block never was shared).</description></item>
///		<item><term><c>PAGE_NOACCESS</c></term><description>Decommitted.</description></item>
///		<item><term><c>PAGE_EXECUTE_READWRITE</c></term><description>The page is mapped and never was shared.</description></item>
///		<item><term><c>PAGE_WRITECOPY</c></term><description>The page is mapped and was shared.</description></item>
///		<item><term><c>PAGE_READWRITE</c></term><description>The page is write-copyed (private, disconnected from mapping).</description></item>
///		<item><term><c>PAGE_EXECUTE_READ</c></term><description>The read-only mapped page never was shared.</description></item>
///		<item><term><c>PAGE_EXECUTE</c></term><description>The read-only mapped page was shared.</description></item>
///		<item><term><c>PAGE_READONLY</c></term><description>The page is not mapped. Page was write-copyed, than access was changed from PAGE_READWRITE to PAGE_READONLY.</description></item>
///	</list>
///	Note: "Page was shared" means that page has been shared at least once. Currently, page may be still shared or already not.
///	</para><para>
///	Page state changes.
///	<list>
///		<item>
///		Prepare to share:
///		<list>
///			<item><term><c>0</c> (not committed)-><c>PAGE_NOACCESS</c></term><description>Commit + decommit.</description></item>
///			<item><term><c>PAGE_EXECUTE_READWRITE</c>-><c>PAGE_WRITECOPY</c></term></item>
///			<item><term><c>PAGE_EXECUTE_READONLY</c>, <c>PAGE_WRITECOPY</c>, <c>PAGE_EXECUTE</c>, <c>PAGE_NOACCESS</c></term><description>Unchanged.</description></item>
///			<item><term><c>PAGE_READWRITE</c>, <c>PAGE_READONLY</c></term><description>We need to remap the block.</description></item>
///		</list>
///		</item><item>
///		Remap:
///		<list>
///			<item><term><c>PAGE_READWRITE</c>-><c>PAGE_EXECUTE_READWRITE</c></term>
///			<item><term><c>PAGE_READONLY</c>-><c>PAGE_EXECUTE_READONLY</c></term>
///		</list>
///		</item><item>
///		Write-protection:
///		<list>
///			<item><term><c>PAGE_EXECUTE_READWRITE</c><-><c>PAGE_EXECUTE_READONLY</c></term>
///			<item><term><c>PAGE_WRITECOPY</c><-><c>PAGE_EXECUTE</c></term>
///			<item><term><c>PAGE_READWRITE</c><-><c>PAGE_READONLY</c></term>
///		</list>
///		</item>
///	</list>
///	</para>
///	</remarks>
class PageState
{
public:
	enum
	{
		/// <summary>Page not committed (entire block never was shared).</summary>
		NOT_COMMITTED = 0,
		/// <summary>Decommitted.</summary>
		DECOMMITTED = PAGE_NOACCESS,
		/// <summary>The page is mapped and never was shared.</summary>
		RW_MAPPED_PRIVATE = PAGE_EXECUTE_READWRITE,
		/// <summary>The page is mapped and was shared.</summary>
		RW_MAPPED_SHARED = PAGE_WRITECOPY,
		/// <summary>The page is write-copyed (private, disconnected from mapping).</summary>
		RW_UNMAPPED = PAGE_READWRITE,
		/// <summary>The read-only mapped page never was shared.</summary>
		RO_MAPPED_PRIVATE = PAGE_EXECUTE_READ,
		/// <summary>The read-only mapped page was shared.</summary>
		RO_MAPPED_SHARED = PAGE_EXECUTE,
		/// <summary>The page is not mapped. Page was write-copyed, than access was changed from <c>PAGE_READWRITE</c> to <c>PAGE_READONLY</c>.</summary>
		RO_UNMAPPED = PAGE_READONLY,

		// Page states for reserved block.
		RW_RESERVED = PAGE_READWRITE,
		RO_RESERVED = PAGE_READONLY,

		// Page state masks.
		MASK_RW = PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_READWRITE,
		MASK_RO = PAGE_EXECUTE_READ | PAGE_EXECUTE | PAGE_READONLY,
		MASK_ACCESS = MASK_RW | MASK_RO,
		MASK_UNMAPPED = RW_UNMAPPED | RO_UNMAPPED,
		MASK_MAY_BE_SHARED = RW_MAPPED_SHARED | RO_MAPPED_SHARED | MASK_UNMAPPED | DECOMMITTED
	};
};

/// <summary>
/// Logical address space of some Windows process.
/// </summary>
class AddressSpace
{
public:
	/// <summary>
	/// Initializes this instance for current process.
	/// </summary>
	void initialize ()
	{
		initialize (GetCurrentProcessId (), GetCurrentProcess ());
	}

	/// <summary>
	/// Terminates this instance.
	/// </summary>
	void terminate ();

	bool is_current_process () const
	{
		return GetCurrentProcess () == m_process;
	}

	void* reserve (SIZE_T size, LONG flags, void* dst = 0);
	void release (void* dst, SIZE_T size);

	// Quick methods for block sizes <= ALLOCATION_GRANULARITY
	void* copy (HANDLE src, SIZE_T offset, SIZE_T size, LONG flags);

	enum MappingType
	{
		MAP_PRIVATE = PageState::RW_MAPPED_PRIVATE,
		MAP_SHARED = PageState::RW_MAPPED_SHARED
	};

	void* map (HANDLE mapping, MappingType protection);

	void query (const void* address, MEMORY_BASIC_INFORMATION& mbi) const
	{
		verify (VirtualQueryEx (m_process, address, &mbi, sizeof (mbi)));
	}

	void protect (void* address, SIZE_T size, DWORD protection)
	{
		DWORD old;
		verify (VirtualProtectEx (m_process, address, size, protection, &old));
	}

	struct BlockInfo
	{
		// Currently we are using only one field. But we create structure for possible future extensions.
		HANDLE mapping;
	};

	class Block
	{
	public:
		Block (AddressSpace& space, void* address) :
			m_space (space),
			m_address (round_down ((BYTE*)address, ALLOCATION_GRANULARITY)),
			m_info (space.allocated_block (address))
		{
			assert (m_info.mapping);
		}

		BYTE* address () const
		{
			return m_address;
		}

		HANDLE& mapping ()
		{
			return m_info.mapping;
		}

		void copy (HANDLE src, DWORD offset, DWORD size, LONG flags);
		void unmap ();

	protected:
		struct State
		{
			enum : DWORD
			{
				INVALID,
				RESERVED,
				MAPPED
			}
			state;

			/// <summary>
			/// The OR of all page_state.
			/// </summary>
			DWORD page_state_bits;

			union
			{
				/// <summary>
				/// Valid if block is mapped.
				/// </summary>
				struct
				{
					DWORD page_state [PAGES_PER_BLOCK];
				}
				mapped;

				/// <summary>
				/// Valid if block is reserved.
				/// </summary>
				struct
				{
					BYTE* begin;
					BYTE* end;
				}
				reserved;
			};

			State () :
				state (INVALID)
			{}
		};

		const State& state ();

		void invalidate_state ()
		{
			m_state.state = State::INVALID;
		}

		void map (HANDLE mapping, MappingType protection);

	private:
		AddressSpace& m_space;
		BYTE* m_address;
		BlockInfo& m_info;
		State m_state;
	};

private:
#ifdef _WIN64
	static const size_t SECOND_LEVEL_BLOCK = ALLOCATION_GRANULARITY / sizeof (BlockInfo);
#endif

public:

	BlockInfo & block (void* address);
	BlockInfo & allocated_block (void* address);

	HANDLE process () const
	{
		return m_process;
	}

protected:
	void initialize (DWORD process_id, HANDLE process_handle);

private:
	static void raise_condition ()
	{
		Sleep (0);
	}

private:
	HANDLE m_process;
	HANDLE m_mapping;
#ifdef _WIN64
	BlockInfo** m_directory;
#else
	BlockInfo* m_directory;
#endif
	size_t m_directory_size;
};

}
}

#endif
