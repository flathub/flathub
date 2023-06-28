/* cryptohash.c: This file is intended to provide a common interface for 
   hash functions used for cryptographic purposes.

   This file is really just a header, but named .c because it has function
   definitions in it. It will be directly #included, no need to add it to
   a Makefile.
   
   NOTE: As of December 2008, MD5 should be considered "cryptographically
   broken and unsuitable for further use."[1] MD5 is broken as a cryptographic
   hash because it's possible to "reverse" it using brute-force or English-
   dictionary attacks. MD5 is broken as a checksum because techniques have
   been found to malicously altar data and keep the same MD5 result. MD5
   should never be used for anything, ever.
   
   [1] http://www.kb.cert.org/vuls/id/836068
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_MD5


    #include "qcommon/md5.h"
    #define Com_HashString_OneIter Com_MD5HashString
    #define Com_HMACString Com_HMACMD5String
    
    #define CRYPTOHASH_STRETCH_COUNT 500000 
    
    
#else

	#include <stddef.h>
	#include <stdlib.h>
	#include <assert.h>
    #include "qcommon/hmac_sha2.h"
    static char sha512_hmac_output[1024]; //binary
    
    // jit - string version
    static void BinToHex (void *pData, size_t sizeData, char *HexString, size_t sizeOut) // jit
    {
	    int i, Length;
	    unsigned char LeftHalf, RightHalf;
	    const unsigned char *BinData = pData;

	    Length = sizeOut / 2 - 1; // make sure we don't have any buffer overruns

	    if (sizeData < Length)
		    Length = sizeData;

	    for (i = 0; i < Length; i++)
	    {
		    LeftHalf = BinData[i];
		    LeftHalf >>= 4;
		    RightHalf = BinData[i] & 0xF;

		    if (LeftHalf > 9)
			    LeftHalf += 'a' - 10;
		    else
			    LeftHalf += '0';

		    if (RightHalf > 9)
			    RightHalf += 'a' - 10;
		    else
			    RightHalf += '0';

		    *HexString++ = LeftHalf;
		    *HexString++ = RightHalf;
	    }

	    *HexString = '\0';
    }
    
    #define Com_HashString_OneIter(key, keylen, outbuf, outbuflen) {\
        assert (outbuflen >= 64); \
        sha512 (key, keylen, outbuf); \
    }
    #define Com_HMACString(key, keylen, msg, msglen, outbuf, outbuflen) {\
        assert (outbuflen/2-1 < sizeof(sha512_hmac_output)); \
        hmac_sha512 (key, keylen, msg, msglen, sha512_hmac_output, outbuflen/2);\
        BinToHex (sha512_hmac_output, outbuflen/2, outbuf, outbuflen); \
    }
    
    #define CRYPTOHASH_STRETCH_COUNT 50000
    
#endif

//This "stretches" the hash function by iterating it on itself. This isn't 
//really very slow, considering the calculation need only be done when a
//password is being set.

//On a Core i5 at 2.53 Ghz, a hash recalculation causes about a 1/4 second
//hitch with either 2x500,000 MD5 iterations or 2x50,000 SHA iterations. This
//acceptable and even desirable (computational efficiency = evil in this
//case.)
static void Com_HashString (char *key, int keylen, char *outbuf, int outbuflen) {
    char *aux_outbuf1;
    char *aux_outbuf2;
    char *tmp_aux_outbuf;
    int i;
    
    //MD5 version already outputs hex string version, for SHA we'll have to
    //convert at the end. So for SHA we halve outbuflen (binary representation
    //is twice as compact as hexadecimal text.)
#ifdef USE_MD5
    int iter_buflen = outbuflen;
#else
    int iter_buflen = outbuflen/2-1; 
#endif
    
    aux_outbuf1 = malloc(iter_buflen);
    aux_outbuf2 = malloc(iter_buflen);
    memset (aux_outbuf1, 0, iter_buflen);
    memset (aux_outbuf2, 0, iter_buflen);
    
    Com_HashString_OneIter (key, keylen, aux_outbuf1, iter_buflen);
    
    for (i = 0; i < CRYPTOHASH_STRETCH_COUNT; i++) {
        Com_HashString_OneIter (aux_outbuf1, iter_buflen, aux_outbuf2, iter_buflen);
        tmp_aux_outbuf = aux_outbuf1;
        aux_outbuf1 = aux_outbuf2; 
        aux_outbuf2 = tmp_aux_outbuf;
    }
    
    //get the results into the output buf
#ifdef USE_MD5
    Com_HashString_OneIter (aux_outbuf1, iter_buflen, outbuf, outbuflen);
#else
    BinToHex (aux_outbuf1, iter_buflen, outbuf, outbuflen);
#endif
    
    free (aux_outbuf1);
    free (aux_outbuf2);
}
