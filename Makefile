run: compile
	./build/tower

clean:
	git clean -fdx

compile: ./build
	meson compile -C build

./build:
	meson setup build

.PHONY: clean run compile
