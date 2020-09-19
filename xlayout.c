/*
MIT License

Copyright (c) 2020 Bas Fagginger Auer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

//Compilation: gcc -O2 -Wall -Werror -Wextra -std=c99 -pedantic xlayout.c -o xlayout -lX11

Window get_window_under_cursor(Display *display) {
    XEvent event;
    
    //Initialize event.
    memset(&event, 0, sizeof(event));
    
    event.xbutton.same_screen = True;
    
    //Get current pointer location and window w.r.t. X root.
    XQueryPointer(display,
                  RootWindow(display, DefaultScreen(display)),
                  &event.xbutton.root,
                  &event.xbutton.window,
                  &event.xbutton.x_root,
                  &event.xbutton.y_root,
                  &event.xbutton.x,
                  &event.xbutton.y,
                  &event.xbutton.state);
    
    //Recurse subwindows until we found our window.
    event.xbutton.subwindow = event.xbutton.window;
    
    while (event.xbutton.subwindow != 0) {
        event.xbutton.window = event.xbutton.subwindow;
        
        XQueryPointer(display,
                      event.xbutton.window,
                      &event.xbutton.root,
                      &event.xbutton.subwindow,
                      &event.xbutton.x_root,
                      &event.xbutton.y_root,
                      &event.xbutton.x,
                      &event.xbutton.y,
                      &event.xbutton.state);
    }
    
    return event.xbutton.window;
}

//From https://gist.github.com/kui/2622504.
Window get_top_focus_window(Display *display) {
    Window window;
    Window root, parent;
    Window *children;
    Status status;
    int revert;
    unsigned int nr_children;
    
    XGetInputFocus(display, &window, &revert);
    
    if (window == None) {
        return window;
    }
    
    root = None;
    parent = window;
    
    while (parent != root) {
        window = parent;
        status = XQueryTree(display, window, &root, &parent, &children, &nr_children);
        
        if (status) {
            XFree(children);
        }
    }
    
    return window;
}

//From https://gist.github.com/kui/2622504.
void print_window_name(FILE *out, Display *display, Window window) {
    XTextProperty property;
    
    //Get window name.
    memset(&property, 0, sizeof(property));
    
    if (XGetWMName(display, window, &property) != 0) {
        int count = 0;
        int result;
        char **list = NULL;
        
        result = XmbTextPropertyToTextList(display, &property, &list, &count);
        
        if (result == Success) {
            fprintf(out, "%s", list[0]);
            XFreeStringList(list);
        }
    }
}

//From https://stackoverflow.com/questions/22749444/listening-to-keyboard-events-without-consuming-them-in-x11-keyboard-hooking.
//From https://stackoverflow.com/questions/27581500/hook-into-linux-key-event-handling/27693340#27693340.
//From https://stackoverflow.com/questions/20595716/control-mouse-by-writing-to-dev-input-mice.
//From https://stackoverflow.com/questions/4530786/xlib-create-window-in-mimized-or-maximized-state.
//From https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html.
int main(int argc, char **argv) {
    int nr_columns;
    Display *display;
    Window root;
    unsigned int modifiers = ControlMask;
    int width, height;
    int i;
    
    //Pass command line arguments.
    fprintf(stderr, "Initializing...\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s nr_columns\n\n(Will give you the ability to assign a window to column N by pressing Control + FN.)\n", argv[0]);
        return -1;
    }
    
    nr_columns = atoi(argv[1]);
    
    if (nr_columns < 1 || nr_columns > 12) {
        fprintf(stderr, "Please specify 1 up to 12 columns!\n");
        return -1;
    }
    
    //Connect to X.
    display = XOpenDisplay(NULL);
    
    if (display == NULL) {
        fprintf(stderr, "Unable to open display!\n");
        return -1;
    }
    
    //Get screen size.
    width = XWidthOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));
    height = XHeightOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));
    
    //Setup key grabbing (Control + function keys).
    root = DefaultRootWindow(display);
    
    for (i = 0; i < nr_columns; ++i) {
        XGrabKey(display, XKeysymToKeycode(display, XK_F1 + i), modifiers, root, True, GrabModeAsync, GrabModeAsync);
    }
    
    XSelectInput(display, root, KeyPressMask | KeyReleaseMask);
    
    //Start listening for events.
    fprintf(stderr, "Running for %d columns for a %dx%d screen...\n", nr_columns, width, height);
    
    while (true) {
        XEvent event;
        KeySym keysym;
        Window focus_window;
        XWindowAttributes attributes;
        
        //Query event.
        XNextEvent(display, &event);
        
        switch (event.type) {
            case KeyPress:
                //Get column index.
                keysym = XLookupKeysym(&event.xkey, 0);
                //printf("keycode: %x\n", event.xkey.keycode);
                //printf("keysym : %lx\n", keysym);
                i = keysym - XK_F1;
                
                //Get currently active window.
                //focus_window = get_window_under_cursor(display);
                focus_window = get_top_focus_window(display);
                XGetWindowAttributes(display, focus_window, &attributes);
                
                //Put window into its column.
                XMoveResizeWindow(display, focus_window, width*i/nr_columns, 0, width/nr_columns, attributes.height);
                break;
            case KeyRelease:
                break;
            default:
                break;
        }
    }
    
    fprintf(stderr, "Shutting down...\n");
    
    XCloseDisplay(display);
    
    fprintf(stderr, "Done.\n");
    
    return 0;
}

