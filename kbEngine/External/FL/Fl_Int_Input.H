//
// "$Id: Fl_Int_Input.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $"
//
// Integer input header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Int_Input widget . */

#ifndef Fl_Int_Input_H
#define Fl_Int_Input_H

#include "Fl_Input.H"

/**
  The Fl_Int_Input class is a subclass of Fl_Input that only allows
  the user to type decimal digits (or hex numbers of the form 0xaef).
*/
class FL_EXPORT Fl_Int_Input : public Fl_Input {
public:
  /**
    Creates a new Fl_Int_Input widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
  Fl_Int_Input(int X,int Y,int W,int H,const char *l = 0);
};

#endif

//
// End of "$Id: Fl_Int_Input.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $".
//
