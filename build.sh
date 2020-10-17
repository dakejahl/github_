# g++ example.cpp -o example -lcurl
# rm -rf build

mkdir -p build && cd build
cmake ..
make
cd ..
