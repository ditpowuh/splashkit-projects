# Alterheart (project_game)
### Controls/Instructions
#### General
Eliminate all enemies in the location before dying or before the music finishes playing. Dashing can only be done in sync to the beat of the song.
<br>
Shooting and casting can be done without being in beat, but the energy/power of the player recharges in sync to the beat.
#### Input controls
`W` - Move up<br>
`A` - Move left<br>
`S` - Move down<br>
`D` - Move right<br>
<br>
`Left click` - Shoot (In game)/Select (In any menu)<br>
`Right click` - Cast (In game)<br>
`Space` - Dash/Dodge (In game whilst in sync to the beat)<br>
`Q` - Blast/Triple shoot (In game)<br>
<br>
`ESC` - Pause/Unpause (In game)/Skip (Before game start)<br>
`F11` - Fullscreen shortcut (Only when game is paused or before game starts, if available)<br>
### Quick compile commands (with settings)
#### g++ (Regular)
```
g++ -std=c++17 icon.o game.cpp -o Alterheart.exe
```
#### g++ (SKM)
```
skm g++ -std=c++17 icon.o game.cpp -o Alterheart.exe
```
### Recommended Specifications
A display, at minimum of `1280x720 pixels`, should be used. Fullscreen cannot be enabled or toggled when the display does not use a `16:9` aspect ratio.
<br>
The program may use up to `2 GB` of RAM at most. With that said, a device with at least `4 GB` of RAM is recommended.
### Additional Information
For compiling, any C++ standard **above** C++14 can be used - thus, the following arguments can be used:
<br>`-std=c++17` `-std=c++20` `-std=c++23`

The file `icon.o` can also be left out in compiling, but will result in no icon for the application.
### `icon.o` generation (Windows)
To generate the `icon.o`:
1. Get the `.ico` file that is to be used (usually converted from a `.png`).
2. Create a new file called `icon.rc` and write `ICON_NAME ICON "icon.ico"` in it.
3. Run the command `windres icon.rc -O coff -o icon.o` (or use a `.bat` file that has that command).
