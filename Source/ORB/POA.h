#ifndef NIRVANA_ORB_CORE_POA_H_
#define NIRVANA_ORB_CORE_POA_H_

#include "ServantBase.h"
#include <CORBA/POA_s.h>
#include <unordered_map>

namespace CORBA {
namespace Nirvana {
namespace Core {

extern StaticI_ptr <PortableServer::POA> g_root_POA; // Temporary solution

class POA :
	public Servant <POA, PortableServer::POA>
{
public:
	static Type <String>::ABI_ret _activate_object (Bridge <PortableServer::POA>* obj, Interface* servant, Interface* env)
	{
		try {
			return Type <String>::ret (_implementation (obj).activate_object (TypeI <Object>::in (servant)));
		} catch (const Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return Type <String>::ABI_ret ();
	}

	String activate_object (Object_ptr proxy)
	{
		if (active_object_map_.empty ())
			_add_ref ();
		std::pair <AOM::iterator, bool> ins = active_object_map_.emplace (std::to_string ((uintptr_t)&proxy), 
			Object_var (Object::_duplicate (proxy)));
		if (!ins.second)
			throw PortableServer::POA::ServantAlreadyActive ();
		return ins.first->first;
	}

	void deactivate_object (const String& oid)
	{
		if (!active_object_map_.erase (oid))
			throw PortableServer::POA::ObjectNotActive ();
		if (active_object_map_.empty ())
			_remove_ref ();
	}

private:
	typedef std::unordered_map <String, Object_var> AOM;
	AOM active_object_map_;
};

}
}
}

#endif
