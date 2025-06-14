#ifndef LOCAL_H_INCLUDED
#define LOCAL_H_INCLUDED

#define MINIMENU
#define USG
#define GENERATE_LIBRARY_PROTOS
#define	HAS_RENAME
/* Not all linuxes have yacc, but all have bison */
#define YACC "bison -y"

#define TERMLIB       "-ltinfo"

#define BINDIR	"/app/bin"
#define LIBDIR	"/app/lib"
#define MAN1DIR	"/app/man/man1"
#define	MAN45DIR "/app/man/man5"
#define	MAN45EXT ".5"

#endif /* LOCAL_H_INCLUDED */
