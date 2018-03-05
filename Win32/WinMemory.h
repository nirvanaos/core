// Nirvana Memory Service emulation for Win32

#ifndef _WIN_MEMORY_H_
#define _WIN_MEMORY_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Win32.h"

class WinMemory;

class WinMemInit
{
protected:
  WinMemInit (WinMemory* pmain);
  WinMemInit () {}
};

class WinMemory : private WinMemInit, public Memory
{
public:

  // Memory::
  virtual Pointer allocate (Pointer dst, UWord size, UWord flags);
  virtual Pointer copy (Pointer dst, Pointer src, UWord size, UWord flags);
  virtual void release (Pointer dst, UWord size);
  virtual void commit (Pointer dst, UWord size);
  virtual void decommit (Pointer dst, UWord size);
  virtual Word query (Pointer p, QueryParam q);
  
  ~WinMemory ();
  
protected:

  WinMemory ();

  enum {
    FIXED_HEAP_PTR = 0x400000
  };

private:
  friend class WinMemInit;
  WinMemory (WinMemory* pmain);

  inline void init ();

  class CostOfOperation;
  friend class CostOfOperation;
  class MemoryBasicInformation;
  friend class MemoryBasicInformation;
  class LineRegions;
  
  // Line

  struct LogicalLine
  {
    Line* shared_next () const
    {
      return line_address (m_line_next);
    }

    HANDLE m_mapping;
    UShort m_line_prev, m_line_next;
  };

  friend struct LogicalLine;

  // Win32 protection types for shared and private memory
  
  enum {
    WIN_NO_ACCESS = PAGE_NOACCESS,

    WIN_WRITE_RESERVE = PAGE_READWRITE,
    WIN_READ_RESERVE = PAGE_READONLY,

    WIN_WRITE_MAPPED_PRIVATE = PAGE_EXECUTE_READWRITE,
    WIN_WRITE_MAPPED_SHARED = PAGE_WRITECOPY,
    WIN_WRITE_COPIED = PAGE_READWRITE,
    WIN_READ_MAPPED = PAGE_EXECUTE_READ,
    WIN_READ_COPIED = PAGE_EXECUTE,

    WIN_MASK_WRITE = PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY,
    WIN_MASK_READ = PAGE_READONLY | PAGE_EXECUTE_READ | PAGE_EXECUTE,
    WIN_MASK_COMMITTED = WIN_MASK_READ | WIN_MASK_WRITE,
    WIN_MASK_MAPPED = WIN_WRITE_MAPPED_PRIVATE | WIN_WRITE_MAPPED_SHARED | WIN_READ_MAPPED,
    WIN_MASK_COPIED = WIN_WRITE_COPIED | WIN_READ_COPIED
  };

  // Page states

  enum {
    PAGE_NOT_COMMITTED = 0x00, // Virtual page newer been committed
    PAGE_DECOMMITTED = 0x01,   // Logical page decommitted
    PAGE_MAPPED_SHARED = 0x02, // Logical page mapped to virtual
    PAGE_COPIED = 0x03,        // Logical page write-copied (unmapped)

    PAGE_COMMITTED = 0x02,     // Committed bit

    PAGE_VIRTUAL_PRIVATE = 0x04, // Flag that virtual page not mapped at other lines

    PAGE_MAPPED_PRIVATE = PAGE_MAPPED_SHARED | PAGE_VIRTUAL_PRIVATE
  };

  // Line state information

  struct LineState {
    UWord m_allocation_protect;
    Octet m_page_states [PAGES_PER_LINE];
  };
  
  void get_line_state (const Line* line, LineState& state);

  // Некоторые операции (copy, commit) можно выполнить разными способами.
  // Для выбора оптимального пути необходимо понятие стоимости операции, которую требуется
  // минимизировать. При выполнении операции производится физическое копирование некоторого
  // количества байт и выделение некоторого количества физических страниц.
  // За единицу стоимости принимается стоимость копирования одного байта.
  // Стоимость выделения одной физической страницы принимается эквивалентной
  // копированию PAGE_ALLOCATE_COST байт.
  // При выборе значения этой константы надо учитывать, что:
  // 1. При выделении системой физической страницы производится ее обнуление.
  //    Эта операция примерно в 2 раза быстрее физического копирования страницы.
  // 2. Сама по себе физическая память, хоть и является возобновляемым ресурсом, в отличие
  //    от процессорного времени, но тоже чего-то стоит.

