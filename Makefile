# vim: syntax=make
# Dummy Makefile to get -j value from flatpak-builder and pass it on to
# cabal install
DASH_J = $(filter -j%,$(MAKEFLAGS))
EXTRA = --extra-lib-dirs=/app/lib --extra-include-dirs=/app/include

all: build-.

# make build-foo to build subdirectory foo
# make build-. to build top-level directory
# cabal install -j does not pass -j to ghc!
build-%:
	( \
		cd $* && \
		cabal configure --global $(EXTRA) && \
		cabal build $(DASH_J) && \
		cabal copy && \
		cabal register \
	)

install:

# Don't run rules within this file in parallel
.NOTPARALLEL:
