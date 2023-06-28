#ifndef _bsp_memory_h
#define _bsp_memory_h

#ifdef _DEBUG
  void *GetMemory(long size);
  void FinalReport();
  void FreeMemory(void *p);
#else
  #define GetMemory(x) malloc((x))
  #define FreeMemory(x) free((x))
#endif

#endif