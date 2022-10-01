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
#ifndef NIRVANA_ORB_CORE_TC_SEQUENCE_H_
#define NIRVANA_ORB_CORE_TC_SEQUENCE_H_
#pragma once

#include "TC_Base.h"
#include "TC_Impl.h"
#include "TC_Ref.h"

namespace CORBA {
namespace Core {

class TC_Sequence : public TC_Impl <TC_Sequence, TC_Base>
{
	typedef TC_Impl <TC_Sequence, TC_Base> Impl;
	typedef Internal::ABI <Internal::Sequence <void> > ABI;

public:
	using Servant::_s_length;
	using Servant::_s_content_type;

	TC_Sequence (TC_Ref&& content_type, ULong bound);

	bool equal (TypeCode::_ptr_type other) const
	{
		return TCKind::tk_sequence == other->kind () && bound_ == other->length ();
	}

	bool equivalent (TypeCode::_ptr_type other) const
	{
		return equal (dereference_alias (other));
	}

	ULong length () const NIRVANA_NOEXCEPT
	{
		return bound_;
	}

	TypeCode::_ref_type content_type () const NIRVANA_NOEXCEPT
	{
		return content_type_;
	}

	static size_t _s_n_size (Internal::Bridge <TypeCode>*, Internal::Interface*)
	{
		return sizeof (ABI);
	}

	static size_t _s_n_align (Internal::Bridge <TypeCode>*, Internal::Interface*)
	{
		return alignof (ABI);
	}

	static Octet _s_n_is_CDR (Internal::Bridge <TypeCode>*, Internal::Interface*)
	{
		return false;
	}

	void n_construct (void* p) const
	{
		Internal::check_pointer (p);
		reinterpret_cast <ABI*> (p)->reset ();
	}

	void n_destruct (void* p) const
	{
		Internal::check_pointer (p);
		ABI& abi = *reinterpret_cast <ABI*> (p);
		size_t size = abi.size;
		if (size) {
			Internal::check_pointer (abi.ptr);
			if (kind_ == KIND_NONCDR) {
				Octet* p = (Octet*)abi.ptr;
				do {
					content_type_->n_destruct (p);
					p += element_size_;
				} while (--size);
			}
		}
		abi.size = 0;
		if (abi.allocated) {
			Nirvana::g_memory->release (abi.ptr, abi.allocated);
			abi.allocated = 0;
			abi.ptr = nullptr;
		}
	}

	void n_copy (void* dst, const void* src) const
	{
		Internal::check_pointer (dst);
		Internal::check_pointer (src);
		const ABI& abi_src = *(const ABI*)src;
		ABI& abi_dst = *(ABI*)dst;
		abi_dst.reset ();
		size_t count = abi_src.size;
		if (count) {
			Internal::check_pointer (abi_src.ptr);
			size_t size = count * element_size_;
			if (abi_src.allocated < size)
				throw BAD_PARAM ();

			if (kind_ != KIND_NONCDR) {
				abi_dst.ptr = Nirvana::g_memory->copy (nullptr, abi_src.ptr, size, 0);
				abi_dst.size = count;
				abi_dst.allocated = size;
			} else {
				const Octet* psrc = (const Octet*)abi_src.ptr;
				const Octet* src_end = psrc + size;
				void* ptr = Nirvana::g_memory->allocate (nullptr, size, 0);
				Octet* pdst = (Octet*)ptr;
				try {
					do {
						content_type_->n_copy (pdst, psrc);
						pdst += element_size_;
						psrc += element_size_;
					} while (psrc != src_end);
				} catch (...) {
					while (pdst > ptr) {
						content_type_->n_destruct (pdst);
						pdst -= element_size_;
					}
					Nirvana::g_memory->release (ptr, size);
					throw;
				}
				abi_dst.ptr = ptr;
				abi_dst.size = count;
				abi_dst.allocated = size;
			}
		}
	}

	static void n_move (void* dst, void* src)
	{
		Internal::check_pointer (dst);
		Internal::check_pointer (src);
		ABI* abi_src = (ABI*)src;
		ABI* abi_dst = (ABI*)dst;
		*abi_dst = *abi_src;
		abi_src->reset ();
	}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		marshal (src, count, rq, false);
	}

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		marshal (src, count, rq, true);
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		Internal::check_pointer (dst);
		ABI* pdst = (ABI*)dst, * end = pdst + count;
		switch (kind_) {
			case KIND_CHAR:
				for (; pdst != end; ++pdst) {
					rq->unmarshal_char_seq ((IDL::Sequence <Char>&)*pdst);
				}
				break;

			case KIND_WCHAR:
				for (; pdst != end; ++pdst) {
					rq->unmarshal_wchar_seq ((IDL::Sequence <WChar>&)*pdst);
				}
				break;

			case KIND_CDR:
				for (; pdst != end; ++pdst) {
					if (rq->unmarshal_seq (element_align_, element_size_, pdst->size, pdst->ptr, pdst->allocated))
						content_type_->n_byteswap (pdst->ptr, pdst->size);
				}
				break;

			default:
				for (; pdst != end; ++pdst) {
					content_type_->n_destruct (pdst);
					pdst->reset ();
					size_t size = rq->unmarshal_seq_begin ();
					if (size) {
						size_t cb = size * element_size_;
						void* p = Nirvana::g_memory->allocate (nullptr, cb, 0);
						try {
							content_type_->n_unmarshal (rq, size, p);
						} catch (...) {
							Nirvana::g_memory->release (p, cb);
							throw;
						}
					}
				}
				break;
		}
	}

	using TC_Base::_s_n_byteswap;

private:
	void marshal (const void* src, size_t count, Internal::IORequest_ptr rq, bool out) const;

private:
	TC_Ref content_type_;
	size_t element_size_;
	size_t element_align_;

	enum
	{
		KIND_CHAR,
		KIND_WCHAR,
		KIND_CDR,
		KIND_NONCDR
	} kind_;

	ULong bound_;
};

}
}

#endif
