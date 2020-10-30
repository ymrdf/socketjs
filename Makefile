all: clean configure build-all

configure: 
	node-gyp configure

build-all: 
	node-gyp build

clean:
	-rm -rf build