//
// "$Id: Fl_Toggle_Button.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//
// Toggle button header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Toggle_Button widget . */

#ifndef Fl_Toggle_Button_H
#define Fl_Toggle_Button_H

#include "Fl_Button.H"

/**
  The toggle button is a push button that needs to be clicked once 
  to toggle on, and one more time to toggle off.
  The Fl_Toggle_Button subclass displays the "on" state by
  drawing a pushed-in button.</P>
  <P>Buttons generate callbacks when they are clicked by the user.  You
  control exactly when and how by changing the values for type()
  and when().
*/
class FL_EXPORT Fl_Toggle_Button : public Fl_Button {
public:
  /**
    Creates a new Fl_Toggle_Button widget using the given
    position, size, and label string.
    <P>The inherited destructor deletes the toggle button.
  */
  Fl_Toggle_Button(int X,int Y,int W,int H,const char *l=0);
};

#endif

//
// End of "$Id: Fl_Toggle_Button.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
