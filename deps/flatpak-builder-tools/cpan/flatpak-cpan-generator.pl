#!/usr/bin/env perl

use v5.14;

use strict;
use warnings;

use Digest::SHA;
require File::Temp;
use File::Temp ();

use Getopt::Long::Descriptive;
use JSON::MaybeXS;
use LWP::UserAgent;
use MetaCPAN::Client;
use Capture::Tiny qw(tee);


sub scan_deps {
  my @deps = grep(/^Successfully installed/, @_);

  for (@deps)
  {
      s/^Successfully installed (\S+).*/$1/;
  }

   @deps
}

sub get_url_sha256 {
  my ($url) = @_;

  my $state = Digest::SHA->new(256);
  my $ua = LWP::UserAgent->new;
  $ua->env_proxy;

  my $resp = $ua->get($url, ':read_size_hint' => 1024,
                      ':content_cb' => sub {
                        my ($data) = @_;
                        $state->add($data);
                      });

  die "Failed to get sha256 of $url: @{[$resp->status_line]}\n" if !$resp->is_success;
  $state->hexdigest;

}
sub get_source_for_dep {
  my ($cpan, $dep, $outdir) = @_;
  my $release_set = $cpan->release({ name => $dep });

  die "Unexpected @{[$release_set->total]} releases for $dep"
    if $release_set->total != 1;
  my $release = $release_set->next;

  my $url = $release->download_url;
  my $sha256 = get_url_sha256 $url;

  {
    type => 'archive',
    url => $url,
    sha256 => $sha256,
    dest => "$outdir/@{[$release->distribution]}",
  };
}

sub write_module_to_file {
  my ($output, $root) = @_;

  my $serializer = JSON::MaybeXS->new(indent => 1, space_after => 1, canonical => 1);
  my $json = $serializer->encode($root);

  open my $fh, '>', $output or die "Could not open $output for writing\n";
  print $fh $json;
  close $fh;
}

sub main {
  my ($opts, $usage) = describe_options(
    'flatpak-cpan-generator %o <packages...>',
    ['output|o=s', 'The generated sources file', { default => 'generated-sources.json' }],
    ['dir|d=s', 'The output directory used inside the sources file', { default => 'perl-libs' }],
    ['help|h', 'Show this screen', { shortcircuit => 1, hidden => 1 }],
  );

  if ($opts->help) {
    print $usage->text;
    exit;
  }

  die "At least one package is required.\n" if @ARGV == 0;

  my $cpan = MetaCPAN::Client->new;

  say '** Installing dependencies with cpanm...';

  my $tmpdir = File::Temp->newdir;
  my ($stdout, $stderr, $exit) = tee {
      system ('cpanm', '-n', '-L', $tmpdir, "--", @ARGV);
  };
  die "cpanm failed with exit status $exit\n" if $exit != 0;

  say '** Scanning dependencies...';

  my @stdout = split "\n", $stdout;
  my @deps = scan_deps @stdout;
  # my @deps = scan_deps 'lib';
  my @sources = ();

  foreach my $dep (@deps) {
    say "** Processing: $dep";
    my $source = get_source_for_dep $cpan, $dep, $opts->dir;
    push @sources, $source;
  }

  push @sources, {
    type => 'script',
    dest => $opts->dir,
    'dest-filename' => 'install.sh',
    commands => [
      "set -e",
      "function make_install {",
      "    mod_dir=\$1",
      "    cd \$mod_dir",
      "    if [ -f 'Makefile.PL' ]; then",
      "        perl Makefile.PL PREFIX=\${FLATPAK_DEST} && make install PREFIX=\${FLATPAK_DEST}",
      "    elif [ -f 'Build.PL' ]; then",
      "        perl Build.PL && ./Build && ./Build install",
      "    else",
      "        echo 'No Makefile.PL or Build.PL found. Do not know how to install this module'",
      "        exit 1",
      "    fi",
      "}",
      map { "(make_install $_->{dest})" } @sources
    ],
  };

  write_module_to_file $opts->output, \@sources;
}

main;
