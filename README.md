# Memcached Clone

### Description
This project implements a key-value store for a simple lookaside cache, and serves clients on a multi-threaded server using TCP sockets. The project desing is based on Memcached with a simple API (`set`, `get`, `delete`).

This caching system allows you to define your own evictor policy, max memory, pthreads, hash function and collision handling

### Dependencies
This project uses the C++ Boost library. To install, run:
```
brew install boost
```

### Build and Run
1. Build instructions:
    - To build all, run `make all`
    - To build all tests, run `make test`
    - To test the server, run `make test_server`


2. Run program:
    - The default values for all variables are set. To run program with custom values, run the server with the following:
    ```
    ./cache_server -m [maxmem] -p [port] -s [host_string] -t [threads]
    ```

3. To check memory leaks
    - Run `make valgrind`