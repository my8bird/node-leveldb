REPORTER=dot
BINARY=./lib/leveldb.node

# prefer installed scripts
PATH:=./node_modules/.bin:${PATH}

build:
	if [ ! -d ./build ]; then node-gyp configure; fi
	node-gyp build

coffee:
	coffee --bare --compile --output lib src/coffee

clean:
	node-gyp clean
	rm -rf tmp

distclean: clean
	rm -rf lib node_modules

pkgclean:
	if [ ! -d .git ]; then rm -r deps src; fi

test: build coffee
	rm -rf tmp
	mkdir -p tmp
	@mocha --compilers coffee:coffee-script --reporter $(REPORTER) test/*-test.coffee

.PHONY: build coffee clean distclean pkgclean test
