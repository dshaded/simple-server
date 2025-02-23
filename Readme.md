# Simple TCP server in C++ with Boost::asio.

It's a tcp server implementation for a specific test task.

## Build and run instructions:
Run cmake in the project root directory to build:
```shell
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build ./build --target server
```

Run the server at port 12345:
```shell
./build/server -p 12345
```

Run a simple python client code separately (port number 12345 is hard-coded):
```shell
./scripts/test_client.py
```
