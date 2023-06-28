// For two values that sort identically, the compare function you supply 
// should still return nonzero if you want to support duplicates. But you can
// manipulate the values you return to group duplicates together.
typedef int (*cmpfunc_t) (const void *a, const void *b);

// You must provide these functions to store and retrieve the position of the
// item within the heap.
typedef void (*setidxfunc_t) (void *a, unsigned int idx);
typedef unsigned int (*getidxfunc_t) (const void *a);

typedef struct
{
	cmpfunc_t		cmp;
	setidxfunc_t	setidx;
	getidxfunc_t	getidx;
	unsigned int	nitems;
	void			**items;
} binheap_t;

void binheap_heapify (binheap_t *h);
void binheap_remove (binheap_t *h, void *val);
void binheap_insert (binheap_t *h, void *val);
void *binheap_find_root (const binheap_t *h);
