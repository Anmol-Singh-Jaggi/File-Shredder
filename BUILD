# Change these according to the Boost installation path in your system
BOOST_HEADER_PATH=~/boost
BOOST_OBJECT_PATH=~/boost/stage/lib

g++ -std=c++11 -fexceptions -isystem $BOOST_HEADER_PATH -c file_shredder.cpp -o file_shredder.o
g++ -o file_shredder file_shredder.o $BOOST_OBJECT_PATH/libboost_filesystem.a $BOOST_OBJECT_PATH/libboost_system.a

