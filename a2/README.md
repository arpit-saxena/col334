# COL334 Assignment 2

Submitted by Arpit Saxena, 2018MT10742

## How to run

Requirements:

1. CMake (>= 3.10)
2. C++ compiler with support for C++17

**NOTE**: If a lesser minor version of CMake is available, try reducing the version number in CMakeLists.txt at the root of the project. It might work. I've set minimum to 3.10 based on what is available in Ubuntu's repos

We'll first make a directory `build` in which the build files will go.

```sh
mkdir build
cd build/
```

Inside the `build` directory, we'll then invoke CMake and then run the Makefile produced.

```sh
cmake ..
cmake --build .
```

The binaries are placed in the `bin` directory. Run `bin/client` and `bin/server` to run the client and server applications respectively.
