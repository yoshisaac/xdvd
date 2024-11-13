/* 
 * Copyright (c) 2020 ericek111 <erik.brocko@letemsvetemapplem.eu>.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * XDvD developer <isfortner@gmail.com>
 *
 * This source code applies to all the same GPLv3 rules as the original
 * developer mentioned in the comment above.
 *
 * Not sure if any Copyright applies to me, besides my additions lololololol
 */

#include <iostream>
#include <stdlib.h>
#include <climits>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xdbe.h>
#include <math.h>

// Events for normal windows
#define BASIC_EVENT_MASK (StructureNotifyMask|ExposureMask|PropertyChangeMask|EnterWindowMask|LeaveWindowMask|KeyPressMask|KeyReleaseMask|KeymapStateMask)
#define NOT_PROPAGATE_MASK (KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ButtonMotionMask)

using namespace std;

Display *g_display;
int      g_screen;
Screen*  g_display_screen;
Window   g_win;
XdbeBackBuffer g_back_buffer;
int      g_disp_width;
int      g_disp_height;
Pixmap   g_bitmap;
Colormap g_colormap;

XColor red;
XColor blue;
XColor green;
XColor black;
XColor white;

std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
int fpsmeterc = 0;
#define FPSMETERSAMPLE 100
auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
string fpsstring = "";

int shape_event_base;
int shape_error_base;

long event_mask = (StructureNotifyMask|ExposureMask|PropertyChangeMask|EnterWindowMask|LeaveWindowMask|KeyRelease | ButtonPress|ButtonRelease|KeymapStateMask);

void allow_input_passthrough(Window w) {
    XserverRegion region = XFixesCreateRegion (g_display, NULL, 0);

    //XFixesSetWindowShapeRegion (g_display, w, ShapeBounding, 0, 0, 0);
    XFixesSetWindowShapeRegion (g_display, w, ShapeInput, 0, 0, region);

    XFixesDestroyRegion (g_display, region);
}

void list_fonts() {
    char **fontlist;
    int num_fonts;
    fontlist = XListFonts (g_display, "*", 1000, &num_fonts);
    for (int i = 0; i < num_fonts; ++i) {
        fprintf(stderr, "> %s\n", fontlist[i]);
    }
}
// Create a XColor from 3 byte tuple (0 - 255, 0 - 255, 0 - 255).
XColor createXColorFromRGB(short red, short green, short blue) {
    XColor color;

    // m_color.red = red * 65535 / 255;
    color.red = (red * 0xFFFF) / 0xFF;
    color.green = (green * 0xFFFF) / 0xFF;
    color.blue = (blue * 0xFFFF) / 0xFF;
    color.flags = DoRed | DoGreen | DoBlue;

    if (!XAllocColor(g_display, DefaultColormap(g_display, g_screen), &color)) {
        std::cerr << "createXColorFromRGB: Cannot create color" << endl;
        exit(-1);
    }
    return color;
}
// Create a XColor from 3 byte tuple (0 - 255, 0 - 255, 0 - 255).
XColor createXColorFromRGBA(short red, short green, short blue, short alpha) {
    XColor color;

    // m_color.red = red * 65535 / 255;
    color.red = (red * 0xFFFF) / 0xFF;
    color.green = (green * 0xFFFF) / 0xFF;
    color.blue = (blue * 0xFFFF) / 0xFF;
    color.flags = DoRed | DoGreen | DoBlue;

    if (!XAllocColor(g_display, DefaultColormap(g_display, g_screen), &color)) {
        std::cerr << "createXColorFromRGB: Cannot create color" << endl;
        exit(-1);
    }

    *(&color.pixel) = ((*(&color.pixel)) & 0x00ffffff) | (alpha << 24);
    return color;
}

