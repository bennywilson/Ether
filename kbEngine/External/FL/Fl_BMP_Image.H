//
// "$Id: Fl_BMP_Image.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $"
//
// BMP image header file for the Fast Light Tool Kit (FLTK).
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
   Fl_BMP_Image widget . */

#ifndef Fl_BMP_Image_H
#define Fl_BMP_Image_H
#  include "Fl_Image.H"

/**
  The Fl_BMP_Image class supports loading, caching,
  and drawing of Windows Bitmap (BMP) image files.
*/
class FL_EXPORT Fl_BMP_Image : public Fl_RGB_Image {

  public:

  Fl_BMP_Image(const char* filename);
};

#endif

//
// End of "$Id: Fl_BMP_Image.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $".
//
