//
// "$Id: Fl_Hor_Slider.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $"
//
// Horizontal slider header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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
   Fl_Hor_Slider widget . */

#ifndef Fl_Hor_Slider_H
#define Fl_Hor_Slider_H

#include "Fl_Slider.H"

/** Horizontal Slider class.

  \see class Fl_Slider.
*/
class FL_EXPORT Fl_Hor_Slider : public Fl_Slider {
public:

  /**
    Creates a new Fl_Hor_Slider widget using the given position,
    size, and label string.
  */
  Fl_Hor_Slider(int X,int Y,int W,int H,const char *l=0);
};

#endif

//
// End of "$Id: Fl_Hor_Slider.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $".
//
