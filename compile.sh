#echo Cleaning build cache
#rm -rf buildtmp/
#mkdir buildtmp/
#cp -rf src/* buildtmp/

#cd pb
#rm -rf buildtmp/
#mkdir buildtmp/
#echo Done.

#echo Compiling protobuf files
#find . | grep -o [^/]*\\.proto | xargs protoc --cpp_out=buildtmp/
#echo Done.

#cd ../

#cd src
#echo Generating help page source
#python3 helpToSrc.py
#echo Done.

#cd ../

#echo Compiling program
#cp -rf pb/buildtmp/* buildtmp/
#cd buildtmp
#g++ -pipe -lncurses -lpthread -lprotobuf -lmbedtls -lmbedcrypto -lmbedx509 -DMG_ENABLE_MBEDTLS=1 -x c++ -o botExecutable *.h *.c *.cc
#echo Done.

#cd ../

#cp buildtmp/botExecutable botExecutable
#echo Executable moved to current directory

make && make clean
