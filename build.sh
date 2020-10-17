# g++ example.cpp -o example -lcurl
rm -rf build

mkdir build && cd build
cmake ..
make
cd ..
