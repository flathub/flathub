#!/bin/sh

architecture_type=$(uname -m)

export PATH=/app/texlive/bin:/app/texlive/bin/${architecture_type}-linux:${PATH}

perl_version=$(perl -e "print substr($^V, 1)")

export LD_LIBRARY_PATH=/app/texlive/lib:/app/texlive/lib/perl5/${perl_version}/${architecture_type}-linux/CORE
export PERL5LIB=/app/texlive/lib/perl5/${perl_version}:/app/texlive/lib/perl5/site_perl/${perl_version}
export ASPELL_CONF='dict-dir /app/share/dicts'


exec texmacs "$@"
