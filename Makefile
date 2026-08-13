run: compile
	./build/tower

clean:
	git clean -fdx

compile: ./build
	cmake --build build

./build:
	cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

.PHONY: clean run compile
