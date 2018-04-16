# libReliableStream
## Introduction
Reliably transfer data on any POSIX stream. 
- Designed to work on choppy streams, for instance, a serial line.
- TCP-like method to ensure data integrity and correct sequence.
- Callback design. Users can customize I/O and checksum functions.
- Pipe mode. Provides a new FD to ease integration with current codebase.


## Build
### Dependencies
This project has no dependencies.
### Compile
Nearly all my projects use CMake. It's very simple:

    mkdir build
    cd build
    cmake ..
    make -j `nproc`

If you want to install compiled stuff to system (default location is `/usr/local/`), just run `make install`.


## How to use
You can have a look at the examples in the `tests` directory.


## To Do
- Exising code are written in a hurry. May need some refinements.
- Needs a dedicated document/wiki.
