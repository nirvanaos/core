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
#ifndef NIRVANA_ORB_CORE_REQUESTLOCALPOA_H_
#define NIRVANA_ORB_CORE_REQUESTLOCALPOA_H_
#pragma once

#include "RequestLocalBase.h"
#include "RequestInPOA.h"

namespace CORBA {
namespace Core {

class ReferenceLocal;
typedef servant_reference <ReferenceLocal> ReferenceLocalRef;

class NIRVANA_NOVTABLE RequestLocalPOA :
	public RequestLocalBase,
	public RequestInPOA
{
public:
	virtual const IOP::ObjectKey& object_key () const noexcept override;
	virtual Internal::StringView <Char> operation () const noexcept override;

protected:
	RequestLocalPOA (ReferenceLocal& reference, Internal::OperationIndex op,
		unsigned response_flags);

	virtual void _add_ref () noexcept override;
	virtual Nirvana::Core::MemContext* memory () const noexcept override;

	virtual void set_exception (Any& e) override;
	virtual void invoke () override;
	virtual void serve (const ServantProxyBase& proxy) override;

	virtual bool is_cancelled () const noexcept override;

	Internal::OperationIndex op_idx () const noexcept
	{
		return op_idx_;
	}

private:
	ReferenceLocalRef reference_;
	Internal::OperationIndex op_idx_;
};

class NIRVANA_NOVTABLE RequestLocalOnewayPOA :
	public RequestLocalPOA
{
protected:
	RequestLocalOnewayPOA (ReferenceLocal& reference, Internal::OperationIndex op,
		unsigned response_flags) :
		RequestLocalPOA (reference, op, response_flags)
	{}

	virtual void invoke () override;
	virtual void serve (const ServantProxyBase& proxy) override;

};

class NIRVANA_NOVTABLE RequestLocalAsyncPOA :
	public RequestLocalOnewayPOA
{
	typedef RequestLocalOnewayPOA Base;

protected:
	RequestLocalAsyncPOA (RequestCallback::_ptr_type callback,
		ReferenceLocal& reference, Internal::OperationIndex op,
		unsigned response_flags) :
		Base (reference, op, response_flags),
		callback_ (callback)
	{
	}

	virtual void cancel () noexcept override;
	virtual void finalize () noexcept override;

private:
	RequestCallback::_ref_type callback_;
};

}
}

#endif
