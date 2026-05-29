# Verification notes

What was checked in the artifact build environment:

- The project tree was reorganized and CMake subprojects were updated.
- CMake syntax and source discovery were checked with a temporary fake Qt package to ensure the new folder layout is picked up correctly.
- A real configure/build was attempted, but the environment does not contain Qt6 development packages, so it stops at `find_package(Qt6 ...)`.

To fully verify locally:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j$(nproc)
./server/Server
./client/Client
```

Before testing PDF preview, place the official Typst binary at `tools/typst/typst` or set `MATHFORCES_TYPST_BIN`.
