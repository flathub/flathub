import json
import urllib.request
import aiohttp
import asyncio
import hashlib

def fetch_json(url):
  print(f'Fetching {url}')
  with urllib.request.urlopen(url) as response:
    contents = response.read()
    return json.loads(contents)

def get_packager():
  packager = fetch_json('https://raw.githubusercontent.com/TurboWarp/desktop/master/scripts/packager.json')
  return {
    'type': 'file',
    'url': packager['src'],
    'sha256': packager['sha256'],
    'dest': 'static',
    'dest-filename': 'packager.html'
  }

async def fetch_asset(session, semaphore, md5ext):
  async with semaphore:
    url = f'https://assets.scratch.mit.edu/{md5ext}'
    print(f'Fetching {url}')
    async with session.get(url) as resp:
      content = await resp.content.read()
      sha256 = hashlib.sha256(content).hexdigest()
      return {
        'type': 'file',
        'url': url,
        'sha256': sha256,
        'dest': 'library-files',
        'dest-filename': md5ext
      }

async def get_assets():
  md5exts = set()
  def add_asset(md5ext):
    md5exts.add(md5ext)
  for costume in fetch_json('https://raw.githubusercontent.com/TurboWarp/scratch-gui/develop/src/lib/libraries/costumes.json'):
    add_asset(costume['md5ext'])
  for sound in fetch_json('https://raw.githubusercontent.com/TurboWarp/scratch-gui/develop/src/lib/libraries/sounds.json'):
    add_asset(sound['md5ext'])
  for backdrop in fetch_json('https://raw.githubusercontent.com/TurboWarp/scratch-gui/develop/src/lib/libraries/backdrops.json'):
    add_asset(backdrop['md5ext'])
  for sprite in fetch_json('https://raw.githubusercontent.com/TurboWarp/scratch-gui/develop/src/lib/libraries/sprites.json'):
    for costume in sprite['costumes']:
      add_asset(costume['md5ext'])
    for sound in sprite['sounds']:
      add_asset(sound['md5ext'])
  md5exts = sorted(md5exts)

  async with aiohttp.ClientSession() as session:
    semaphore = asyncio.Semaphore(20)
    tasks = [asyncio.ensure_future(fetch_asset(session, semaphore, md5ext)) for md5ext in md5exts]
    return await asyncio.gather(*tasks)

sources = [get_packager()] + asyncio.run(get_assets())

with open('asset-sources.json', 'w') as f:
  f.write(json.dumps(sources, indent=4))
