#!/bin/bash
haxelib setup ./haxelib
haxelib install hxcpp
haxelib install lime
haxelib install openfl
haxelib --never install flixel
haxelib install flixel-tools
haxelib install flixel-ui
haxelib install flixel-addons
haxelib install tjson
haxelib install hxjsonast
haxelib install hscript
haxelib install hxcpp-debug-server
haxelib git polymod https://github.com/larsiusprime/polymod.git
haxelib git linc_luajit https://github.com/superpowers04/linc_luajit
haxelib git hscript-ex https://github.com/ianharrigan/hscript-ex
haxelib git discord_rpc https://github.com/Aidan63/linc_discord-rpc
haxelib install hxCodec
haxe -cp ./setup -D analyzer-optimize -main Main --interp
