//
// "$Id: Fl_Output.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//
// Output header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Output widget . */

#ifndef Fl_Output_H
#define Fl_Output_H

#include "Fl_Input.H"
/**
  This widget displays a piece of text.
  
  When you set the value() , Fl_Output does a strcpy() to its own storage,
  which is useful for program-generated values.  The user may select
  portions of the text using the mouse and paste the contents into other
  fields or programs.

  <P align=CENTER>\image html text.png</P>
  \image latex text.png "Fl_Output" width=8cm

  There is a single subclass, Fl_Multiline_Output, which allows you to
  display multiple lines of text. Fl_Multiline_Output does not provide
  scroll bars. If a more complete text editing widget is needed, use
  Fl_Text_Display instead.

  The text may contain any characters except \\0, and will correctly
  display anything, using ^X notation for unprintable control characters
  and \\nnn notation for unprintable characters with the high bit set. It
  assumes the font can draw any characters in the ISO-Latin1 character set.
*/
class FL_EXPORT Fl_Output : public Fl_Input {
public:
  /**
    Creates a new Fl_Output widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */

    Fl_Output(int X,int Y,int W,int H, const char *l = 0);
};

#endif 

//
// End of "$Id: Fl_Output.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
