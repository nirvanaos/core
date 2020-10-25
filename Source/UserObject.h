#ifndef NIRVANA_CORE_USEROBJECT_H_
#define NIRVANA_CORE_USEROBJECT_H_

namespace Nirvana {
namespace Core {

/// \brief Object allocated from the user heap.
class UserObject
{
public:
	void* operator new (size_t cb);
	void operator delete (void* p, size_t cb);
};


}
}

#endif
