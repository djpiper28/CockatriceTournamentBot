LIBS = -lncurses -pthread -lpthread -lprotobuf -lmbedtls -lmbedcrypto -lmbedx509
ARGS = -DMG_ENABLE_MBEDTLS=1 -DMG_ENABLE_OPENSSL=1 -DMG_ENABLE_IPV6=1
DO_DEBUG = -DDEBUG=1 -g
BASE_CC = g++ $(CFLAGS) ${LIBS} ${ARGS} -x c++ -o botExecutable *.h *.c *.cc *.cpp

build:
	make prep-src	
	make build-prj

prep-src: src/* pb/*	
	make gendocs
	
	if [ ! -d "buildtmp/" ]; then mkdir buildtmp/; fi
	if [ ! -d "pb/buildtmp/" ]; then mkdir pb/buildtmp/; fi

	rm -rf builtmp/*.c builtmp/*.h builtmp/*.cpp builtmp/*.cc
	rm -rf pb/builtmp/
	
	cd pb && find . | grep -o [^/]*\\.proto | xargs protoc --cpp_out=buildtmp/

	cd src/ && python3 helpToSrc.py

	cp -rf pb/buildtmp/* buildtmp/ && cp -rf src/* buildtmp/
	
build-prj: src/* pb/* buildtmp/*
	cd buildtmp/ && ${BASE_CC} && cd ../ && cp buildtmp/botExecutable botExecutable

build-debug: src/* pb/* buildtmp/*
	make prep-src
	cd buildtmp/ && ${BASE_CC} ${DO_DEBUG} && cd ../ && cp buildtmp/botExecutable botExecutable
	
gendocs:
	cd src/ && python3 helpToSrc.py
	
clean:
	rm -rf buildtmp/ pb/buildtmp/
