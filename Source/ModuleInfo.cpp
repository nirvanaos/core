#include "ModuleInfo.h"
#include <CORBA/CORBA.h>
#include <llvm/BinaryFormat/COFF.h>
#include <Nirvana/OLF.h>

namespace Nirvana {
namespace Core {

class COFF
{
public:
	static bool initialize (ModuleInfo& mi)
	{
		const void* image_base = mi.base_address;
		const llvm::COFF::DOSHeader* dos_header = (const llvm::COFF::DOSHeader*)image_base;
		if (dos_header->Magic != 'ZM')
			return false;

		const uint32_t* PE_magic = (const uint32_t*)((const uint8_t*)image_base + dos_header->AddressOfNewExeHeader);
		if (*PE_magic != *(const uint32_t*)llvm::COFF::PEMagic)
			throw CORBA::INITIALIZE ();

		const llvm::COFF::header* hdr = (llvm::COFF::header*)(PE_magic + 1);

		const llvm::COFF::section* metadata = 0;

		const llvm::COFF::section* s = (const llvm::COFF::section*)((const uint8_t*)(hdr + 1) + hdr->SizeOfOptionalHeader);
		for (uint32_t cnt = hdr->NumberOfSections; cnt; --cnt, ++s) {
			if (is_section (s, OLF_BIND)) {
				metadata = s;
				break;
			}
		}

		if (!metadata)
			throw CORBA::INITIALIZE ();

		mi.olf_section = (const uint8_t*)image_base + metadata->VirtualAddress;
		mi.olf_size = metadata->VirtualSize;

		return true;
	}

private:

	static bool is_section (const llvm::COFF::section* s, const char* name);
};

bool COFF::is_section (const llvm::COFF::section* s, const char* name)
{
	const char* sn = s->Name;
	char c;
	while ((c = *name) && *sn == c) {
		++sn;
		++name;
	}
	if (c)
		return false;
	if (sn - s->Name < llvm::COFF::NameSize)
		return !*sn;
	else
		return true;
}

void ModuleInfo::initialize (const void* base)
{
	base_address = base;
	if (!COFF::initialize (*this))
		throw CORBA::INITIALIZE ();
}

}
}
