REPORTER=dot
BINARY=./lib/leveldb.node
BUILD_CONFIG=./deps/leveldb/build_config.mk
SOURCES=\
	./src/binding \
	./src/binding/batch.cc \
	./src/binding/batch.h \
	./src/binding/binding.cc \
	./src/binding/comparator.cc \
	./src/binding/comparator.h \
	./src/binding/handle.cc \
	./src/binding/handle.h \
	./src/binding/helpers.h \
	./src/binding/iterator.cc \
	./src/binding/iterator.h \
	./src/binding/node_async_shim.h \
	./src/binding/operation.h \
	./src/binding/options.h

COFFEE=./node_modules/.bin/coffee
MOCHA=./node_modules/.bin/mocha

build: $(BINARY)

package: distclean build clean

coffee:
	$(COFFEE) --bare --compile --output lib src

clean:
	node-waf clean

distclean: clean
	rm -rf lib

test: $(BINARY) coffee
	mkdir -p tmp
	-@$(MOCHA) --reporter $(REPORTER) test/*-test.coffee
	rm -rf tmp

$(BUILD_CONFIG):
	node-waf configure

$(BINARY): $(SOURCES) $(BUILD_CONFIG)
	node-waf build

.PHONY: build coffee clean distclean package test
