#!/bin/sh

architecture_type=$(uname -m)
perl_version=$(ls /app/texlive/lib/perl5/site_perl/)

if ! [[ $perl_version =~ ^[0-9.]+$ ]]; then
  echo " startTeXmacs.sh: Possible error in environment variables."
  echo " "
fi

export PATH=/app/texlive/bin:/app/texlive/bin/${architecture_type}-linux:${PATH}
export LD_LIBRARY_PATH=/app/texlive/lib:/app/texlive/lib/perl5/${perl_version}/${architecture_type}-linux/CORE
export PERL5LIB=/app/texlive/lib/perl5/${perl_version}:/app/texlive/lib/perl5/site_perl/${perl_version}
export ASPELL_CONF='dict-dir /app/share/dicts'


exec texmacs "$@"
