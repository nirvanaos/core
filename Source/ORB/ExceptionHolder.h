/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include <CORBA/Server.h>
#include "RequestEncap.h"
#include "StreamInEncap.h"
#include "StreamOutEncap.h"
#include "../SyncContext.h"
#include "../Module.h"

namespace CORBA {
namespace Core {

class ExceptionHolderImpl;

class NIRVANA_NOVTABLE ExceptionHolder :
	public Internal::ValueImpl <ExceptionHolder, ValueBase>,
	public Internal::ValueImpl <ExceptionHolder, ::Messaging::ExceptionHolder>,
	public Internal::ValueBaseCopy <ExceptionHolderImpl>,
	public Internal::ValueBaseFactory < ::Messaging::ExceptionHolder>,
	public Internal::ValueBaseMarshal <ExceptionHolder>,
	public Internal::ValueNonTruncatable
{
	typedef Internal::Aggregated <ExceptionHolder, Messaging::ExceptionHolder> Base;

	DECLARE_CORE_INTERFACE
public:
	typedef Messaging::ExceptionHolder PrimaryInterface;

	ExceptionHolder () :
		user_exceptions_ (nullptr),
		user_exceptions_cnt_ (0)
	{}

	ExceptionHolder (Any& exc);

	Internal::Interface* _query_valuetype (Internal::String_in id) noexcept
	{
		return Internal::FindInterface <Messaging::ExceptionHolder>::find (*this, id);
	}

	static ULong _refcount_value () noexcept
	{
		return 1;
	}

	void _marshal (Internal::IORequest::_ptr_type rq) const
	{
		Internal::ValueData <Messaging::ExceptionHolder>::_marshal (rq);
	}

	void _unmarshal (Internal::IORequest::_ptr_type rq)
	{
		Internal::ValueData <Messaging::ExceptionHolder>::_unmarshal (rq);
	}

	void set_user_exceptions (const Internal::ExceptionEntry* exc_list, size_t count)
	{
		if (exc_list)
			module_ = Nirvana::Core::SyncContext::current ().module ();
		else {
			if (count)
				throw BAD_PARAM ();
			module_ = nullptr;
		}
		user_exceptions_ = exc_list;
		user_exceptions_cnt_ = count;
	}

	Internal::IORequest::_ref_type get_exception (bool& is_system, const Internal::ExceptionEntry*& exc_list, size_t& count)
	{
		exc_list = user_exceptions_;
		count = user_exceptions_cnt_;
		is_system = is_system_exception ();

		return make_pseudo <Nirvana::Core::ImplDynamic <Request> > (std::ref (*this));
	}

private:
	class Request :
		public RequestEncap
	{
	public:
		Request (ExceptionHolder& eh) :
			RequestEncap (nullptr, nullptr),
			stream_ (std::ref (eh.marshaled_exception ()), true),
			holder_ (&eh)
		{
			stream_.little_endian (eh.byte_order ());
			stream_in_ = &stream_;
		}

	private:
		Nirvana::Core::ImplStatic <StreamInEncap> stream_;
		servant_reference <ExceptionHolder> holder_;
	};

	static OctetSeq marshal_exception (Any& exc)
	{
		Nirvana::Core::ImplStatic <StreamOutEncap> stm (true);
		Nirvana::Core::ImplStatic <RequestEncap> rq (nullptr, &stm);
		TypeCode::_ptr_type tc = exc.type ();
		Internal::Type <IDL::String>::marshal_in (tc->id (), rq._get_ptr ());
		tc->n_marshal_out (exc.data (), 1, rq._get_ptr ());
		return std::move (stm.data ());
	}

private:
	const Internal::ExceptionEntry* user_exceptions_;
	size_t user_exceptions_cnt_;

	// Hold module reference to ensure that user_exceptions_ in module constant memory are available.
	servant_reference <Nirvana::Core::Module> module_;
};

class ExceptionHolderImpl :
	public ExceptionHolder,
	public Internal::RefCountBase <ExceptionHolderImpl>
{
public:
	ExceptionHolderImpl ()
	{}

	ExceptionHolderImpl (const ExceptionHolder& src) :
		ExceptionHolder (src)
	{}

	ULong _refcount_value () const noexcept
	{
		Internal::RefCountBase <ExceptionHolderImpl>::_refcount_value ();
	}

	virtual void _add_ref () noexcept override
	{
		Internal::RefCountBase <ExceptionHolderImpl>::_add_ref ();
	}

	virtual void _remove_ref () noexcept override
	{
		Internal::RefCountBase <ExceptionHolderImpl>::_remove_ref ();
	}
};

}
}
