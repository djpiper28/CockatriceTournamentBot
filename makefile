MBEDTLS=-lmbedtls -lmbedcrypto -lmbedx509 -DMG_ENABLE_MBEDTLS=1
OPENSSL=`pkg-config --cflags --libs openssl` -DMG_ENABLE_OPENSSL=1
PROTOBUF=`pkg-config --cflags --libs protobuf`

#Use openssl or mbedtls
LIBS=-pthread -lpthread ${PROTOBUF} ${MBEDTLS}
MG_ARGS=-DMG_ENABLE_IPV6=1 -DMG_ENABLE_LINES=1 -DMG_ENABLE_DIRECTORY_LISTING=1 -DMG_ENABLE_FS=1
DO_DEBUG=-DDEBUG=1 -g -DDEBUG=0 -DMEGA_DEBUG=0
BASE_CC=g++ -Wall $(CFLAGS) ${LIBS} ${MG_ARGS} ${DO_DEBUG}

objectsc=$(wildcard buildtmp/*.c) 
objectscc=$(wildcard buildtmp/*.cc)
objectscpp=$(wildcard buildtmp/*.cpp)

all:
	@echo "-> Done :)"
	make prep-src
	make comp
	
comp: $(objectsc) $(objectscc) $(objectscpp)
	@echo Creating executable
	${BASE_CC} buildtmp/*.o -pie -o botExecutable
	
$(objectsc): %.c: %
$(objectscc): %.cc: %
$(objectscpp): %.cpp: %

%:
	@echo "-> Compiling $@.[^h]* to $@.o"
	${BASE_CC} $@.[^ho]* -c -o $@.o
		
prep-src: gendocs
	@echo "-> Preparing src"
	if [ ! -d "buildtmp/" ]; then mkdir buildtmp/; fi
	if [ ! -d "pb/buildtmp/" ]; then mkdir pb/buildtmp/; fi

	rm -rf builtmp/*.c builtmp/*.h builtmp/*.cpp builtmp/*.cc
	rm -rf pb/builtmp/
	
	cd pb && find . | grep -o [^/]*\\.proto | xargs protoc --cpp_out=buildtmp/

	cd src/ && python3 helpToSrc.py

	cp -rf pb/buildtmp/* buildtmp/ && cp -rf src/* buildtmp/
	
gendocs:
	@echo "-> Generating docs"
	cd src/ && python3 helpToSrc.py
	
clean:
	rm -rf buildtmp/ pb/buildtmp/
