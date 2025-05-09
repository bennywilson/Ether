//
// "$Id: Fl_Round_Clock.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//
// Round clock header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Round_Clock widget . */

#ifndef Fl_Round_Clock_H
#define Fl_Round_Clock_H

#include "Fl_Clock.H"

/** A clock widget of type FL_ROUND_CLOCK. Has no box. */
class FL_EXPORT Fl_Round_Clock : public Fl_Clock {
public:
  /** Creates the clock widget, setting his type and box. */
  Fl_Round_Clock(int X,int Y,int W,int H, const char *L = 0);
};

#endif

//
// End of "$Id: Fl_Round_Clock.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
