#include "ObjectMap.h"

namespace Nirvana {
namespace Core {

class ObjMapTest : public ObjectMap <int>
{
private:
	virtual void* create (iterator it)
	{
		return nullptr;
	}

	virtual void destroy (void* p)
	{}
};

}
}
