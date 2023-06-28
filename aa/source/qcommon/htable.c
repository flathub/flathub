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


#include "qcommon.h"
#include "htable.h"


/*=============================================*
 * Type definitions                            *
 *=============================================*/


/*
 * "List heads" - used to store various lists
 */

struct listhead_t
{
	struct listhead_t *	previous;
	struct listhead_t *	next;
};


/*
 * Resets a list head
 */
#define RESET_LIST(LPTR) \
	( (LPTR)->previous = (LPTR)->next = (LPTR) )


/*
 * Table entry - used to store actual items
 */

struct tentry_t
{
	/* Entry in one of the table's sub-lists */
	struct listhead_t	loc_list;

	/* Entry in the table's main list */
	struct listhead_t	full_list;

	/* Cached hash value */
	unsigned int		hash;
};


/*
 * Function pointers
 */

typedef unsigned int ( * getkey_t )( const char * key );
typedef char * ( * keyfromentry_t )( struct tentry_t * entry , size_t key_offset );
typedef int ( * comparekey_t )( const char * k1 , const char * k2 );


/*
 * Main hash table structure
 */
struct hashtable_s
{
	/* Actual size of the table */
	size_t			size;

	/* Table flags */
	unsigned int		flags;

	/* Item size */
	size_t			item_size;

	/* Key offset in an item */
	size_t			key_offset;

	/* Length of key (0 for pointer) */
	size_t			key_length;

	/* Functions */
	getkey_t		GetKey;
	keyfromentry_t		KeyFromEntry;
	comparekey_t		CompareKey;

	/* List of all items */
	struct listhead_t	all_items;
};


/*
 * Macro that finds the first list head after a table's main structure
 */
#define TABLE_START(TABLE) \
	( (struct listhead_t *)( ((char*)(TABLE)) + sizeof( struct hashtable_s ) ) )


/*=============================================*
 * Internal functions prototypes               *
 *=============================================*/

/* Checks if a size is a prime number */
static qboolean _HT_IsPrime( size_t n );

/* Finds the next higher prime number */
static size_t _HT_NextPrime( size_t n );

/* Computes a string's hash key (case insensitive) */
static unsigned int _HT_GetCIKey( const char * key );

/* Computes a string's hash key (case sensitive) */
static unsigned int _HT_GetKey( const char * key );

/* Returns a table entry's key (in-table items, fixed size key) */
static char * _HT_KeyFromEntryII( struct tentry_t * entry , size_t key_offset );

/* Returns a table entry's key (in-table items, pointer key) */
static char * _HT_KeyFromEntryIP( struct tentry_t * entry , size_t key_offset );

/* Returns a table entry's key (external items, fixed size key) */
static char * _HT_KeyFromEntryPI( struct tentry_t * entry , size_t key_offset );

/* Returns a table entry's key (external items, pointer key) */
static char * _HT_KeyFromEntryPP( struct tentry_t * entry , size_t key_offset );

/* Allocate and initialise a table entry. */
static struct tentry_t * _HT_CreateEntry( hashtable_t table , unsigned int hash , struct listhead_t * list_entry , const char * key );

/* Insert a table entry into a table's global list */
static void _HT_InsertInGlobalList( hashtable_t table , struct tentry_t * t_entry , const char * key );


/*=============================================*
 * Hash table public functions                 *
 *=============================================*/


hashtable_t HT_Create(
		size_t		size ,
		unsigned int	flags ,
		size_t		item_size ,
		size_t		key_offset ,
		size_t		key_length
	)
{
	hashtable_t		table;
	size_t			real_size;
	struct listhead_t *	t_item;

	// Allocate table
	real_size = _HT_NextPrime( size );
	table = Z_Malloc( sizeof( struct hashtable_s ) + real_size * sizeof( struct listhead_t ) );
	assert( table );

	// Initialise main table fields
	table->size = real_size;
	table->flags = flags;
	table->item_size = item_size;
	table->key_offset = key_offset;
	table->key_length = key_length;
	RESET_LIST( &table->all_items );

	// Set functions
	table->GetKey = ( flags & HT_FLAG_CASE ) ? _HT_GetKey : _HT_GetCIKey;
	table->CompareKey = ( flags & HT_FLAG_CASE ) ? strcmp : Q_strcasecmp;
	if ( ( flags & HT_FLAG_INTABLE ) == 0 ) {
		table->KeyFromEntry = key_length ? _HT_KeyFromEntryPI : _HT_KeyFromEntryPP;
	} else {
		table->KeyFromEntry = key_length ? _HT_KeyFromEntryII : _HT_KeyFromEntryIP;
	}

	// Initialise table entries
	t_item = TABLE_START( table );
	while ( real_size > 0 ) {
		RESET_LIST( t_item );
		t_item ++, real_size --;
	}

	return table;
}


