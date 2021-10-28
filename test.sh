#!/bin/bash
PWDD="$(pwd)"
DIR="$HOME/.nuget/packages"
cd $DIR
echo '[' > $PWDD/nuget.json
for i in *; do
	[[ -d $i ]] || continue
	name="$i"
	cd "$i"
	for ii in *; do
		[[ -d $ii ]] || continue
		version="$ii"
		cd "$ii"
		sha512="$(sha512sum *.nupkg | cut -d " " -f 1)"
		cat <<EOF >> $PWDD/nuget.json
    {
        "type": "file",
        "url": "https://api.nuget.org/v3-flatcontainer/$name/$version/$name.$version.nupkg",
        "sha512": "$sha512",
        "dest": "nuget-sources",
        "dest-filename": "$name.$version.nupkg"
    },
EOF
	cd ..
	done
	cd $DIR
done
sed '$d' -i $PWDD/nuget.json
echo '    }' >> $PWDD/nuget.json
echo ']' >> $PWDD/nuget.json
