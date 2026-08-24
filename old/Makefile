t: compile
	./build/tower-t

a: compile
	./build/tower-a

clean:
	git clean -fdx

compile: ./build
	cmake --build build

./build:
	cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

.PHONY: clean t a compile
