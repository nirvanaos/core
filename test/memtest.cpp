#include <nirvana.h>
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

using namespace Nirvana;

//#define memory system_memory

void stack_test (int n)
{
  char buf [1024];
  
  strcpy (buf, "test");
  if (n > 0)
    stack_test (n - 1);
}

void memtest ()
{
  char* p1 = (char*)memory ()->allocate (0, 1, 0);
  p1 [0] = 0x11;

  char* p2 = (char*)memory ()->copy (0, p1, 1, 0);
  assert (0x11 == p1 [0]);

  assert (0x11 == p2 [0]);

  p1 [0] = 0x12;
  assert (0x11 == p2 [0]);
  
  p2 [0] = 0x21;
  assert (0x12 == p1 [0]);
  
  char* p3 = (char*)memory ()->copy (0, p1, 1, 0);
  assert (0x12 == p1 [0]);
  assert (0x12 == p3 [0]);
  
  memory ()->release (p1, 1);
  memory ()->release (p2, 1);
  memory ()->release (p3, 1);

  p1 = (char*)memory ()->allocate (0, 1, 0);
  p2 = (char*)memory ()->allocate (0, 1, 0);
  memory ()->release (p2, 1);
  memory ()->release (p1, 1);
  
  stack_test (500);
}