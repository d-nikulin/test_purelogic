CURRENT=.

BIN_TARGET=$CURRENT/bin
LIB_TARGET=$CURRENT/bin/lib
echo $BIN_TARGET
echo $LIB_TARGET

mkdir $BIN_TARGET
mkdir $LIB_TARGET

rm $BIN_TARGET/srv
rm $BIN_TARGET/cli
rm $LIB_TARGET/libcommon*

echo
echo common
make clean --file Makefile_common
make --file Makefile_common
make clean --file Makefile_common

echo
echo cli
make clean --file Makefile_cli
make --file Makefile_cli
make clean --file Makefile_cli

echo
echo srv
make clean --file Makefile_srv
make --file Makefile_srv
make clean --file Makefile_srv

