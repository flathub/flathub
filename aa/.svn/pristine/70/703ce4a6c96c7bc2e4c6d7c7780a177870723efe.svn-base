/*
Copyright (C) 2010 COR Entertainment

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// Hash table interface
#ifndef __H_HASHTABLE
#define __H_HASHTABLE

#include "game/q_shared.h"



/*=============================================*
 * Hash table types                            *
 *=============================================*/

/* Hash table (opaque type) */
struct hashtable_s;
typedef struct hashtable_s * hashtable_t;

/* Function pointer for HT_Apply */
typedef qboolean ( * ht_apply_funct )( void * item , void * extra );


/*=============================================*
 * Hash table flags                            *
 *=============================================*/

/* Items are stored inside the table */
#define HT_FLAG_INTABLE		( 1 << 0 )
/* Free items on table destruction */
#define HT_FLAG_FREE		( 1 << 1 )
/* Keys are case-sensitive */
#define HT_FLAG_CASE		( 1 << 2 )
/* Iteration is sorted by key */
#define HT_FLAG_SORTED		( 1 << 3 )



/*=============================================*
 * Hash table functions                        *
 *=============================================*/

/*
 * Hash table creation
 *
 * Parameters:
 *	size		size of the table (will be rounded up to the next
 *			prime number)
 *	flags		combination of HT_FLAG_* for the table
 *	item_size	size of the table's items
 *	key_offset	offset of the key in the table's items
 *	key_length	maximal length of the key in the table; if 0, the key
 *			will be accessed as a pointer instead of an array
 */
hashtable_t HT_Create(
		size_t		size ,
		unsigned int	flags ,
		size_t		item_size ,
		size_t		key_offset ,
		size_t		key_length
	);


/*
 * Macro that determines the offset of a field in a structure
 */
#define HT_OffsetOfField(TYPE,FIELD) \
	( (char *)( &( ( (TYPE *) NULL )->FIELD ) ) - (char *) NULL )


/*
 * Hash table destruction
 */
void HT_Destroy(
		hashtable_t	table
	);


/*
 * Gets an item from the table.
 *
 * Parameters:
 *	table		the hash table to access
 *	key		the key to look up
 *	create		pointer to a boolean which will be set to true if
 *			the item was created; if NULL, no creation will take
 *			place
 */
void * HT_GetItem(
		hashtable_t	table ,
		const char *	key ,
		qboolean *	created
	);


/*
 * Stores an item into the table
 *
 * Parameters:
 *	table			the hash table to add an item to
 *	item			the item to add to the table
 *	allow_replacement	whether replacement of a previous item
 *				is allowed
 *
 * Returns:
 *	the item that matched the specified key, or NULL if no item
 *	using the same key existed
 *
 * Note:
 *	Replacement behaviour varies greatly depending on the flags.
 *	If the items are stored in-table, or if the table must free the
 *	memory they use, a replacement will still return NULL, as the
 *	memory will have been freed or reused.
 */
void * HT_PutItem(
		hashtable_t	table ,
		void *		item ,
		qboolean	allow_replacement
	);


/*
 * Deletes an item from the table
 *
 * Parameters:
 *	table			the hash table from which an item is to be
 *				deleted
 *	key			the key to delete
 *	found			a pointer to a pointer which will be set to
 *				the deleted item's value; may be NULL
 *
 * Returns:
 *	true if an item was deleted, false otherwise
 *
 * Note:
 *	If the items are stored in-table or are freed automatically, then
 *	the "found" parameter will always be ignored.
 */
qboolean HT_DeleteItem(
		hashtable_t	table ,
		const char *	key ,
		void **		found
	);


/*
 * Applies a function to all items in the table.
 *
 * Parameters:
 *	table			the hash table onto which the function is to
 *				be applied
 *	function		pointer to the function to apply
 *	data			extra data to pass as the function's second
 *				parameter
 *
 * Notes:
 *	The order in which the function is applied is either the insertion
 *	order or, if the table has HT_FLAG_SORTED set, the increasing key
 *	order.
 *	The function should return false if processing is to stop.
 */
void HT_Apply(
		hashtable_t	table ,
		ht_apply_funct	function ,
		void *		data
	);



#endif // __H_HASHTABLE
