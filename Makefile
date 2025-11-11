all: build

CMAKE_FLAGS ?=
UNAME := $(shell uname -s)

build:
	@mkdir -p build
	cd build && cmake $(CMAKE_FLAGS) ..
	cmake --build build -j

.PHONY: install
install:
	cmake --install build --prefix /usr/local

.PHONY: clean
clean:
	rm -rf build

.PHONY: doc
doc:
	@doxygen doc/Doxyfile
ifeq ($(UNAME),Darwin)
	@open doc/html/index.html
endif

.PHONY: test
test:
	@mkdir -p build
	cd build && cmake -DBUILD_TESTING=ON $(CMAKE_FLAGS) ..
	cmake --build build -j
	ctest --test-dir build --output-on-failure
