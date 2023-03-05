include = -Isrc/include
ldFlags = -lGL -lX11 -lraylib -lenet
ldWinFlags = -Llib -lopengl32 -lraylib -lenet64 -lwinmm
debug = 
optimisation = 

linux:
	make fixobj
	rm -rf ./build/resources
	cp -r ./resources ./build
	cp ./lib/libenet.so.7 ./build/libenet.so.7
	g++ src/main.cpp $(include) $(ldFlags) $(debug) $(optimisation) -Wno-write-strings -o build/PenkInDanger

run:
	cd build && ./PenkInDanger

win:
	make fixobj
	rm -rf ./build/resources
	cp -r ./resources ./build
	x86_64-w64-mingw32-g++ src/main.cpp -mwindows -static $(include) $(ldWinFlags) $(debug) $(optimisation) -Wno-write-strings -o build/PenkInDanger.exe

fixobj:
	python fixobj.py

debug:
	make linux debug=-g
	cd build && gdb ./PenkInDanger

release:
	rm -r ./build/*
	make linux optimisation=-O2
	make win optimisation=-O2
	make separate

separate:
	rm -rf build/windows
	rm -rf build/linux

	mkdir -p build/windows
	mkdir -p build/linux

	cp build/libenet.so.7 build/linux/libenet.so.7
	cp build/PenkInDanger build/linux/PenkInDanger

	cp -r build/resources build/linux/

	cp build/PenkInDanger.exe build/windows/PenkInDanger.exe

	cp -r build/resources build/windows/

	rm -r build/resources
	rm build/PenkInDanger
	rm build/libenet.so.7
	rm build/PenkInDanger.exe