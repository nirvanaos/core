#include "COFF.h"
#include "llvm/BinaryFormat/COFF.h"

namespace Nirvana {
namespace Core {

inline
bool COFF::is_section (const Section& s, const char* name) NIRVANA_NOEXCEPT
{
	const char* sn = s.Name;
	char c;
	while ((c = *name) && *sn == c) {
		++sn;
		++name;
	}
	if (c)
		return false;
	if (sn - s.Name < llvm::COFF::NameSize)
		return !*sn;
	else
		return true;
}

const COFF::Section* COFF::find_section (const char* name) const NIRVANA_NOEXCEPT
{
	const Section* s = (const Section*)((const uint8_t*)(hdr_ + 1) + hdr_->SizeOfOptionalHeader);
	for (uint32_t cnt = hdr_->NumberOfSections; cnt; --cnt, ++s) {
		if (is_section (*s, name))
			return s;
	}

	return nullptr;
}

}
}
