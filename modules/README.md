### A bunch of modules for inclusion in flatpak build recipes                                                                           

#### Note     
This repo is only compatible with flatpak-builder 0.9.3 and later due to some backwards-incompatible changes.     

#### How to use                                                                                                                       
Just add this repo as a submodule:                                                                                                 
```shell                                                                                                                                   
git submodule add https://github.com/casept/flatpak-modules.git modules     
git submodule init         
git submodule update        
```

And add whichever module you need to your main .json under the modules key, for example:    
```json
"modules": [
  "modules/ffmpeg-3.2.4.json",    
  your-own-stuff-here
```
will add FFMPEG as a module.                                                 

