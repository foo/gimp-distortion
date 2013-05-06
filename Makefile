install: plugin.c
	gimptool-2.0 --install plugin.c
uninstall:
	gimptool-2.0 --uninstall-bin plugin.c
reinstall: uninstall install
run: 
	gimp checkers.jpeg
