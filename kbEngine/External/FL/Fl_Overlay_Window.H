//
// "$Id: Fl_Overlay_Window.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//
// Overlay window header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

/* \file
   Fl_Overlay_Window class . */

#ifndef Fl_Overlay_Window_H
#define Fl_Overlay_Window_H

#include "Fl_Double_Window.H"

/**
  This window provides double buffering and also the ability to draw the
  "overlay" which is another picture placed on top of the main image. The
  overlay is designed to be a rapidly-changing but simple graphic such as
  a mouse selection box. Fl_Overlay_Window uses the overlay
  planes provided by your graphics hardware if they are available.
  <P>If no hardware support is found the overlay is simulated by drawing
  directly into the on-screen copy of the double-buffered window, and
  "erased" by copying the backbuffer over it again.  This means the
  overlay will blink if you change the image in the window.
*/
class FL_EXPORT Fl_Overlay_Window : public Fl_Double_Window {
#ifndef FL_DOXYGEN
  friend class _Fl_Overlay;
#endif
protected:
  /**
   You must subclass Fl_Overlay_Window and provide this method.
   It is just like a draw() method, except it draws the overlay.
   The overlay will have already been "cleared" when this is called.  You
   can use any of the routines described in &lt;FL/fl_draw.H&gt;.
   */
  virtual void draw_overlay() = 0;
private:
  Fl_Window *overlay_;
public:
  void show();
  void flush();
  void hide();
  void resize(int,int,int,int);
  ~Fl_Overlay_Window();
  /** Returns non-zero if there's hardware overlay support */
  int can_do_overlay();
  void redraw_overlay();
protected:
  /**
   See Fl_Overlay_Window::Fl_Overlay_Window(int X, int Y, int W, int H, const char *l=0)
   */
  Fl_Overlay_Window(int W, int H, const char *l=0);
  /**
   Creates a new Fl_Overlay_Window widget using the given
   position, size, and label (title) string. If the 
   positions (x,y) are not given, then the window manager
   will choose them.
   */
  Fl_Overlay_Window(int X, int Y, int W, int H, const char *l=0);
public:
  void show(int a, char **b) {Fl_Double_Window::show(a,b);}
};

#endif

//
// End of "$Id: Fl_Overlay_Window.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
