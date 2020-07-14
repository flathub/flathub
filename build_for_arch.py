import platform
import os
arch = platform.architecture()
if(arch[0]=='64bit'):
    print("extracting nuclear-fca030.tar.gz")
    os.system('tar --extract -f nuclear-fca030.tar.gz')
    print("creating required dirs")
    os.system('mkdir -p /app/main ')
    print("copying binares to main")
    os.system('cp -r  nuclear-fca030/* /app/main/')
else:
    print('Unsupported arch')
