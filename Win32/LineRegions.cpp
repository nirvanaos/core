#include "LineRegions.h"

void WinMemory::LineRegions::push_back (UWord offset, UWord size)
{
  assert (offset + size <= LINE_SIZE);

  if (m_end > m_regions) {
    if (m_end->offset + m_end->size == offset) {
      m_end->size += size;
      return;
    }
    assert (m_end->offset + m_end->size < offset);
  }

  assert (m_end < m_regions + (sizeof (m_regions) / sizeof (m_regions [1])));

  m_end->offset = offset;
  m_end->size = size;
  ++m_end;
}