// Create a window
void createShapedWindow() {
    XSetWindowAttributes wattr;
    XColor bgcolor = createXColorFromRGBA(0, 0, 0, 0);

    Window root    = DefaultRootWindow(g_display);
    Visual *visual = DefaultVisual(g_display, g_screen);

    XVisualInfo vinfo;
    XMatchVisualInfo(g_display, DefaultScreen(g_display), 32, TrueColor, &vinfo);
    g_colormap = XCreateColormap(g_display, DefaultRootWindow(g_display), vinfo.visual, AllocNone);

    XSetWindowAttributes attr;
    attr.background_pixmap = None;
    attr.background_pixel = bgcolor.pixel;
    attr.border_pixel=0;
    attr.win_gravity=NorthWestGravity;
    attr.bit_gravity=ForgetGravity;
    attr.save_under=1;
    attr.event_mask=BASIC_EVENT_MASK;
    attr.do_not_propagate_mask=NOT_PROPAGATE_MASK;
    attr.override_redirect=1; // OpenGL > 0
    attr.colormap = g_colormap;

    //unsigned long mask = CWBackPixel|CWBorderPixel|CWWinGravity|CWBitGravity|CWSaveUnder|CWEventMask|CWDontPropagate|CWOverrideRedirect;
    unsigned long mask = CWColormap | CWBorderPixel | CWBackPixel | CWEventMask | CWWinGravity|CWBitGravity | CWSaveUnder | CWDontPropagate | CWOverrideRedirect;

    g_win = XCreateWindow(g_display, root, 0, 0, g_disp_width, g_disp_height, 0, vinfo.depth, InputOutput, vinfo.visual, mask, &attr);

    //XShapeCombineMask(g_display, g_win, ShapeBounding, 900, 500, g_bitmap, ShapeSet);
    XShapeCombineMask(g_display, g_win, ShapeInput, 0, 0, None, ShapeSet );

    // We want shape-changed event too
    #define SHAPE_MASK ShapeNotifyMask
    XShapeSelectInput(g_display, g_win, SHAPE_MASK );

    // Tell the Window Manager not to draw window borders (frame) or title.
	wattr.override_redirect = 1;
    XChangeWindowAttributes(g_display, g_win, CWOverrideRedirect, &wattr);
    allow_input_passthrough(g_win);

    g_back_buffer = XdbeAllocateBackBufferName(g_display, g_win, 0);
    
    // Show the window
    XMapWindow(g_display, g_win);

    red = createXColorFromRGBA(255, 0, 0, 255);
    blue = createXColorFromRGBA(0, 0, 255, 255);
    green = createXColorFromRGBA(0, 255, 0, 255);
    black = createXColorFromRGBA(0, 0, 0, 200);
    white = createXColorFromRGBA(255, 255, 255, 255);
}

inline void db_swap_buffers(Display* d, Window win)
{
  XdbeSwapInfo swap_info;
  swap_info.swap_window = win;
  swap_info.swap_action = 0;
  XdbeSwapBuffers(d, &swap_info, 1);
}

inline void db_clear(XdbeBackBuffer back_buffer, Display* d, GC gc)
{
  XSetForeground(d, gc, 0);
  XFillRectangle(d, back_buffer, gc, 0, 0, g_disp_width, g_disp_height);
}

#define logoh 90
#define logow 210
int xlogo = 0;
bool xlogo_sub = false;
int ylogo = 0;
bool ylogo_sub = false;

