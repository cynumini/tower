.PHONY: run clean tidy

BIN := ./build/tower

$(BIN): build
	meson compile -C build

run: $(BIN)
	$(BIN)

tidy: build
	clang-tidy -p build main.c

build:
	meson setup build --buildtype=debug
	# meson setup build

clean:
	rm -rf ./build
