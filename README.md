# Blink
A Simple Blink program with Pico SDK and C.

This is kind of like a Template so you can clone this and run this to Build uf2 Builds for your RP2040 RPi Pico

## Compilation:
Copy the `pico_sdk_import.cmake` file to your project directory and include it in your `CMakeLists.txt` file. Then, you can compile the program using CMake.

```bash
mkdir build
cd build
cmake ..
make
```

Now copy the UF2 to your Pico while it's in Flash/Bootsel mode.

**NOTE: Set the `PICO_SDK_PATH` env var during the cmake build**
