#include <assert.h>
#include <string.h>
#include <windows.h>

// TODO: To make portable version

#define MAXLINELEN 60
#define MAX_TEXT 512

#define FILEINTRO "File: "
#define EXPINTRO "Expression: "

static char* __cat (char* buf, char* buf_end, const char* s, int l = -1)
{
  if (l < 0)
    l = strlen (s);
  int b = buf_end - buf - 1;
  if (l > b)
    l = b;
  if (l > 0) {
    memcpy (buf, s, l * sizeof (char));
    buf += l;
  }
  *buf = '\0';
  return buf;
}

static char* __cat (char* buf, unsigned u)
{
  char* p = buf;
  
  do {
    *(p++) = (u % 10) + '0';
    u /= 10;
  } while (u);
  
  char* ret = p;
  *(p--) = '\0';

  // Invert string
  while (buf < p) {
    char tmp = *buf;
    *buf = *p;
    *p = tmp;
    ++buf;
    --p;
  }

  return ret;
}

extern "C" void __assert (const char *expr, const char *filename, unsigned lineno)
{
  char buf [MAX_TEXT];

  char* p = buf;
  char* end = buf + MAX_TEXT;
  p = __cat (p, end, "Assertion failed!\n\n");
  p = __cat (p, end, FILEINTRO);
  int l = strlen (filename);
  if (l > MAXLINELEN - sizeof (FILEINTRO)) {
    p = __cat (p, end, "...");
    filename += l + sizeof (FILEINTRO) + 3 - MAXLINELEN;
  }
  p = __cat (p, end, filename);
  p = __cat (p, end, "\nLine:");
  p = __cat (p, lineno);
  p = __cat (p, end, "\n" EXPINTRO);
  l = strlen (expr);
  if (l > MAXLINELEN - sizeof (EXPINTRO)) {
    p = __cat (p, end, expr, MAXLINELEN - sizeof (EXPINTRO) - 3);
    p = __cat (p, end, "...");
  }
  p = __cat (p, end, "\n\n");
  
  p = __cat (p, end, "Press Retry to debug");

  int ans = MessageBox (0, buf, "Nirvana", MB_ABORTRETRYIGNORE | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);

  switch (ans) {

  case IDABORT:
    ExitProcess (3);
    break;

  case IDRETRY:
    DebugBreak ();
    break;
  }
}
