// Nirvana Memory Service for Win32

#include "WinMemory.h"
#include <cpplib.h>

namespace Nirvana {

WinMemory::Data WinMemory::sm_data;

void WinMemory::get_line_state (const Line* line, LineState& state)
{
	assert (!((UWord)line % LINE_SIZE));

	bool check_sharing = false;

	{
		MemoryBasicInformation mbi (line);

		if (MEM_FREE == mbi.State) {
			state.m_allocation_protect = 0;
			mbi.Type = 0;
		} else
			state.m_allocation_protect = mbi.AllocationProtect;

		if (MEM_MAPPED != mbi.Type) {
			zero ((UWord*)state.m_page_states, (UWord*)(state.m_page_states + PAGES_PER_LINE));
			return;
		}

		Octet* page_state_ptr = state.m_page_states;
		const Page* page = line->pages;
		const Line* line_end = line + 1;
		for (;;) {

			assert (MEM_FREE != mbi.State);
			assert (mbi.allocation_base () == line);

			Octet page_state;
			if (MEM_COMMIT == mbi.State) {

				if (PAGE_NOACCESS == mbi.Protect)
					page_state = PAGE_DECOMMITTED | PAGE_VIRTUAL_PRIVATE;
				else if (WIN_MASK_MAPPED & mbi.Protect)
					page_state = PAGE_MAPPED_PRIVATE;
				else {
					assert (WIN_MASK_COPIED & mbi.Protect);
					page_state = PAGE_COPIED | PAGE_VIRTUAL_PRIVATE;
				}

				check_sharing = true;

			} else
				page_state = PAGE_NOT_COMMITTED;

			const Page* rgn_end = mbi.end ();

			assert (rgn_end <= line_end->pages);

			do {
				*(page_state_ptr++) = page_state;
			} while (++page < rgn_end);

			if (rgn_end >= line_end->pages)
				break;

			mbi.at (rgn_end);
		}
	}

	if (check_sharing) {

		// Check sharing state of pages

		// Work with mapping list - lock it
		Lock lock (sm_data.critical_section);

		for (
			const Line* shared_line = logical_line (line).shared_next ();
			shared_line != line;
			shared_line = logical_line (shared_line).shared_next ()
			) {

			const Page* page = shared_line->pages;
			Octet* page_state_ptr = state.m_page_states;
			Octet* page_state_end = state.m_page_states + PAGES_PER_LINE;
			check_sharing = false;

			do {
				if (*page_state_ptr & PAGE_VIRTUAL_PRIVATE) {

					MemoryBasicInformation mbi (page);
					const Page* rgn_end = mbi.end ();
					assert (rgn_end <= (shared_line + 1)->pages);

					if (WIN_MASK_MAPPED & mbi.Protect) {
						do {
							*(page_state_ptr++) &= ~PAGE_VIRTUAL_PRIVATE;
						} while (++page < rgn_end);
					} else {
						check_sharing = true;
						page_state_ptr += rgn_end - page;
						page = rgn_end;
					}
				} else
					++page_state_ptr;

			} while (page_state_ptr < page_state_end);

			if (!check_sharing)
				break;
		}
	}
}

Line* WinMemory::reserve (UWord size, UWord flags, Line* dst)
{
	assert (size);
	assert (!((UWord)dst % LINE_SIZE));

	UWord protect;
	if (flags & Memory::READ_ONLY)
		protect = WIN_READ_RESERVE;
	else
		protect = WIN_WRITE_RESERVE;

	// Reserve full lines

	UWord full_lines_size = (size + LINE_SIZE - 1) & ~(LINE_SIZE - 1);
	Line* begin = (Line*)VirtualAlloc (dst, full_lines_size, MEM_RESERVE, protect);
	if (!(begin || dst))
		throw NO_MEMORY ();

	assert (!((UWord)begin % LINE_SIZE));

	return begin;
}

UWord WinMemory::check_allocated (Page* begin, Page* end)
{
	assert (begin);
	assert (end);
	assert (!((UWord)begin % PAGE_SIZE));
	assert (!((UWord)end % PAGE_SIZE));
	assert (begin < end);

	// Start address must be valid
	check_pointer (begin);

	Line* line = Line::begin (begin);
	Line* end_line = Line::end (end);

	UWord allocation_protect = 0;

	// Check windows memory state
	do {

		MemoryBasicInformation mbi (line);

		// Check allocation

		if (MEM_FREE == mbi.State)
			throw BAD_PARAM (); // Not allocated

		allocation_protect |= mbi.AllocationProtect;

		// Check mapping

		switch (mbi.Type) {

		case MEM_PRIVATE:

			if (MEM_RESERVE != mbi.State)
				throw BAD_PARAM (); // This pages not from memory service

			if (mbi.RegionSize % LINE_SIZE)
				throw BAD_PARAM (); // This pages not from memory service

			line = (Line*)mbi.end ();
			break;

		case MEM_MAPPED:

			if (!mapping (mbi.BaseAddress))
				throw BAD_PARAM (); // This pages not from memory service

			++line;
			break;

		default:
			throw BAD_PARAM (); // This pages not from memory service
		}
	} while (line < end_line);

	return allocation_protect;
}

void WinMemory::release (Line* begin, Line* end)
{
	// Release full lines
	for (Line* line = begin; line < end;) {

		MemoryBasicInformation mbi (line);

		switch (mbi.Type) {

		case MEM_PRIVATE: // Reserved space
			{
				// Release entire region
				VirtualFree (mbi.AllocationBase, 0, MEM_RELEASE);

				// Re-reserve at begin
				UWord re_reserve = (UWord)line - (UWord)mbi.AllocationBase;
				if (re_reserve)
					verify (VirtualAlloc (mbi.AllocationBase, re_reserve, MEM_RESERVE, mbi.Protect));

				// Re-reserve at end
				line = (Line*)((UWord)mbi.BaseAddress + mbi.RegionSize);
				if ((UWord)line > (UWord)end)
					verify (VirtualAlloc (end, (UWord)line - (UWord)end, MEM_RESERVE, mbi.Protect));
			}
			break;

		case MEM_MAPPED:  // Mapped space
			unmap_and_release (line);
			++line;
			break;
		}
	}
}

void WinMemory::copy_one_line (Octet* dst_begin, Octet* src_begin, UWord size, UWord flags)
{
	assert (src_begin);
	assert (dst_begin);
	assert (dst_begin != src_begin);
	assert (size);
	assert ((dst_begin + size) <= Line::end (dst_begin + 1)->pages->bytes);

	// To use sharing, source and destination must have same offset from line begin.

	if (!(
		(((UWord)dst_begin & (LINE_SIZE - 1)) == ((UWord)src_begin & (LINE_SIZE - 1)))
		&&
		copy_one_line_aligned (dst_begin, src_begin, size, flags)
		)) {

		// Perform simple copy

		LineState dst_line_state;
		get_line_state (Line::begin (dst_begin), dst_line_state);

		CostOfOperation cost;
		commit_line_cost (dst_begin, dst_begin + size, dst_line_state, cost, true);

		copy_one_line_really (dst_begin, src_begin, size, cost.decide_remap (), dst_line_state);
	}

	// Освобождаем исходные страницы, если нужно.
	// Вместо release делаем decommit. Иначе другой тред может успеть занять область внутри копируемого участка.
	if (flags &= (Memory::DECOMMIT | Memory::RELEASE)) {

		Page* begin, *end;
		if (Memory::RELEASE == flags) {
			begin = Line::begin (src_begin)->pages;
			end = Line::end (src_begin + size)->pages;
		} else {
			begin = Page::end (src_begin);
			end = Page::begin (src_begin + size);
		}

		if (dst_begin < src_begin) {
			Page* dst_end = Page::end (dst_begin + size);
			if (begin < dst_end)
				begin = dst_end;
		} else {
			Page* pg_begin = Page::begin (dst_begin);
			if (end > pg_begin)
				end = pg_begin;
		}

		if (begin < end)
			decommit (begin, end);
	}
}

bool WinMemory::copy_one_line_aligned (Octet* dst_begin, Octet* src_begin, UWord size, UWord flags)
{
	// We must minimize two parameters: amount of copied data
	// and amount of allocated physical memory.

	// Будем считать, что копирование каждого байта добавляет к стоимости операции единицу.
	// Выделение физической страницы добавляет PAGE_ALLOCATE_COST единиц.

	// We can copy by two ways:
	// 1. Sharing
	// 1.1. Re-map source line (data copying possible)
	// 1.2. Map destination line at temporary address
	// 1.3. Map source line to destination
	// 1.4. Copy unchanged data back to destination.
	// 1.5. Unmap temporary mapping
	// 2. Copying
	// 2.1. Commit destination pages
	// 2.2. Copy source data to destination

	// We must choose optimal way now

	/*
	При подсчете объема данных, копируемых при выполнении операции, необходимо учитывать
	не только те данные, которые копируем мы, но и те, которые копирует система,
	если страница copy-on-write и мы пишем в нее.
	*/

	HANDLE src_mapping = mapping (src_begin);
	if (src_mapping) {

		// Source line allocated by this service

		Octet* dst_end = dst_begin + size;

		// Определяем диапазон целевых страниц, которым соответствуют исходные страницы,
		// освобождающиеся в процессе копирования.
		// Для простоты вычисления воспользуемся тем, что исходная и целевая области
		// имеют одинаковое смещение относительно начала строки.
		Page* dst_release_begin = 0;
		Page* dst_release_end = 0;

		switch (flags & (Memory::DECOMMIT | Memory::RELEASE)) {

		case 0:
			break;

		case Memory::DECOMMIT:
			dst_release_begin = Page::end (dst_begin);
			dst_release_end = Page::begin (dst_end);
			break;

		case Memory::RELEASE:
			dst_release_begin = Page::begin (dst_begin);
			dst_release_end = Page::end (dst_end);
			break;

		default:
			assert (false);
		}

		HANDLE dst_mapping = mapping (dst_begin); // can be == src_mapping

		// Получаем состояния страниц для исходной и целевой строк.

		Line* src_line = Line::begin (src_begin);
		LineState src_line_state;
		get_line_state (src_line, src_line_state);
		assert (src_line_state.m_allocation_protect); // not free

		Line* dst_line = Line::begin (dst_begin);
		LineState dst_line_state;
		get_line_state (dst_line, dst_line_state);
		assert (dst_line_state.m_allocation_protect); // not free

		// Анализируем состояния страниц, определяем возможность и стоимость отображения.

		CostOfOperation cost_of_share;  // стоимость разделения

		bool need_full_remap = false;
		// если true, разделение возможно только после полного переотображения

		LineRegions bytes_in_place, bytes_not_mapped; // см. ниже

		Octet* src_page_state_ptr = src_line_state.m_page_states;
		Octet* dst_page_state_ptr = dst_line_state.m_page_states;
		Octet* dst_page_state_end = dst_line_state.m_page_states + PAGES_PER_LINE;
		Page* dst_page = dst_line->pages;

		do {

			Octet src_page_state = *src_page_state_ptr;
			Octet dst_page_state = *dst_page_state_ptr;

			if (dst_page_state & PAGE_COMMITTED) {

				// Если исходная и целевая логические страницы не отображены на общую виртуальную,
				if ((src_mapping != dst_mapping) || (src_page_state != PAGE_MAPPED_SHARED) || (dst_page_state != PAGE_MAPPED_SHARED)) {

					// Если целевая страница не разделена, она будет освобождена
					if (dst_page_state & PAGE_VIRTUAL_PRIVATE)
						cost_of_share += -PAGE_ALLOCATE_COST;

					// Определяем количество байт, которые должны остаться на целевой странице.
					Word dst_bytes = dst_begin - dst_page->bytes;
					if (dst_bytes >= PAGE_SIZE) {

						// Страница целиком перед областью копирования
						dst_bytes = PAGE_SIZE;

						// При разделении, эта страница должна быть скопирована в исходный маппинг.
						bytes_in_place.push_back (dst_page->bytes - dst_line->pages->bytes, PAGE_SIZE);

					} else {

						if (dst_bytes <= 0)
							dst_bytes = 0;
						else
							// При разделении, dst_bytes из начала страницы должны быть скопированы
							// в исходный маппинг.
							bytes_in_place.push_back (dst_page->bytes - dst_line->pages->bytes, dst_bytes);

						Word bytes = dst_page->bytes + PAGE_SIZE - dst_end;
						if (bytes >= PAGE_SIZE) {

							// Страница целиком за областью копирования.
							bytes = PAGE_SIZE;

							// При разделении, эта страница должна быть скопирована в исходный маппинг.
							bytes_in_place.push_back (dst_page->bytes - dst_line->pages->bytes, PAGE_SIZE);

						} else if (bytes <= 0)
							bytes = 0;
						else
							// При разделении, bytes из конца страницы должны быть скопированы
							// в исходный маппинг.
							bytes_in_place.push_back (dst_end - dst_line->pages->bytes, bytes);

						dst_bytes += bytes;
					}

					if (dst_bytes) {

						// На целевой странице должны остаться dst_bytes.
						// При отображении они должны попасть в исходный маппинг.
						// Чтобы не усложнять алгоритм отображения, примем требование,
						// что исходные виртуальная и логическая страницы должны быть свободны
						// или освобождаться. При этом целевую страницу можно отобразить приватно.
						// В противном случае, отображение будем считать невозможным.

						if (
							// исходная страница уже содержит данные,
							(src_page_state & PAGE_COMMITTED)
							&& // и мы не собираемся ее освобождать
							((dst_page >= dst_release_end) || (dst_page < dst_release_begin))
							) {

							// считаем отображение невозможным.
							break;
						}

						if (!(src_page_state & PAGE_VIRTUAL_PRIVATE))
							// Исходная виртуальная страница занята.
							// Разделение возможно только после полного переотображения,
							// когда она освободится.
							need_full_remap = true;

						// Учитываем стоимость копирования dst_bytes в исходный маппинг.
						cost_of_share += dst_bytes;
					}
				}
			}

			// Проверяем необходимость переотображения исходной страницы
			if ((src_page_state & PAGE_COPIED) == PAGE_COPIED) {

				// Исходная страница нуждается в переотображении.
				// Подсчитаем количество байт, попадающих в область копирования.
				Page* page_end = dst_page + 1;
				Octet* copy_begin = max (dst_begin, dst_page->bytes);
				Word src_bytes = min (dst_end, page_end->bytes) - copy_begin;
				if (src_bytes > 0) {

					// Если не делать переотображение, надо скопировать src_bytes,
					// при этом будет выделена новая страница.
					cost_of_share [REMAP_NONE] += src_bytes + PAGE_ALLOCATE_COST;

					// Запомним не отображенный блок.
					bytes_not_mapped.push_back (copy_begin - dst_line->pages->bytes, src_bytes);

					// Если виртуальная страница разделена, она будет скопирована системой при записи.
					if (!(src_page_state & PAGE_VIRTUAL_PRIVATE))
						cost_of_share [REMAP_NONE] += PAGE_SIZE;

					// Если исходная страница освобождается, вычтем ее из стоимости.
					if ((dst_page < dst_release_end) && (dst_page >= dst_release_begin))
						cost_of_share [REMAP_NONE] -= PAGE_ALLOCATE_COST;
				}

				// Тип переотображения зависит от занятости виртуальной страницы.
				cost_of_share.remap_type ((src_page_state & PAGE_VIRTUAL_PRIVATE) ? REMAP_PART : REMAP_FULL);
			}

			++dst_page;
			++src_page_state_ptr;
			++dst_page_state_ptr;
		} while (dst_page_state_ptr < dst_page_state_end);

		if (dst_page_state_ptr >= dst_page_state_end) {

			// Разделение возможно.
			bool can_share = true;

			// Decide source line remap type for sharing
			RemapType remap_share;

			// No remapping possible for 'this' object location.        
/*			if (((void*)this >= src_line) && ((void*)this < (src_line + 1))) {

				if (need_full_remap)
					can_share = false;
				else
					remap_share = REMAP_NONE;

			} else */
				remap_share = need_full_remap ? REMAP_FULL : cost_of_share.decide_remap ();

			// Calculate cost of simple copying.

			CostOfOperation cost_of_copy = size;
			commit_line_cost (dst_begin, dst_end, dst_line_state, cost_of_copy, true);

			// Decide destination line remap type for simple copy.
			RemapType remap_copy;

			// No remapping possible for 'this' object location.        
/*			if (((void*)this >= dst_line) && ((void*)this < (dst_line + 1)))
				remap_copy = REMAP_NONE;
			else */
				remap_copy = cost_of_copy.decide_remap ();

			// Выбираем оптимальный вариант.
			if (can_share && (cost_of_share [remap_share] <= cost_of_copy [remap_copy])) {

				// Копируем путем разделения.

				// Переотображаем исходную строку.
				if (remap_share != REMAP_NONE)
					src_mapping = remap_line (src_line->pages->bytes, src_line->pages->bytes, src_line_state, remap_share);

				// Копируем даные целевой строки, которые должны остаться на месте.
				if (bytes_in_place.not_empty ()) {

					Line* tmp_line;
					// Виртуальные страницы, в которые мы копируем, заведомо свободны, это мы проверили раньше.
					// Однако, если переотображение не выполнялось, они могут быть не отображены на исходную строку.
					// Поэтому, если remap_share == REMAP_NONE, отображаем исходную виртуальную строку
					// по временному адресу.

					if (remap_share == REMAP_NONE) {
						if (!(tmp_line = (Line*)MapViewOfFileEx (src_mapping, FILE_MAP_WRITE, 0, 0, LINE_SIZE, 0)))
							throw NO_MEMORY ();
					} else
						tmp_line = src_line;

					for (LineRegions::Iterator it = bytes_in_place.begin (); it != bytes_in_place.end (); ++it) {

						Octet* dst = tmp_line->pages->bytes + it->offset;

						// Даже если страница read-only, VirtualAlloc сделает ее read-write
						if (!VirtualAlloc (dst, it->size, MEM_COMMIT, PAGE_READWRITE))
							throw NO_MEMORY ();

						Octet* src_begin = dst_line->pages->bytes + it->offset;
						Octet* src_end = src_begin + it->size;
						real_copy (src_begin, src_end, dst);

						// Запрещаем доступ из исходной строки.
						DWORD old;
						verify (VirtualProtect (src_line->pages->bytes + it->offset, it->size, WIN_NO_ACCESS, &old));
					}

					// Освобождаем временное отображение.
					if (tmp_line != src_line)
						UnmapViewOfFile (tmp_line);
				}

				// Отображаем исходную строку на целевую.
				map (dst_line, src_line);

				// Вычисляем новые состояния страниц целевой строки.

				src_page_state_ptr = src_line_state.m_page_states;
				dst_page_state_ptr = dst_line_state.m_page_states;
				dst_page = dst_line->pages;

				UWord prot_private, prot_shared;
				if (WIN_MASK_WRITE & dst_line_state.m_allocation_protect) {
					prot_private = WIN_WRITE_MAPPED_PRIVATE;
					prot_shared = WIN_WRITE_MAPPED_SHARED;
				} else {
					prot_private =
						prot_shared = WIN_READ_MAPPED;
				}

				do {

					DWORD old;

					Page* next_page = dst_page + 1;

					// Если целевая страница содержит данные, которые нужно сохранить,
					// она всегда отображается приватно.
					if (
						(*dst_page_state_ptr & PAGE_COMMITTED)
						&&
						((dst_page->bytes < dst_begin) || (next_page->bytes > dst_end))
						) {

						// Целевая страница содержит данные и находится вне области копирования, по крайней мере, частично.
						// Сохраняемые целевые страницы всегда отображаются приватно.
						// Исходная страница не должна быть в маппинге или должна освобождаться.
						assert ((PAGE_NOT_COMMITTED == *src_page_state_ptr) || (PAGE_COPIED == *src_page_state_ptr) || ((dst_page >= dst_release_begin) && (dst_page < dst_release_end)));
						verify (VirtualProtect (dst_page, PAGE_SIZE, prot_private, &old));
						*dst_page_state_ptr = PAGE_MAPPED_PRIVATE;

					} else if ((dst_page->bytes < dst_end) && (next_page->bytes > dst_begin)) {

						// Страница в области копирования

						if ((dst_page < dst_release_end) && (dst_page >= dst_release_begin)) {

							// Страница соответствует освобождающейся исходной странице.
							// Приватность сохраняется.
							if (*src_page_state_ptr & PAGE_VIRTUAL_PRIVATE) {
								verify (VirtualProtect (dst_page, PAGE_SIZE, prot_private, &old));
								*dst_page_state_ptr = PAGE_MAPPED_PRIVATE;
							} else {
								verify (VirtualProtect (dst_page, PAGE_SIZE, prot_shared, &old));
								*dst_page_state_ptr = PAGE_MAPPED_SHARED;
							}

						} else {

							// Страница разделяется.
							verify (VirtualProtect (dst_page, PAGE_SIZE, prot_shared, &old));
							*dst_page_state_ptr = PAGE_MAPPED_SHARED;
						}
					}

					++dst_page;
					++src_page_state_ptr;
					++dst_page_state_ptr;
				} while (dst_page_state_ptr < dst_page_state_end);

				// Если переотображение не выполнялось, физически копируем не отображенные данные.
				if (remap_share == REMAP_NONE)
					for (LineRegions::Iterator it = bytes_not_mapped.begin (); it != bytes_not_mapped.end (); ++it)
						copy_one_line_really (dst_line->pages->bytes + it->offset, src_line->pages->bytes + it->offset, it->size, REMAP_NONE, dst_line_state);

				// Если исходная строка read-write,
				if (WIN_MASK_WRITE & src_line_state.m_allocation_protect) {
					// надо поменять защиту разделенных страниц на copy-on-write.
					// Если страница не отображена, она останется read-write.
					DWORD old;
					verify (VirtualProtect (src_begin, size, WIN_WRITE_MAPPED_SHARED, &old));
				}

			} else
				// Выполняем простое копирование, состояние целевой страницы проанализировано.
				copy_one_line_really (dst_begin, src_begin, size, remap_copy, dst_line_state);

			return true; // Копирование выполнено.

		}

	} else
		// Source line is not allocated by this service
		assert (!(flags & (Memory::DECOMMIT | Memory::RELEASE)));

	return false;
}

void WinMemory::copy_one_line_really (Octet* dst_begin, const Octet* src_begin, UWord size, RemapType remap_type, LineState& dst_line_state)
{
	assert (size);

	Octet* dst_end = dst_begin + size;

	if (remap_type != REMAP_NONE)
		remap_line (dst_begin, dst_end, dst_line_state, remap_type);

	// Commit pages
	Page* dst_begin_page = Page::begin (dst_begin);
	Page* dst_end_page = Page::end (dst_end);
	commit_one_line (dst_begin_page, dst_end_page, dst_line_state, false);

	// Do copy
	real_move (src_begin, src_begin + size, dst_begin);

	if (WIN_MASK_READ & dst_line_state.m_allocation_protect) {
		// Restore read-only protection

		Octet* page_state_ptr = dst_line_state.m_page_states + (dst_begin_page - Line::begin (dst_begin)->pages);
		Page* page = dst_begin_page;
		do {

			DWORD old;
			verify (VirtualProtect (page, PAGE_SIZE,
				((*page_state_ptr & PAGE_COPIED) == PAGE_COPIED) ? WIN_READ_COPIED : WIN_READ_MAPPED,
															&old));

			++page_state_ptr;
			++page;
		} while (page < dst_end_page);
	}
}

void WinMemory::commit (Page* begin, Page* end, UWord zero_init)
{
	assert (begin);
	assert (end);
	assert (begin < end);
	assert (!((UWord)begin % PAGE_SIZE));
	assert (!((UWord)end % PAGE_SIZE));

	do {

		// Define line margins

		Page* line_end = Line::end (begin->bytes + 1)->pages;
		if (line_end > end)
			line_end = end;

		Line* line = Line::begin (begin);

		// Get current line state

		LineState line_state;
		get_line_state (line, line_state);

		// Do not remap line if one contains this object
/*		if (((void*)this < line) || ((void*)this >= (line + 1))) { */

			// Calculate operation cost for remapping

			CostOfOperation cost;
			commit_line_cost (begin->bytes, end->bytes, line_state, cost, false);

			// Decide remap type

			RemapType remap_type = cost.decide_remap ();
			if (remap_type != REMAP_NONE)
				remap_line (begin->bytes, begin->bytes, line_state, remap_type);
/*		} */

		commit_one_line (begin, line_end, line_state, zero_init);

		begin = line_end;

	} while (begin < end);
}

void WinMemory::commit_line_cost (const Octet* begin, const Octet* end, const LineState& line_state, CostOfOperation& cost, bool copy)
{
	const Page* page = Line::begin (begin)->pages;
	const Page* line_end = page + PAGES_PER_LINE;
	const Octet* page_state_ptr = line_state.m_page_states;

	do {

		Octet page_state = *page_state_ptr;

		const Page* page_end;

		if ((page->bytes < end) && ((page_end = page + 1)->bytes > begin)) {

			// Страница в области фиксации.
			// Стоимость фиксации должна включать перевод этой страницы в private
			// состояние, поскольку, после commit обычно следует запись в данный диапазон.
			// В частности, это происходит при физическом копировании, где тоже
			// вызывается данная функция.

			// Определим количество байт, которые должны остаться на странице без изменений.

			UWord dst_bytes;
			if (copy) {

				// Подсчитывается стоимость копирования.
				// Байты, находящиеся между begin и end, не участвуют в переотображении.
				Word bytes = begin - page->bytes;
				if (bytes > 0)
					dst_bytes = bytes;
				else
					dst_bytes = 0;

				bytes = page_end->bytes - end;
				if (bytes > 0)
					dst_bytes += bytes;
			} else
				dst_bytes = PAGE_SIZE;

			switch (page_state) {

			case PAGE_MAPPED_SHARED:

				// Целевая страница разделена, она будет скопирована системой
				// перед копированием в нее src_bytes.
				// Таким образом, кроме копирования данных,
				// будет выделена и скопирована целая страница.

				cost [REMAP_NONE] += PAGE_SIZE + PAGE_ALLOCATE_COST;
				cost [REMAP_PART] += PAGE_SIZE + PAGE_ALLOCATE_COST;

				// При полном переотображении, мы выделим новую страницу и скопируем в нее dst_bytes.

				cost [REMAP_FULL] += dst_bytes + PAGE_ALLOCATE_COST;

				break;

			case PAGE_DECOMMITTED | PAGE_VIRTUAL_PRIVATE:

				// Страница расфиксирована, виртуальная страница свободна.
				// Страница может быть не отображена. Есть смысл сделать переотображение.
				cost.remap_type (REMAP_PART);

			case PAGE_NOT_COMMITTED:

				// Виртуальная страница свободна
				// В любом случае, мы просто выделяем новую страницу.
				cost += PAGE_ALLOCATE_COST;

				break;

			case PAGE_DECOMMITTED:

				// Виртуальная страница занята

				// Без полного переотображения, мы должны отобразить ее, как copy-on-write.
				// При записи в нее, она будет скопирована системой во вновь выделенную
				// физическую страницу.

				cost [REMAP_NONE] += PAGE_SIZE + PAGE_ALLOCATE_COST;
				cost [REMAP_PART] += PAGE_SIZE + PAGE_ALLOCATE_COST;

				// При полном переотображении, мы только выделим новую страницу.

				cost [REMAP_FULL] += PAGE_ALLOCATE_COST;

				break;

			case PAGE_MAPPED_PRIVATE:

				// Страница отображена и не разделена.
				// При полном переотображении надо скопировать dst_bytes.

				cost [REMAP_FULL] += dst_bytes;

				break;

			case PAGE_COPIED:

				// Страница не отображена, виртуальная страница занята.
				// Если будет переотображение, оно должно быть полным.
				cost.remap_type (REMAP_FULL);

			case PAGE_COPIED | PAGE_VIRTUAL_PRIVATE:

				// Страница не отображена.
				cost.remap_type (REMAP_PART);

				// При любом переотображении, страницу надо скопировать.
				cost [REMAP_PART] += dst_bytes;
				cost [REMAP_FULL] += dst_bytes;

				break;

			default:
				assert (false);
			}

		} else {

			// Страница вне области копирования
			if (PAGE_COMMITTED & page_state) {

				// При полном переотображении копируем любую зафиксированную страницу.

				cost [REMAP_FULL] += PAGE_SIZE;

				if (PAGE_MAPPED_SHARED == page_state)

					// Страница разделена, при переотображении будет выделена новая.
					cost [REMAP_FULL] += PAGE_ALLOCATE_COST;

				else if ((page_state & PAGE_COPIED) == PAGE_COPIED) {

					// Страница не отображена, переотображение имеет смысл.

					// При частичном переотображении она должна быть скопирована.
					cost [REMAP_PART] += PAGE_SIZE;

					// Тип переотображения зависит от занятости виртуальной страницы.
					cost.remap_type ((PAGE_VIRTUAL_PRIVATE & page_state) ? REMAP_PART : REMAP_FULL);
				}
			}
		}

		++page_state_ptr;
		++page;
	} while (page < line_end);
}

void WinMemory::commit_one_line (Page* begin, Page* end, LineState& line_state, UWord zero_init)
{
	assert (line_state.m_allocation_protect); // not free

	Line* begin_line = Line::begin (begin);

	assert (Line::end (end) == begin_line + 1);

	map (begin_line);

	Octet* page_state_ptr = line_state.m_page_states + (UWord)begin / PAGE_SIZE % PAGES_PER_LINE;
	Octet* page_state_end = line_state.m_page_states + (UWord)end / PAGE_SIZE % PAGES_PER_LINE;

	do {

		if (PAGE_NOT_COMMITTED == *page_state_ptr) {

			// These pages are not committed anywhere

			// Calculate region size
			UWord size = PAGE_SIZE;
			for (;;) {
				if (++page_state_ptr >= page_state_end)
					break;
				if (PAGE_NOT_COMMITTED != *page_state_ptr)
					break;
				size += PAGE_SIZE;
			}

			// Commit in this line
			if (!VirtualAlloc (begin, size, MEM_COMMIT, WIN_WRITE_MAPPED_PRIVATE))
				throw NO_MEMORY ();

			// Disable access at other shared lines
			UWord offset = (begin - begin_line->pages) * PAGE_SIZE;
			for (
				Line* shared_line = logical_line (begin_line).shared_next ();
				shared_line != begin_line;
				shared_line = logical_line (shared_line).shared_next ()
				)
				decommit_surrogate ((Page*)(shared_line->pages->bytes + offset), size);

			*page_state_ptr = PAGE_MAPPED_PRIVATE;

			begin = (Page*)(begin->bytes + size);

		} else {

			if (PAGE_DECOMMITTED & *page_state_ptr) {

				// Recommit
				DWORD old_protect;
				verify (VirtualProtect (begin, PAGE_SIZE, WIN_WRITE_MAPPED_SHARED, &old_protect));

				// Если страница отображена, система присвоит ей защиту
				// WIN_WRITE_MAPPED_SHARED, иначе - WIN_WRITE_COPIED.

				// Если страница отображена на неразделенную виртуальную страницу,
				// нужно изменить защиту WIN_WRITE_MAPPED_SHARED на WIN_WRITE_MAPPED_PRIVATE.

				if (*page_state_ptr & PAGE_VIRTUAL_PRIVATE) {

					// Виртуальная страница не разделена

					MemoryBasicInformation mbi (begin);

					if (WIN_WRITE_MAPPED_SHARED == mbi.Protect) {
						DWORD old;
						verify (VirtualProtect (begin, PAGE_SIZE, WIN_WRITE_MAPPED_PRIVATE, &old));
						*page_state_ptr = PAGE_MAPPED_PRIVATE;
					} else
						*page_state_ptr = PAGE_COPIED | PAGE_VIRTUAL_PRIVATE;

				} else
					*page_state_ptr = PAGE_COPIED;

				if (zero_init && (WIN_NO_ACCESS == old_protect)) {

					// Zero-init recommitted page

					UWord* pw = (UWord*)begin;
					UWord* page_end = (UWord*)(begin + 1);

					do {
						if (*pw) {
							do {
								*pw = 0;
							} while (++pw < page_end);
							break;
						}
					} while (++pw < page_end);
				}
			}

			++begin;
			++page_state_ptr;
		}
	} while (begin < end);
}

void WinMemory::decommit (Page* begin, Page* end)
{
	Line* begin_line = Line::end (begin);
	Line* end_line = Line::begin (end);

	// Unmap full lines
	for (Line* line = begin_line; line < end_line; ++line)
		unmap (line);

	// Decommit partial lines
	UWord cb;
	if (cb = (begin_line->pages - begin) * PAGE_SIZE)
		decommit_surrogate (begin, cb);
	if (cb = (end - end_line->pages) * PAGE_SIZE)
		decommit_surrogate (end_line->pages, cb);
}

HANDLE WinMemory::mapping (const void* p)
{
	try {
		return logical_line (p).m_mapping;
	} catch (...) {
		return 0;
	}
}

HANDLE WinMemory::new_mapping ()
{
	HANDLE h = CreateFileMapping (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
	if (!h)
		throw NO_MEMORY ();
	return h;
}

bool WinMemory::map (Line* line, HANDLE mapping)
{
	// Check for current stack intersects with line
	if (((void*)&mapping >= line) && ((void*)&mapping < (line + 1))) {
		// Call in separate stack
		MapParam param = {line, mapping};
		call_in_fiber ((FiberMethod)map_in_fiber, &param);
		return param.ret;
	}

	LogicalLine& ll = logical_line (line);
	if (!VirtualAlloc (&ll, sizeof (ll), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();

	bool is_new_mapping;
	if (is_new_mapping = !mapping) {
		if (ll.m_mapping)
			return false;
		mapping = new_mapping ();
	}

	// No remapping possible for this object location.
/*	assert (((void*)(this + 1) <= line) || ((void*)this >= (line + 1))); */

	try {

		MemoryBasicInformation mbi (line);

		assert (MEM_FREE != mbi.State);

		if (MEM_PRIVATE == mbi.Type) {

			if (MEM_RESERVE != mbi.State)
				throw BAD_PARAM ();

			assert (!ll.m_mapping);

			VirtualFree (mbi.AllocationBase, 0, MEM_RELEASE);

			UWord re_reserve = (UWord)line - (UWord)mbi.AllocationBase;
			if (re_reserve)
				verify (VirtualAlloc (mbi.AllocationBase, re_reserve, MEM_RESERVE, mbi.AllocationProtect));

			re_reserve = (UWord)mbi.BaseAddress + mbi.RegionSize - (UWord)(line + 1);
			if (re_reserve)
				verify (VirtualAlloc ((line + 1), re_reserve, MEM_RESERVE, mbi.AllocationProtect));

		} else {

			assert (mbi.AllocationBase == line);

			assert (ll.m_mapping);

			if (mapping != ll.m_mapping)
				unmap (line, ll);
			else if (!UnmapViewOfFile (line))
				throw BAD_PARAM ();
		}

		UWord map_access;
		if (WIN_MASK_WRITE & mbi.AllocationProtect)
			map_access = FILE_MAP_WRITE;
		else
			map_access = FILE_MAP_READ;

		if (!MapViewOfFileEx (mapping, map_access, 0, 0, LINE_SIZE, line))
			throw INTERNAL ();

	} catch (...) {
		if (is_new_mapping)
			CloseHandle (mapping);
		throw;
	}

	if (mapping != ll.m_mapping) {  // if mapping has replaced
		ll.m_mapping = mapping;
		// link in single list element
		ll.m_line_next = ll.m_line_prev = (UShort)line_index (line);
		return true;
	}

	return false;
}

void WinMemory::map_in_fiber (MapParam* param)
{
	param->ret = map (param->line, param->mapping);
}

void WinMemory::map (Line* dst, const Line* src)
{
	UWord src_li = line_index (src);
	LogicalLine& src_ll = logical_line (src_li);

	assert (src_ll.m_mapping);

	bool is_new = map (dst, src_ll.m_mapping);

	// If source mapping is old then pages can be committed elsewhere.
	// In this case, memory state can be MEM_COMMIT, not MEM_RESERVE.
	// So, we must 'decommit' this pages now.

	if (!VirtualFree (dst, PAGES_PER_LINE, MEM_DECOMMIT)) {

		// MEM_DECOMMIT does not work for memory mappings

		Page* begin = dst->pages;
		Page* end = (dst + 1)->pages;
		while (begin < end) {

			MemoryBasicInformation mbi (begin);

			Page* reg_end = mbi.end ();
			if (reg_end > end)
				reg_end = end;

			if (MEM_COMMIT == mbi.State) {
				DWORD old;
				UWord reg_size = (UWord)reg_end - (UWord)begin;
				verify (VirtualProtect (begin, reg_size, PAGE_NOACCESS, &old));
				verify (VirtualAlloc (begin, reg_size, MEM_RESET, PAGE_NOACCESS));
			}

			begin = reg_end;
		}
	}

	if (is_new) {

		// Link lines to cycle list.

		UWord dst_li = line_index (dst);
		LogicalLine& dst_ll = logical_line (dst_li);

		Lock lock (sm_data.critical_section);

		LogicalLine& next_ll = logical_line (dst_ll.m_line_next = src_ll.m_line_next);
		next_ll.m_line_prev = (UShort)dst_li;
		dst_ll.m_line_prev = (UShort)src_li;
		src_ll.m_line_next = (UShort)dst_li;
	}
}

void WinMemory::unmap (Line* line)
{
	MemoryBasicInformation mbi (line);

	assert (MEM_FREE != mbi.State);

	if (MEM_MAPPED == mbi.Type) {

		assert (mbi.AllocationBase == line);

		unmap_and_release (line);

		verify (VirtualAlloc (line, LINE_SIZE, MEM_RESERVE,
			(mbi.AllocationProtect & WIN_MASK_READ) ? WIN_READ_RESERVE : WIN_WRITE_RESERVE));
	}
}

void WinMemory::unmap (Line* line, LogicalLine& ll)
{
	assert (ll.m_mapping);

	if (!UnmapViewOfFile (line))
		throw BAD_PARAM ();

	Lock lock (sm_data.critical_section);

	if (ll.shared_next () == line)  // line not shared
		CloseHandle (ll.m_mapping);   // close mapping
	else {

		// Remove line from list.
		logical_line (ll.m_line_next).m_line_prev = ll.m_line_prev;
		logical_line (ll.m_line_prev).m_line_next = ll.m_line_next;
	}

	ll.m_mapping = 0;
	ll.m_line_next = ll.m_line_prev = 0;
}

HANDLE WinMemory::remap_line (Octet* exclude_begin, Octet* exclude_end, LineState& line_state, Word remap_type)
{
	assert (remap_type != REMAP_NONE);

	HANDLE file_mapping;
	if (REMAP_FULL <= remap_type)
		file_mapping = new_mapping ();
	else {
		file_mapping = mapping (exclude_begin);
		assert (file_mapping);
	}

	try {

		Line* tmp_line = (Line*)MapViewOfFileEx (file_mapping, FILE_MAP_WRITE, 0, 0, LINE_SIZE, 0);
		if (!tmp_line)
			throw NO_MEMORY ();
		Line* line = Line::begin (exclude_begin);
		Page* begin_page = Page::begin (exclude_begin);
		Page* tmp_page = tmp_line->pages;
		Octet* page_state_ptr = line_state.m_page_states;
		Line* line_end = line + 1;
		Page* page = line->pages;

		do {

			if (
				(REMAP_FULL == remap_type)
				?
				(PAGE_COMMITTED & *page_state_ptr)
				:
				(PAGE_COPIED & *page_state_ptr)
				) {

				assert ((REMAP_FULL == remap_type) || (PAGE_VIRTUAL_PRIVATE& *page_state_ptr));

				if ((exclude_end <= page->bytes) || (page < begin_page))
					*tmp_page = *page;
				else {
					Page* page_end = page + 1;
					if (page->bytes < exclude_begin)
						real_copy (page->bytes, exclude_begin, tmp_page->bytes);
					if (page_end->bytes > exclude_end) {
						UWord offset = exclude_end - page->bytes;
						real_copy (exclude_end, page_end->bytes, tmp_page->bytes + offset);
					}
				}

				// Modify page state
				*page_state_ptr = PAGE_MAPPED_PRIVATE;
			}

			++tmp_page;
			++page_state_ptr;
			++page;
		} while (page < line_end->pages);

		map (line, file_mapping);
		UnmapViewOfFile (tmp_line);

		if (REMAP_PART == remap_type) {

			// Возможно, строка содержит decommitted страницы.
			// После map, эти страницы будут доступны.
			// Имитируем decommit, запрещая доступ к этим страницам.

			page_state_ptr = line_state.m_page_states;
			page = line->pages;
			do {

				if (PAGE_DECOMMITTED == (PAGE_COPIED & *page_state_ptr))
					if ((page < begin_page) || (exclude_end <= page->bytes))
						decommit_surrogate (page, PAGE_SIZE);

				++page_state_ptr;
				++page;
			} while (page < line_end->pages);
		}

	} catch (...) {

		if (REMAP_FULL == remap_type)
			CloseHandle (file_mapping);

		throw;
	}

	return file_mapping;
}

NT_TIB* WinMemory::current_tib ()
{
	NT_TIB* ptib;

	__asm {
		mov eax, fs:[18h]
		mov [ptib], eax
	}

	return ptib;
}

void WinMemory::thread_prepare ()
{
	// Prepare stack of current thread to share

	// Obtain stack size
	ShareStackParam param;
	const NT_TIB* ptib = current_tib ();
	param.stack_base = ptib->StackBase;
	param.stack_limit = ptib->StackLimit;

	// Call share_stack in fiber
	verify (ConvertThreadToFiber (0));
	call_in_fiber ((FiberMethod)&WinMemory::stack_prepare, &param);
}

void WinMemory::thread_unprepare ()
{}

void WinMemory::stack_prepare (const ShareStackParam* param_ptr)
{
	// Copy parameter from source stack
	// The source stack will become temporary unaccessible
	ShareStackParam param = *param_ptr;

	// Temporary copy committed stack
	UWord cur_stack_size = (Octet*)param.stack_base - (Octet*)param.stack_limit;
	void* ptmp = VirtualAlloc (0, cur_stack_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!ptmp)
		throw NO_MEMORY ();

	//memcpy (ptmp, param.stack_limit, cur_stack_size);
	real_copy ((const UWord*)param.stack_limit, (const UWord*)param.stack_base, (UWord*)ptmp);

	// Get stack region
	// The first page before param.m_stack_limit is a guard page
	MemoryBasicInformation mbi ((Page*)param.stack_limit - 1);

	assert (MEM_COMMIT == mbi.State);
	assert ((PAGE_GUARD | PAGE_READWRITE) == mbi.Protect);
	assert (PAGE_SIZE == mbi.RegionSize);

	// Decommit Windows memory
	VirtualFree (mbi.BaseAddress, (Octet*)param.stack_base - (Octet*)mbi.BaseAddress, MEM_DECOMMIT);

#ifdef _DEBUG
	{
		MemoryBasicInformation dmbi (mbi.AllocationBase);

		assert (dmbi.State == MEM_RESERVE);
		assert (mbi.AllocationBase == dmbi.AllocationBase);
		assert ((((Octet*)mbi.AllocationBase) + dmbi.RegionSize) == (Octet*)param.stack_base);
	}
#endif // _DEBUG

	Line* stack_begin = mbi.allocation_base ();
	assert (!((UWord)param.stack_base % LINE_SIZE));
	Line* stack_end = (Line*)param.stack_base;
	Line* line = stack_begin;

	try {

		// Map our memory (but not commit)
		do
			map (line);
		while (++line < stack_end);

	} catch (...) {

		// Unmap
		while (line > stack_begin)
			unmap (--line);

		// Commit current stack
		VirtualAlloc (param.stack_limit, cur_stack_size, MEM_COMMIT, PAGE_READWRITE);

		// Commit guard page(s)
		VirtualAlloc (mbi.BaseAddress, mbi.RegionSize, MEM_COMMIT, PAGE_GUARD | PAGE_READWRITE);

		// Release temporary buffer
		VirtualFree (ptmp, 0, MEM_RELEASE);

		throw;
	}

	// Commit current stack
	VirtualAlloc (param.stack_limit, cur_stack_size, MEM_COMMIT, PAGE_READWRITE);

	// Commit guard page(s)
	VirtualAlloc ((Page*)param.stack_limit - 1, mbi.RegionSize, MEM_COMMIT, PAGE_GUARD | PAGE_READWRITE);

	// Copy stack data back
	//memcpy (param.stack_limit, ptmp, cur_stack_size);
	real_copy ((const UWord*)ptmp, (const UWord*)((Octet*)ptmp + cur_stack_size), (UWord*)param.stack_limit);

	// Release temporary buffer
	VirtualFree (ptmp, 0, MEM_RELEASE);

	assert (!memcmp (&param, param_ptr, sizeof (ShareStackParam)));
}

void WinMemory::call_in_fiber (FiberMethod method, void* param)
{
	// Only one thread can use fiber call
	Lock lock (sm_data.critical_section);

	sm_data.fiber_param.source_fiber = GetCurrentFiber ();
	sm_data.fiber_param.method = method;
	sm_data.fiber_param.param = param;
	SwitchToFiber (sm_data.fiber);
	if (sm_data.fiber_param.failed)
		((SystemException*)sm_data.fiber_param.system_exception)->_raise ();
}

void WinMemory::fiber_proc (FiberParam* param)
{
	for (;;) {

		try {
			param->failed = false;
			(*param->method) (param->param);
		} catch (const SystemException& ex) {
			param->failed = true;
			real_copy ((const UWord*)&ex, (const UWord*)(&ex + 1), param->system_exception);
		} catch (...) {
			param->failed = true;

			UNKNOWN ue;
			real_copy ((const UWord*)&ue, (const UWord*)(&ue + 1), param->system_exception);
		}

		SwitchToFiber (param->source_fiber);
	}
}

WinMemory::RemapType WinMemory::CostOfOperation::decide_remap () const
{
	if (m_remap_type & REMAP_FULL) {
		if (m_costs [REMAP_FULL] <= m_costs [REMAP_NONE])
			return REMAP_FULL;
	} else if (m_remap_type & REMAP_PART) {
		if (m_costs [REMAP_PART] <= m_costs [REMAP_NONE])
			return REMAP_PART;
	}
	return REMAP_NONE;
}

void WinMemory::LineRegions::push_back (UWord offset, UWord size)
{
	assert (offset + size <= LINE_SIZE);

	if (m_end > m_regions) {
		if (m_end->offset + m_end->size == offset) {
			m_end->size += (UShort)size;
			return;
		}
		assert ((UWord)m_end->offset + (UWord)m_end->size < offset);
	}

	assert (m_end < m_regions + (sizeof (m_regions) / sizeof (m_regions [1])));

	m_end->offset = (UShort)offset;
	m_end->size = (UShort)size;
	++m_end;
}

}