unsigned int corners = 0;
unsigned int bounces = 0;
// Draw on the shaped window.
// Yes it's possible, but only pixels that hits the mask are visible.
// A hint: You can change the mask during runtime if you like.
void draw()
{
  fpsmeterc++;
  if(fpsmeterc == FPSMETERSAMPLE) {
    fpsmeterc = 0;
    t1 = t2;
    t2 = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
    fpsstring = /*to_string(duration) + " / " +*/ to_string(1000000*FPSMETERSAMPLE/duration);
  }
  
  GC gc;
  XGCValues gcv;

  /*// Line width and type
    gcv.line_width = 1;
    gcv.line_style = LineSolid;
    gcv.foreground = red.pixel;

    unsigned long mask = GCLineWidth | GCLineStyle | GCForeground;
    gc = XCreateGC(g_display, g_win, mask, &gcv);*/
    
  gc = XCreateGC (g_display, g_win, 0, 0);
  XSetBackground (g_display, gc, white.pixel); 
  XSetForeground (g_display, gc, red.pixel);

  db_clear(g_back_buffer, g_display, gc);  

  XFontStruct* font;
  // const char * fontname = "-misc-fixed-bold-r-normal--18-120-100-100-c-90-iso8859-2";
  // const char * fontname = "rk24"; // ~ chinese shit
  // list_fonts(); 

  const char* fontname = "9x15bold";
  font = XLoadQueryFont(g_display, fontname);
  /* If the font could not be loaded, revert to the "fixed" font. */
  if (!font) {
    fprintf(stderr, "unable to load font %s > using fixed\n", fontname);
    font = XLoadQueryFont (g_display, "fixed");
  }
  XSetFont (g_display, gc, font->fid);

  XSetForeground (g_display, gc, black.pixel);
  XFillRectangle(g_display, g_back_buffer, gc, 0, 0, 80, 30);
  
  XSetForeground (g_display, gc, red.pixel);
  if(duration > 0.0f) {
    const char* text = ("FPS: " + fpsstring).c_str();
    XDrawString(g_display, g_back_buffer, gc, 10, 20, text, strlen(text));
  }

  switch (bounces%3) {
  case 0:
    XSetForeground(g_display, gc, red.pixel);
    break;
  case 1:
    XSetForeground(g_display, gc, blue.pixel);
    break;
  case 2:
    XSetForeground(g_display, gc, green.pixel);
  }

  XFillRectangle(g_display, g_back_buffer, gc, xlogo, ylogo, logow, logoh);

  //move the box
  if (xlogo_sub) xlogo-=9;
  else xlogo+=9;

  if (ylogo_sub) ylogo-=6;
  else ylogo+=6;

  //wall detection
  if (xlogo+logow >= g_disp_width) {
    xlogo_sub = true;
    ++bounces;
  }
  if (xlogo <= 0) {
    xlogo_sub = false;
    ++bounces;
  }
  if (ylogo+logoh >= g_disp_height) {
    ylogo_sub = true;
    ++bounces;
  }
  if (ylogo <= 0) {
    ylogo_sub = false;
    ++bounces;
  }

  //corner detection
  if (xlogo <= 0 && ylogo+logoh >= g_disp_height) {
    ++corners;
    printf("Corner count: %u", corners);
  }
  if (xlogo <= 0 && ylogo <= 0) {
    ++corners;
    printf("Corner count: %u", corners);
  }
  if (xlogo+logow >= g_disp_width && ylogo <= 0) {
    ++corners;
    printf("Corner count: %u", corners);
  }
  if (xlogo+logow >= g_disp_width && ylogo+logoh >= g_disp_height) {
    ++corners;
    printf("Corner count: %u", corners);
  }
  
  db_swap_buffers(g_display, g_win);
  XFreeGC(g_display, gc);

}

void openDisplay() {
    g_display = XOpenDisplay(0);

    if (!g_display) {
        cerr << "Failed to open X display" << endl;
        exit(-1);
    }

    g_screen         = DefaultScreen(g_display);
    g_display_screen = DefaultScreenOfDisplay(g_display);

    g_disp_width  = g_display_screen->width;
    g_disp_height = g_display_screen->height;

    // Has shape extions?
    if (!XShapeQueryExtension (g_display, &shape_event_base, &shape_error_base)) {
       cerr << "NO shape extension in your system !" << endl;
       exit (-1);
    }
}


int main() {
  srand(time(NULL));
  
  openDisplay();

  createShapedWindow();

  //random offset along the x axis
  xlogo = (rand()%g_disp_width)+1;

  printf("width: %d\n", g_disp_width);
  printf("height: %d\n", g_disp_height);
  
  XEvent xevt;
  XExposeEvent *eev;
  XConfigureEvent *cev;
  XKeyEvent *kev;

  for (;;) {
    draw();
    usleep(16000); //update 60 ish times a second
  }

  return 0;
}

