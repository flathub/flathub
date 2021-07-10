# frozen_string_literal: true

require 'optparse'
require 'net/http'
require 'uri'
require 'json'

# %s is 'gemname-x.y.z.gem'
# ex. 'rails-3.2.1.gem'
GEM_URL = 'https://rubygems.org/gems/%s'

# %s is gemname
GEM_VERSIONS_URL = 'https://rubygems.org/api/v1/versions/%s.json'

RE = /^(.+)-(#{Gem::Version::VERSION_PATTERN}).gem$/

def split_filename(filename)
  gemname, version = RE.match(filename).captures
  [gemname, version]
end

def get_file_hash(gemname, version)
  # https://guides.rubygems.org/rubygems-org-api/#gem-version-methods
  uri = URI.parse(GEM_VERSIONS_URL % gemname)
  result = JSON.parse(Net::HTTP.get(uri))
  result.select { |h| h['number'] == version && h['platform'] == 'ruby' }.first['sha']
end

params = { source: nil, out: 'rubygems.json' }
OptionParser.new do |opt|
  opt.on('-s', '--source=SOURCE') { |v| v }
  opt.on('-o', '--out=OUTPUT') { |v| v }
  opt.parse! ARGV, into: params
end

bundle_command = 'bundle install --local'
sources = Dir.glob('*.gem', base: 'vendor/cache').map do |f|
  {
    type: 'file',
    url: GEM_URL % f,
    sha256: get_file_hash(*split_filename(f)),
    dest: 'vendor/cache'
  }
end
sources = [params[:source]] + sources unless params[:source].nil?
main_module = {
  name: 'rubygems',
  buildsystem: 'simple',
  'build-commands' => [bundle_command],
  sources: sources
}

File.write params[:out], JSON.pretty_generate(main_module)
