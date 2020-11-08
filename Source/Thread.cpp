#include "Thread.inl"

namespace Nirvana {
namespace Core {

void Thread::_enter_to (SyncDomain* sync_domain, bool ret)
{
	schedule_domain_ = sync_domain;
	schedule_ret_ = ret;
	neutral_context_.run_in_context (*this);
	if (exception_) {
		std::exception_ptr ep;
		std::swap (exception_, ep);
		std::rethrow_exception (ep);
	}
}

void Thread::enter_to (SyncDomain* sync_domain, bool ret)
{
	assert (exec_domain_ == &ExecContext::current ());
	SyncDomain* old_sync_domain = exec_domain_->sync_domain ();
	_enter_to (sync_domain, ret);

	// We are in sync_domain now.
	CORBA::Exception::Code err = exec_domain_->scheduler_error ();
	if (err) {
		if (!ret) {
			// This is an outgoing call.
			// We must enter to old_sync_domain back before throwing the exception.
			// Note that this is another thread, so we must set all variables again.
			try {
				_enter_to (old_sync_domain, true);
			} catch (...) {
				ExecContext::abort ();
			}
		}
		CORBA::SystemException::_raise_by_code (err);
	}
}

void Thread::on_exception ()
{
	exception_ = std::current_exception ();
}

}
}