void HT_Destroy(
		hashtable_t	table
	)
{
	qboolean		del_key;
	struct listhead_t *	list_head;
	struct listhead_t *	list_entry;
	struct tentry_t *	t_entry;

	del_key =  ( table->key_length == 0 && ( table->flags & ( HT_FLAG_INTABLE | HT_FLAG_FREE ) ) != 0 );
	list_head = &( table->all_items );
	list_entry = list_head->next;
	while ( list_entry != list_head ) {
		t_entry = (struct tentry_t *)( ( (char *) list_entry ) - HT_OffsetOfField( struct tentry_t , full_list )  );
		list_entry = list_entry->next;

		if ( del_key )
			Z_Free( table->KeyFromEntry( t_entry , table->key_offset ) );
		if ( ( table->flags & ( HT_FLAG_INTABLE | HT_FLAG_FREE ) ) == HT_FLAG_FREE ) {
			void ** data = (void **)( ( (char *) t_entry ) + sizeof( struct tentry_t ) );
			Z_Free( *data );
		}
		Z_Free( t_entry );
	}
	Z_Free( table );
}


void * HT_GetItem(
		hashtable_t	table ,
		const char *	key ,
		qboolean *	created
	)
{
	unsigned int		hash;
	struct listhead_t *	list_head;
	struct listhead_t *	list_entry;
	struct tentry_t *	t_entry;
	void *			data;

	assert( table->key_length == 0 || table->key_length >= strlen( key ) );

	// Try finding the item
	hash = table->GetKey( key );
	list_head = ( TABLE_START( table ) + ( hash % table->size ) );
	list_entry = list_head->next;
	while ( list_entry != list_head ) {
		t_entry = ( struct tentry_t * ) list_entry;

		if ( t_entry->hash > hash )
			break;

		if ( t_entry->hash == hash ) {
			char * item_key = table->KeyFromEntry( t_entry , table->key_offset );
			if ( ! table->CompareKey( key , item_key ) ) {
				data = (void *)( ( (char *)t_entry ) + sizeof( struct tentry_t ) );
				if ( created != NULL )
					*created = false;
				return ( table->flags & HT_FLAG_INTABLE ) ? data : ( *(void**)data );
			}
		}

		list_entry = list_entry->next;
	}

	// Check if we can create the entry
	if ( created == NULL )
		return NULL;

	// Create entry
	*created = true;
	t_entry = _HT_CreateEntry( table , hash , list_entry , key );

	// Initialise data
	data = (void *)( ( (char *)t_entry ) + sizeof( struct tentry_t ) );
	if ( ( table->flags & HT_FLAG_INTABLE ) == 0 ) {
		*(void **) data = Z_Malloc( table->item_size );
		data = *(void **) data;
	}
	memset( data , 0 , table->item_size );

	// Copy key
	if ( table->key_length == 0 ) {
		char ** key_ptr = (char **)( ( (char*) data ) + table->key_offset );
		*key_ptr = Z_Malloc( strlen( key ) + 1 );
		strcpy( *key_ptr , key );
	} else {
		char * key_ptr = ( (char*) data ) + table->key_offset;
		strcpy( key_ptr , key );
	}

	return data;
}


