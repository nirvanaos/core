#include "PortableExecutable.h"
#include <Nirvana/throw_exception.h>
#include <Nirvana/OLF.h>
#include "llvm/BinaryFormat/COFF.h"

namespace Nirvana {
namespace Core {

const void* PortableExecutable::get_COFF (const void* base_address)
{
	const llvm::COFF::DOSHeader* dos_header = (const llvm::COFF::DOSHeader*)base_address;
	if (dos_header->Magic != 'ZM')
		throw_INITIALIZE ();

	const uint32_t* PE_magic = (const uint32_t*)((const uint8_t*)base_address + dos_header->AddressOfNewExeHeader);
	if (*PE_magic != *(const uint32_t*)llvm::COFF::PEMagic)
		throw_INITIALIZE ();

	return PE_magic + 1;
}

PortableExecutable::PortableExecutable (const void* base_address) :
	COFF (get_COFF (base_address)),
	base_address_ (base_address)
{}

bool PortableExecutable::find_OLF_section (Core::Section& section) const NIRVANA_NOEXCEPT
{
	const COFF::Section* ps = COFF::find_section (OLF_BIND);
	if (ps) {
		section.address = (const uint8_t*)base_address_ + ps->VirtualAddress;
		section.size = ps->SizeOfRawData;
		return true;
	}
	return false;
}

}
}