  enum {
    PAGE_ALLOCATE_COST = PAGE_SIZE
  };
  
  // Некоторые страницы в строке могут быть скопированы системой при их изменении
  // (page_state & PAGE_COPIED). Такие страницы не отображены и не могут быть разделены.
  // Для их разделения, всю строку нужно переотобразить (remap). Переотображение может быть
  // полным (REMAP_FULL) или частичным (REMAP_PART). Частичное переотображение возможно
  // если всем copied логическим страницам соответствуют свободные виртуальные страницы
  // (page_state & PAGE_VIRTUAL_PRIVATE).
  // В этом случае, мы должны отобразить текущий mapping по временному адресу,
  // скопировать туда все copied страницы, и вызвать UnmapViewOfFile/MapViewOfFile.
  // В результате все copied страницы отобразятся в разделяемую память, а отображенные 
  // страницы останутся в прежнем состоянии.
  // Если хотя бы одной copied странице соответствует занятая виртуальная страница
  // ((page_state & PAGE_COPIED) && !(PAGE_STATE & PAGE_VIRTUAL_PRIVATE)), частичное
  // переотображение невозможно. В этом случае мы можем выполнить полное переотображение.
  // Для этого надо выделить новый mapping и скопировать в него все committed страницы
  // (page_state & PAGE_COMMITTED).
  // Заметим, что, при любом типе переотображения мы должна переотобразить все copied
  // страницы, иначе их содержимое будет потеряно.

  enum RemapType
  {
    REMAP_NONE = 0x00,
    REMAP_PART = 0x01,
    REMAP_FULL = 0x02
  };

  // Logical space reservation
  
  Line* reserve (UWord size, UWord flags, Line* begin = 0);
  
  // Release memory
  
  void release (Line* begin, Line* end);

  UWord check_allocated (Page* begin, Page* end);
  // Returns bitwise or of allocation protect for all lines.

  // Map logical line to virtual

  bool map (Line* line, HANDLE mapping = 0);
  // line must be allocated.
  // If mapping = 0
  //   if line is mapped,
  //     do nothing
  //   else
  //     allocate new mapping and map
  //     page states will be not committed
  // else
  //   replace current mapping, if exist
  //   Page states are depends from virtual page states.
  // Returns: true if mapping replaced to new.
  // This function is stack-safe. If line is part of current used
  // stack, map_safe will be called through call_in_fiber in separate stack space.

  // Fiber stub for map function
  struct MapParam
  {
    Line* line;
    HANDLE mapping;
    bool ret;
  };

  void map_in_fiber (MapParam* param);

  void map (Line* dst, const Line* src);
  // Map source line to destination.
  // Source line must be mapped.
  // Destination page states are not committed.

  // Unmap logical line

  void unmap (Line* line);
  // If line not mapped, do nothing.
  // Else, line will be decommitted.

  // Unmap and release logical line
  void unmap_and_release (Line* line)
  {
    unmap (line, logical_line (line));
  }
  
  // Assistant function
  void unmap (Line* line, LogicalLine& ll);
  // Logical line must be mapped.
  
  // Get current mapping

  HANDLE mapping (const void* p);  // no throw

  // Create new mapping

  static HANDLE new_mapping ();

  // Remap line

  HANDLE remap_line (Octet* exclude_begin, Octet* exclude_end, LineState& line_state, Word remap_type);
  // Remaps Line::begin (exclude_begin).
  // Allocated data will be preserved excluding region [exclude_begin, exclude_end).
  // Page state in range [exclude_begin, exclude_end) will be undefined.

  // Committing

  void commit (Page* begin, Page* end, UWord zero_init);
  // Carefully: does not validates memory state.
  // Changes protection of pages in range [begin, end) to read-write.

