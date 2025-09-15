all: libappindicator-gtk3-introspection-12.10.json libappindicator-gtk3-12.10.json libappindicator-gtk2-12.10.json

libappindicator-gtk3-introspection-12.10.json: libappindicator.json.in
	sed -e 's/@GTK_VER@/3/g' -e 's/@INTROSPECTION@/yes/g' $< > $@

libappindicator-gtk3-12.10.json: libappindicator.json.in
	sed -e 's/@GTK_VER@/3/g' -e 's/@INTROSPECTION@/no/g' $< > $@

libappindicator-gtk2-12.10.json: libappindicator.json.in
	sed -e 's/@GTK_VER@/2/g' -e 's/@INTROSPECTION@/no/g' $< > $@