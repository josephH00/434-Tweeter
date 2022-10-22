all:
	make -C ./Tracker -f ./makefile
	make -C ./User -f ./makefile

debug:
	make -C ./Tracker -f ./makefile debug=1
	make -C ./User -f ./makefile debug=1

.PHONY: clean
clean:
	make -C ./Tracker -f ./makefile clean
	make -C ./User -f ./makefile clean

