all:
	true
install:
	mkdir -p /app/Freeplane /app/bin
	cp -ra * /app/Freeplane
	ln -s /app/Freeplane/freeplane.sh /app/bin/freeplane	
