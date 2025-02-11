name = game

build = ./build
bin = $(build)/$(name)

src = ./src
src_files = $(src)/main.cpp

run: build
	$(bin)

build: $(src_files)
	mkdir -p $(build)
	clang -Wall -Werror $^ -lraylib -o $(bin)

clean:
	rm -rf $(build)
