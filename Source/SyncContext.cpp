#include "SyncContext.h"
#include "Thread.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE FreeSyncContext :
	public SyncContext
{
public:
	virtual void enter (bool ret);
	virtual SyncDomain* sync_domain ();
	virtual Heap& memory ();
};

ImplStatic <FreeSyncContext> g_free_sync_context;

SyncContext& SyncContext::free_sync_context ()
{
	return g_free_sync_context;
}

void FreeSyncContext::enter (bool ret)
{
	Thread::current ().enter_to (nullptr, ret);
}

SyncDomain* FreeSyncContext::sync_domain ()
{
	return nullptr;
}

Heap& FreeSyncContext::memory ()
{
	Thread& th = Thread::current ();
	return th.exec_domain ()->heap ();
}

}
}
