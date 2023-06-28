#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "binheap.h"

#include <assert.h>

// NOTE: I am intentionally not using CRX's custom types here because I want
// to be able to use this code in other codebases, such as aaradiant. -Max

static unsigned int leftchild (unsigned int p)
{
	return 2*p+1;
}

static unsigned int rightchild (unsigned int p)
{
	return 2*p+2;
}

static unsigned int minchild (const binheap_t *h, unsigned int p)
{
	unsigned int c1, c2;
	
	c1 = leftchild (p);
	
	if (c1 >= h->nitems)
		return p;
	
	c2 = rightchild (p);
	
	if (c2 >= h->nitems)
		return c1;
	
	if (h->cmp (h->items[c1], h->items[c2]) > 0)
		return c2;
	
	return c1;
}

static unsigned int parent (unsigned int c)
{
	if (c == 0) return 0;
	return (c-1)/2;
}


// End internal functions, begin public API


void binheap_heapify (binheap_t *h)
{
	unsigned int	p, end, i, c;
	
	end = h->nitems-1;
	
	// since p is unsigned, we rely on integer overflow
	for (p = parent (end); p < h->nitems; p--) 
	{
		i = p;
		
		while (leftchild (i) <= end)
		{
			c = minchild (h, i);
			if (h->cmp (h->items[c], h->items[i]) < 0)
			{
				void *tmp = h->items[i];
				h->items[i] = h->items[c];
				h->items[c] = tmp;
				i = c;
			}
			else
			{
				break;
			}
		}
	}
	
	for (i = h->nitems-1; i > 0; i--)
	{
		p = parent(i);
		h->setidx (h->items[i], i);
		if (h->cmp (h->items[i], h->items[p]) < 0)
			assert (0);
	}
	
	h->setidx (h->items[0], 0);
}

// Remove a node from anywhere in the heap, incl. the middle. Kudos to Jim
// Mischel for explaining how to do this:
// http://stackoverflow.com/questions/8705099/how-to-delete-in-a-heap-data-structure
void binheap_remove (binheap_t *h, void *val)
{
	void			*tmp;
	unsigned int	i, p, c, start = h->getidx (val);
	
	tmp = h->items[--h->nitems];
	
	if (start == h->nitems)
		return;
	
	// Bubble up or bubble down. Only one of these loops should actually run.
	
	i = start;
	p = parent (i);
	
	while (p != i && h->cmp (tmp, h->items[p]) < 0)
	{
		h->items[i] = h->items[p];
		h->setidx (h->items[i], i);
		
		i = p;
		p = parent (i);
	}
	
	if (i == start)
	{
		while (i < h->nitems/2)
		{
			c = minchild (h, i);
			if (h->cmp (h->items[c], tmp) >= 0)
				break;
			h->items[i] = h->items[c];
			h->setidx (h->items[i], i);
			i = c;
		}
	}
	
	// Once we're done, fill the remaining hole
	h->items[i] = tmp;
	h->setidx (h->items[i], i);
}

void binheap_insert (binheap_t *h, void *val)
{
	unsigned int i, p;
	
	i = h->nitems++;
	p = parent (i);
	while (p != i && h->cmp (val, h->items[p]) < 0)
	{
		h->items[i] = h->items[p];
		h->setidx (h->items[i], i);
		
		i = p;
		p = parent (i);
	}
	
	h->items[i] = val;
	h->setidx (h->items[i], i);
}

void *binheap_find_root (const binheap_t *h)
{
	return h->items[0];
}
