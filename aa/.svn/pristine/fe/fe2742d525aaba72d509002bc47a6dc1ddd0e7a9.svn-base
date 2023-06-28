#ifdef _DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#define MEG 1048576

static long cnt = 0;
static long chunk = 0;
static long total = 0;
static long cur = 0;
static long peak = 0;
static long next = MEG;

void *GetMemory(long size)
    {
    cnt++;

    if(size > chunk)
        chunk = size;

    cur += size;
    total += size;

    if(peak < cur)
        peak = cur;

    if(next <= peak)
        {
        printf("\n%d Megs currently\n", peak / MEG);

        next += MEG;
        }

    return malloc(size);
    }

void FreeMemory(void *p)
    {
    cur -= _msize(p);

    free(p);
    }

void FinalReport()
    {
    printf("%15ld Chunks.\n", cnt);
    printf("%15ld Biggest chunk.\n", chunk);
    printf("%15ld Average chunk.\n", total / cnt);
    printf("%15ld Total memory.\n", total);
    printf("%15ld Peak memory.\n", peak);

    getchar();
    }

#endif