  void commit_line_cost (const Octet* begin, const Octet* end, const LineState& line_state, CostOfOperation& cost, bool copy);
  // Adds cost of commit in range [begin, end) to cost.
  // Cost includes make pages in range [begin, end) private.
  // Если copy = true, функция не учитывает в стоимости переотображения стоимость копирования end - begin байт.
  // Это - режим для подсчета стоимости физического копирования.
  // Для подсчета стоимости фиксации copy = false.

  void commit_one_line (Page* begin, Page* end, LineState& line_state, UWord zero_init);
  // Changes protection of pages in range [begin, end) to read-write

  void decommit (Page* begin, Page* end);
  void decommit_surrogate (Page* begin, UWord size)
  {
    assert (!((UWord)begin % PAGE_SIZE));
    assert (!(size % PAGE_SIZE));
    UWord old;
    verify (VirtualProtect (begin, size, PAGE_NOACCESS, &old));
    verify (VirtualAlloc (begin, size, MEM_RESET, PAGE_NOACCESS));
  }

  // Copying functions

  void copy_one_line (Octet* dst, Octet* src, UWord size, UWord flags);
  bool copy_one_line_aligned (Octet* dst, Octet* src, UWord size, UWord flags);
  void copy_one_line_really (Octet* dst, const Octet* src, UWord size, RemapType remap_type, LineState& dst_state);
  
  // Pointer to line number and vice versa conversions
  
  void check_pointer (const void* p)
  {
    if (p >= m_address_space_end)
      throw BAD_PARAM ();
  }
  
  static UWord line_index (const void* p)
  {
    return (UWord)p / LINE_SIZE;
  }
  
  static Line* line_address (UWord line_index)
  {
    return (Line*)(line_index * LINE_SIZE);
  }
  
  // Get logical line for memory address
  
  LogicalLine& logical_line (const void* p)
  {
    return m_map [line_index (p)];
  }

  LogicalLine& logical_line (UWord line_index)
  {
    return m_map [line_index];
  }

  // Thread stack processing

  static NT_TIB* current_tib ();
  
  void thread_prepare ();
  void thread_unprepare ();

  typedef void (WinMemory::*FiberMethod) (void*);
  
  void call_in_fiber (FiberMethod method, void* param);
  
  struct ShareStackParam
  {
    void* stack_base;
    void* stack_limit;
  };

  void stack_prepare (const ShareStackParam* param);
  void stack_unprepare (const ShareStackParam* param);
  
  struct FiberParam
  {
    WinMemory* obj_ptr;
    void* source_fiber;
    FiberMethod method;
    void* param;
    bool  failed;
    UWord system_exception [sizeof (SystemException) / sizeof (UWord)];
  };
  
  static void CALLBACK fiber_proc (FiberParam* param);

private:  // Data

  Pointer m_address_space_end;

  // Logical lines map
  LogicalLine* m_map;

  // Critical section to lock Logical lines map
  /*
    Блокировка используется только при работе со списком m_line_prev, m_line_next;
    При нормальной работе, каждая линия всегда используется только одним
    доменом исполнения и ее состояние не может быть изменено асинхронно.
    Асинхронно могут меняться только связи в списке линий m_line_prev, m_line_next.
    Также асинхронно может измениться бит PAGE_VIRTUAL_PRIVATE в состояниях страниц
    текущей строки. Это можеть привести к неправильной оценке стоимости операции и
    к неоптимальному ее выполнению, но не нарушит работу системы. С целью сокращения
    количества и времени блокировок, будем считать это допустимым.
  */
  CRITICAL_SECTION m_critical_section;

  class Lock
  {
  public:

    Lock (CRITICAL_SECTION& cs)
      : m_cs (cs)
    {
      EnterCriticalSection (&m_cs);
    }

    ~Lock ()
    {
      LeaveCriticalSection (&m_cs);
    }

  private:
    CRITICAL_SECTION& m_cs;
  };

  // Stack processing fiber
  FiberParam m_fiber_param;
  void* m_fiber;
};

#endif  // _WIN_MEMORY_H_

