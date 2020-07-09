import platform
import os
arch = platform.architecture()
if(arch[0]=='64bit'):
    os.system('tar --extract -f nuclear-fca030.tar.gz')
    os.system('mkdir -p /app/main /app/bin /app/share/metainfo/  /app/share/applications/')
    os.system('cp -r  nuclear-fca030/* /app/main/')
else:
    print('Unsupported arch')