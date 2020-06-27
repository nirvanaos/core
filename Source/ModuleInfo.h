#ifndef NIRVANA_CORE_MODULEINFO_H_
#define NIRVANA_CORE_MODULEINFO_H_

namespace Nirvana {
namespace Core {

struct ModuleInfo
{
	const void* base_address;
	const void* olf_section;
	size_t olf_size;

	void initialize (const void* base);
};

}
}

#endif
