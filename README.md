# zq, an example for using ZeroMQ

zq is a sample app using zmq's multipart messages to sent different types to the same endpoint.
The concept is simple, put the type info in the first message part, and the value in the second.
Besides trivial copyable types, there is a sample on how to send a std::string, and there is a sample on how to send Protobuf messages.
All these types can be send to the same endpoint and accordantly restored there.
This means also, subscribers do subscribe on types.

The code in the project shall be used as inspiration what can be done and how to work type driven, if wanted, via ZeroMQ.
It is not meant to be a library. See the test for usage examples.

Building the project and the tests requires VCPKG and CMake.
If vcpkg is not installed in `~/vcpkg`, then set the environment variable VCPKG_ROOT to the correct path.
You can run the code on Linux Windows and Mac if you have a compiler that is C++20 capable.

All the code is MPL 2.0 licensed.

## Building the project

The project supports [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html).
You may explore the presets available on your system by executing

```bash
cmake --list-presets
```

Once you have decided which preset you want to use, configure and build like so:

```bash
cmake --preset=<name>
cmake --build --preset=<name>
```

## Running tests

After the project has been built, you are ready to run the tests:

```bash
ctest --preset=<name>
```
