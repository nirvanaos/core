#include "ORB.h"

namespace CORBA {
namespace Core {

TypeCode::_ref_type ORB::get_compact_typecode (TypeCode::_ptr_type tc, const TC_Recurse* parent)
{
	TCKind kind = tc->kind ();
	switch (kind) {
		case TCKind::tk_struct:
		case TCKind::tk_union:
		case TCKind::tk_value: {
			for (const TC_Recurse* p = parent; p; p = p->prev) {
				if (&p->tc == &tc)
					return create_recursive_tc (tc->id ());
			}
			ULong member_count = tc->member_count ();
			TC_Recurse rec { parent, tc };
			switch (kind) {
				case TCKind::tk_struct: {
					StructMemberSeq members;
					members.reserve (member_count);
					for (ULong i = 0; i < member_count; ++i) {
						members.emplace_back (nullptr, get_compact_typecode (tc->member_type (i), &rec), nullptr);
					}
					return create_struct_tc (tc->id (), nullptr, members);
				}

				case TCKind::tk_union: {
					UnionMemberSeq members;
					members.reserve (member_count);
					for (ULong i = 0; i < member_count; ++i) {
						members.emplace_back (nullptr, tc->member_label (i),
							get_compact_typecode (tc->member_type (i), &rec), nullptr);
					}
					return create_union_tc (tc->id (), nullptr, tc->discriminator_type (), members);
				}

				case TCKind::tk_value: {
					ValueMemberSeq members;
					members.reserve (member_count);
					for (ULong i = 0; i < member_count; ++i) {
						members.emplace_back (nullptr, nullptr, nullptr, nullptr,
							get_compact_typecode (tc->member_type (i), &rec), nullptr, tc->member_visibility (i));
					}
					return create_value_tc (tc->id (), nullptr, tc->type_modifier (), tc->concrete_base_type (), members);
				}
			}
		}

		case TCKind::tk_sequence:
			return create_sequence_tc (tc->length (), get_compact_typecode (tc->content_type (), parent));

		case TCKind::tk_array:
			return create_array_tc (tc->length (), get_compact_typecode (tc->content_type (), parent));

		case TCKind::tk_except: {
			// It is not necessary to check the recursion for the tk_except.
			// But I believe that the get_compact_typecode for tk_except is rarely used.
			// So I prefer to produce slow but compact code for it.
			ULong member_count = tc->member_count ();
			StructMemberSeq members;
			members.reserve (member_count);
			for (ULong i = 0; i < member_count; ++i) {
				members.emplace_back (nullptr, get_compact_typecode (tc->member_type (i), parent), nullptr);
			}
			return create_exception_tc (tc->id (), nullptr, members);
		}

		default:
			return tc->get_compact_typecode ();
	}
}

}
}
