//
// "$Id: Fl_Tabs.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//
// Tab header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Tabs widget . */

#ifndef Fl_Tabs_H
#define Fl_Tabs_H

#include "Fl_Group.H"

/**
  The Fl_Tabs widget is the "file card tabs"
  interface that allows you to put lots and lots of buttons and
  switches in a panel, as popularized by many toolkits.
  
  \image html  tabs.png
  \image latex tabs.png "Fl_Tabs" width=8cm
  
  Clicking the tab makes a child visible() by calling
  show() on it, and all other children are made invisible
  by calling hide() on them. Usually the children are Fl_Group widgets
  containing several widgets themselves.
  
  Each child makes a card, and its label() is printed
  on the card tab, including the label font and style.  The
  selection color of that child is used to color the tab, while
  the color of the child determines the background color of the pane.
  
  The size of the tabs is controlled by the bounding box of the
  children (there should be some space between the children and
  the edge of the Fl_Tabs), and the tabs may be placed
  "inverted" on the bottom - this is determined by which
  gap is larger. It is easiest to lay this out in fluid, using the
  fluid browser to select each child group and resize them until
  the tabs look the way you want them to.
*/
class FL_EXPORT Fl_Tabs : public Fl_Group {
  Fl_Widget *value_;
  Fl_Widget *push_;
  int *tab_pos;		// array of x-offsets of tabs per child + 1
  int *tab_width;	// array of widths of tabs per child + 1
  int tab_count;	// array size
  int tab_positions();	// allocate and calculate tab positions
  void clear_tab_positions();
  int tab_height();
  void draw_tab(int x1, int x2, int W, int H, Fl_Widget* o, int sel=0);
protected:
  void redraw_tabs();
  void draw();

public:
  int handle(int);
  Fl_Widget *value();
  int value(Fl_Widget *);
  Fl_Widget *push() const {return push_;}
  int push(Fl_Widget *);
  Fl_Tabs(int,int,int,int,const char * = 0);
  Fl_Widget *which(int event_x, int event_y);
  ~Fl_Tabs();
  void client_area(int &rx, int &ry, int &rw, int &rh, int tabh=0);
};

#endif

//
// End of "$Id: Fl_Tabs.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