void * HT_PutItem(
		hashtable_t	table ,
		void *		item ,
		qboolean	allow_replacement
	)
{
	void *			ret_val = NULL;
	void *			prev_entry = NULL;
	const char *		insert_key;
	unsigned int		hash;
	struct listhead_t *	list_head;
	struct listhead_t *	list_entry;
	struct tentry_t *	t_entry;

	// Extract item key
	if ( table->key_length ) {
		insert_key = ( (const char *) item ) + table->key_offset;
	} else {
		insert_key = *(const char **)( ( (char *) item ) + table->key_offset );
	}

	// Try finding an item with that key, or the new item's location
	hash = table->GetKey( insert_key );
	list_head = ( TABLE_START( table ) + ( hash % table->size ) );
	list_entry = list_head->next;
	while ( list_entry != list_head ) {
		t_entry = ( struct tentry_t * ) list_entry;

		if ( t_entry->hash > hash )
			break;

		if ( t_entry->hash == hash ) {
			const char * item_key = table->KeyFromEntry( t_entry , table->key_offset );
			int cres = table->CompareKey( insert_key , item_key );
			if ( ! cres ) {
				prev_entry = ( ( (char *)t_entry ) + sizeof( struct tentry_t ) );
				ret_val = ( table->flags & HT_FLAG_INTABLE ) ? prev_entry : ( *(void**)prev_entry );
				if ( ! allow_replacement )
					return ret_val;
				break;
			} else if ( cres > 0 ) {
				break;
			}
		}

		list_entry = list_entry->next;
	}

	if ( ret_val != NULL ) {
		// Delete previous item's key if it was a pointer and either
		// items are in-table or should be freed automatically
		if ( table->key_length == 0 && ( table->flags & ( HT_FLAG_INTABLE | HT_FLAG_FREE ) ) != 0 )
			Z_Free( table->KeyFromEntry( t_entry , table->key_offset ) );

		if ( ( table->flags & HT_FLAG_INTABLE ) != 0 ) {
			// Copy item data
			memcpy( prev_entry , item , table->item_size );
			ret_val = NULL;
		} else {
			if ( ( table->flags & HT_FLAG_FREE ) != 0 ) {
				// Free previous item
				Z_Free( ret_val );
				ret_val = NULL;
			}
			*(void **) prev_entry = item;
		}
	} else {
		void * data;

		t_entry = _HT_CreateEntry( table , hash , list_entry , insert_key );
		data = (void *)( ( (char *)t_entry ) + sizeof( struct tentry_t ) );
		if ( ( table->flags & HT_FLAG_INTABLE ) != 0 ) {
			memcpy( data , item , table->item_size );
		} else {
			*(void **) data = item;
		}
	}

	return ret_val;
}


qboolean HT_DeleteItem(
		hashtable_t	table ,
		const char *	key ,
		void **		found
	)
{
	unsigned int		hash;
	struct listhead_t *	list_head;
	struct listhead_t *	list_entry;
	struct tentry_t *	t_entry;
	void *			data = NULL;

	// Try finding the item
	hash = table->GetKey( key );
	list_head = ( TABLE_START( table ) + ( hash % table->size ) );
	list_entry = list_head->next;
	while ( list_entry != list_head ) {
		t_entry = ( struct tentry_t * ) list_entry;

		if ( t_entry->hash > hash )
			break;

		if ( t_entry->hash == hash ) {
			char * item_key = table->KeyFromEntry( t_entry , table->key_offset );
			if ( ! table->CompareKey( key , item_key ) ) {
				data = (void *)( ( (char *)t_entry ) + sizeof( struct tentry_t ) );
				data = ( table->flags & HT_FLAG_INTABLE ) ? data : ( *(void**)data );
				break;
			}
		}

		list_entry = list_entry->next;
	}

	// Did we find it?
	if ( data == NULL ) {
		if ( found != NULL )
			*found = NULL;
		return false;
	}

	// Detach it from its lists
	t_entry->loc_list.previous->next = t_entry->loc_list.next;
	t_entry->loc_list.next->previous = t_entry->loc_list.previous;
	t_entry->full_list.previous->next = t_entry->full_list.next;
	t_entry->full_list.next->previous = t_entry->full_list.previous;

	// Delete key
	if ( table->key_length == 0 && ( table->flags & ( HT_FLAG_INTABLE | HT_FLAG_FREE ) ) != 0 )
		Z_Free( table->KeyFromEntry( t_entry , table->key_offset ) );

	// Delete item
	if ( ( table->flags & ( HT_FLAG_INTABLE | HT_FLAG_FREE ) ) == HT_FLAG_FREE )
		Z_Free( data );

	// Delete entry
	Z_Free( t_entry );

	// Set found pointer
	if ( found != NULL ) {
		if ( ( table->flags & ( HT_FLAG_INTABLE | HT_FLAG_FREE ) ) != 0 )
			data = NULL;
		*found = data;
	}

	return true;
}


void HT_Apply(
		hashtable_t	table ,
		ht_apply_funct	function ,
		void *		data
	)
{
	struct listhead_t *	list_head;
	struct listhead_t *	list_entry;

	list_head = &( table->all_items );
	list_entry = list_head->next;
	while ( list_entry != list_head ) {
		void * item;
		item = ( (char *) list_entry ) - HT_OffsetOfField( struct tentry_t , full_list ) + sizeof( struct tentry_t );
		list_entry = list_entry->next;

		if ( ( table->flags & HT_FLAG_INTABLE ) == 0 )
			item = *(void**) item;
		if ( ! function( item , data ) )
			return;
	}
}


/*=============================================*
 * Functions related to prime numbers          *
 *=============================================*/

static qboolean _HT_IsPrime( size_t n )
{
	size_t temp;
	size_t nsq;
	size_t inc;

	if ( n == 0 )
		return false;

	nsq = ceil( sqrt( (double)n ) );
	for ( inc = 1 , temp = 2 ; temp <= nsq ; temp += inc ) {
		if ( n % temp == 0 )
			return false;
		if ( temp == 3 )
			inc = 2;
	}
	return true;
}


