all:
	make -C ./Tracker -f ./makefile
	make -C ./User -f ./makefile

.PHONY: clean
clean:
	make -C ./Tracker -f ./makefile clean
	make -C ./User -f ./makefile clean

