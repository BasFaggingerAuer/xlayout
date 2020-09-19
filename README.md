# xlayout
Simple Linux X11 program to lay out windows across a set number of columns on screen. Particularly useful for ultra-widescreen displays.
Tested to work with XFCE.

# Compilation
Apart from the X11 library this code has no other dependencies and can be compiled via:

`gcc -O2 -Wall -Werror -Wextra -std=c99 -pedantic xlayout.c -o xlayout -lX11`

After compilation, run the program as follows (to divide your screen into 3 columns, this number can vary between 1 and 12):

`./xlayout 3`

Then by pressing <kbd>Ctrl</kbd>+<kbd>F1</kbd> up to <kbd>Ctrl</kbd>+<kbd>F3</kbd> the currently active window is moved to the respective column.
You can run `xlayout` automatically on login by adding it to your Session and Startup Application Autostart.

# Known issues
- Maximized windows are not moved and need to be unmaximized first before pressing <kbd>Ctrl</kbd>+<kbd>Fk</kbd>.

