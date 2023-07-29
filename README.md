# Vic2Modding
Victoria 2 defines parser, intended as a modding tool, made in C using WIN32.

## Build Instructions
You may need to update the path to your Victoria 2 install folder (or the folder of a mod which has its own version of most of the vanilla defines) by editing `MOD_FOLDER` at the top of `source/database/database.h`.

Building requires `cmake`, and should work with either the MinGW or MSVC/Visual Studio version. Running the following from inside the repository will put the finished executable at `./build/Vic2Modding.exe`.
```bash
cmake -S . -B build
cmake --build build
```

## Interface and Controls
The program will display its loading progress and metrics in a console window and use the loaded data to display a political map in a separate window, which can be moved with WASD or the arrow keys and zoomed in and out with the scroll wheel. Clicking on the map will print out information about the targeted province to the console window.
