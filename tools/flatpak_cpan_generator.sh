#!/bin/bash

# Get the directory of this script
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# cd to the parent directory
cd $SCRIPT_DIR/..

# Setup perl environment
export PERL_LOCAL_LIB_ROOT=$(pwd)/.temp/perl5
export PERL5LIB=$PERL_LOCAL_LIB_ROOT/lib/perl5
export PERL_MB_OPT="--install_base \"$PERL_LOCAL_LIB_ROOT\""
export PERL_MM_OPT="INSTALL_BASE=$PERL_LOCAL_LIB_ROOT"
export PATH=$PERL_LOCAL_LIB_ROOT/bin:$PATH

# Global options for cpan
export PERL_CPANM_HOME=$(pwd)/.temp/.cpanm
export PERL_CPANM_OPT="--notest"

# Install cpanm
if [ ! -f $PERL_LOCAL_LIB_ROOT/bin/cpanm ]; then
    # Install cpanm
    mkdir -p $PERL_LOCAL_LIB_ROOT/bin
    curl -L https://cpanmin.us/ -o $PERL_LOCAL_LIB_ROOT/bin/cpanm
    chmod +x $PERL_LOCAL_LIB_ROOT/bin/cpanm
else
    cpanm --self-upgrade
fi

# Install and update dependencies
cpanm Getopt::Long::Descriptive
cpanm LWP::Protocol::https
cpanm LWP::UserAgent
cpanm MetaCPAN::Client
cpanm Capture::Tiny

# Generate generated-sources.json
./flatpak-builder-tools/cpan/flatpak-cpan-generator.pl --output=cpan-generated-sources.json YAML::XS
