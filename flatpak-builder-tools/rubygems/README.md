> [!WARNING]
> This is not actively maintained.

Flatpak RubyGems Module Generator
=============================
Tool to generate `flatpak-builder` manifest json from `Gemfile`. This tool uses bundler's `bundle package` subcommand to get gem names and version numbers.

Usage
-----
**REQUIREMENT** Your project's repository contains `Gemfile` (and `Gemfile.lock`) in the repository root.

1. `cd` to the repository root and checkout the correct revision.
2. `bundle install && bundle package` to copy `*.gem` files to `vendor/cache`.
3. `ruby flatpak_rubygems_generator.rb -s source.json -o rubygems.json` generates `rubygems.json` from `vendor/cache`. Part of generated `rubygems.json` is shown below. This manifest includes another source to get `Gemfile`.

   ```json
   {
     "name": "rubygems",
     "buildsystem": "simple",
     "build-commands": [
       "bundle install --local"
     ],
     "sources": [
       "source.json",
       {
         "type": "file",
         "url": "https://rubygems.org/gems/memoist-0.16.0.gem",
         "sha256": "70bd755b48477c9ef9601daa44d298e04a13c1727f8f9d38c34570043174085f",
         "dest": "vendor/cache"
       },
       ...
     ]
   }
   ```
4. Edit `source.json` and specify your project's repository as a source. For example,

   ```json
   {
     "type": "git",
     "url": "git://example.com/repo.git",
     "tag": "X.Y.Z"
   }
   ```
5. Edit main manifest json file like below.

   ```json
   ...
   "modules": [
     {
       "name": "ruby",
       "config-opts": [
         "--disable-install-doc"
       ],
       "sources": [
         {
           "type": "archive",
           "url": "https://cache.ruby-lang.org/pub/ruby/2.3/ruby-2.3.6.tar.gz",
           "sha256": "8322513279f9edfa612d445bc111a87894fac1128eaa539301cebfc0dd51571e"
         }
       ]
     },
     {
       "name": "bundler",
       "buildsystem": "simple",
       "build-commands": [
         "gem install --local bundler-1.16.2.gem"
       ],
       "sources": [
         {
           "type": "file",
           "url": "https://rubygems.org/downloads/bundler-1.16.2.gem",
           "sha256": "3bb53e03db0a8008161eb4c816ccd317120d3c415ba6fee6f90bbc7f7eec8690"
         }
       ]
     },
     "rubygems.json",
     ...
   ],
   ...
   ```
   