static size_t _HT_NextPrime( size_t n )
{
	size_t value = n;
	while ( ! _HT_IsPrime( value ) )
		value ++;
	return value;
}



/*=============================================*
 * Key computation functions                   *
 *=============================================*/


static unsigned int _HT_GetCIKey( const char * key )
{
	const char * current = key;
	unsigned int hash = 111119;

	while ( *current ) {
		hash += (unsigned char)tolower( *current );
		hash += ( hash << 10 );
		hash ^= ( hash >> 6 );
		current ++;
	}

	hash += ( hash << 3 );
	hash ^= ( hash >> 11 );
	hash += ( hash << 15 );

	return hash;
}


static unsigned int _HT_GetKey( const char * key )
{
	const char * current = key;
	unsigned int hash = 111119;

	while ( *current ) {
		hash += (unsigned char) *current;
		hash += ( hash << 10 );
		hash ^= ( hash >> 6 );
		current ++;
	}

	hash += ( hash << 3 );
	hash ^= ( hash >> 11 );
	hash += ( hash << 15 );

	return hash;
}



/*=============================================*
 * Key retrieval                               *
 *=============================================*/


static char * _HT_KeyFromEntryII( struct tentry_t * entry , size_t key_offset )
{
	void * item_addr;
	item_addr = (void*)( ( (char*)entry ) + sizeof( struct tentry_t ) );
	return (char *)( ( (char*)item_addr ) + key_offset );
}

static char * _HT_KeyFromEntryIP( struct tentry_t * entry , size_t key_offset )
{
	void * item_addr;
	item_addr = (void*)( ( (char*)entry ) + sizeof( struct tentry_t ) );
	return *(char **)( ( (char*)item_addr ) + key_offset );
}

static char * _HT_KeyFromEntryPI( struct tentry_t * entry , size_t key_offset )
{
	void * item_addr;
	item_addr = *(void**)( ( (char*)entry ) + sizeof( struct tentry_t ) );
	return (char *)( ( (char*)item_addr ) + key_offset );
}

static char * _HT_KeyFromEntryPP( struct tentry_t * entry , size_t key_offset )
{
	void * item_addr;
	item_addr = *(void**)( ( (char*)entry ) + sizeof( struct tentry_t ) );
	return *(char **)( ( (char*)item_addr ) + key_offset );
}



/*=============================================*
 * Other internal functions                    *
 *=============================================*/


static struct tentry_t * _HT_CreateEntry(
	hashtable_t		table ,
	unsigned int		hash ,
	struct listhead_t *	list_entry ,
	const char *		key )
{
	// Allocate new entry
	struct tentry_t * t_entry;
	size_t	entry_size = sizeof( struct tentry_t );
	entry_size += ( table->flags & HT_FLAG_INTABLE ) ? table->item_size : sizeof( void * );
	t_entry = Z_Malloc( entry_size );
	t_entry->hash = hash;

	// Add entry to local list
	t_entry->loc_list.previous = list_entry->previous;
	t_entry->loc_list.next = list_entry;
	list_entry->previous = t_entry->loc_list.previous->next = &( t_entry->loc_list );

	_HT_InsertInGlobalList( table , t_entry , key );

	return t_entry;
}


static void _HT_InsertInGlobalList( hashtable_t table , struct tentry_t * t_entry , const char * key )
{
	if ( ( table->flags & HT_FLAG_SORTED ) == 0 ) {
		// Append to global list
		t_entry->full_list.previous = table->all_items.previous;
		t_entry->full_list.next = &( table->all_items );
		table->all_items.previous = t_entry->full_list.previous->next = &( t_entry->full_list );
	} else {
		// Global list must be kept sorted, find insert location
		struct listhead_t * list_entry = table->all_items.next;
		while ( list_entry != &( table->all_items ) ) {
			struct tentry_t * ai_entry;
			const char * ai_key;
			int cres;

			ai_entry = (struct tentry_t *)( ( (char *) list_entry ) - HT_OffsetOfField( struct tentry_t , full_list )  );
			ai_key = table->KeyFromEntry( ai_entry , table->key_offset );
			cres = table->CompareKey( ai_key , key );

			assert( cres != 0 );
			if ( cres > 0 )
				break;
			list_entry = list_entry->next;
		}
		t_entry->full_list.previous = list_entry->previous;
		t_entry->full_list.next = list_entry;
		list_entry->previous = t_entry->full_list.previous->next = &( t_entry->full_list );
	}
}
