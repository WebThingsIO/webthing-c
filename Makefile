GCC_BIN ?= $(shell which gcc)
CARGO_BIN ?= $(shell which cargo)

release: clean build

run: build
ifdef ex
	./examples/$(ex)
else
	@echo 'Missing example file name. You probably meant to do something like `make ex=single-thing run`.'
endif

clean:
	$(CARGO_BIN) clean
	rm -f ./examples/single-thing
	rm -f ./examples/multiple-things
	rm -f ./examples/tests

build:
	$(CARGO_BIN) build --release
	$(GCC_BIN) -o ./examples/single-thing ./examples/single-thing.c -Isrc  -L. -l:target/release/libwebthing.so -lpthread
	$(GCC_BIN) -o ./examples/multiple-things ./examples/multiple-things.c -Isrc  -L. -l:target/release/libwebthing.so -lpthread
	$(GCC_BIN) -o ./examples/tests ./examples/tests.c -Isrc  -L. -l:target/release/libwebthing.so