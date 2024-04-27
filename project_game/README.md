# Alterheart (project_game)
### Quick compile settings
#### g++ (Regular)
```
g++ -std=c++17 icon.o game.cpp -o Alterheart.exe
```
#### g++ (SKM)
```
skm g++ -std=c++17 icon.o game.cpp -o Alterheart.exe
```
### Additional information
Any C++ standard above C++14 can be used - thus, the following arguments can be used:
<br>`-std=c++17` `-std=c++20` `-std=c++23`

The file `icon.o` can be left out in compiling, but will result in no icon for the application.

### `icon.o` generation (Windows)
To generate the `icon.o`:
1. Get the `.ico` file that is to be used (usually converted from a `.png`).
2. Create a new file called `icon.rc` and write `ICON_NAME ICON "icon.ico"` in it.
3. Run the command `windres icon.rc -O coff -o icon.o` (or use a `.bat` file that has that command).
