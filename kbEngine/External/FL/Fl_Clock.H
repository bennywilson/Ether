//
// "$Id: Fl_Clock.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $"
//
// Clock header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Clock, Fl_Clock_Output widgets . */

#ifndef Fl_Clock_H
#define Fl_Clock_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

// values for type:
#define FL_SQUARE_CLOCK		0	/**< type() of Square Clock variant */
#define FL_ROUND_CLOCK		1	/**< type() of Round Clock variant */
#define FL_ANALOG_CLOCK FL_SQUARE_CLOCK	/**< An analog clock is square */
#define FL_DIGITAL_CLOCK FL_SQUARE_CLOCK /**< Not yet implemented */

// fabien: Please keep the horizontal formatting of both images in class desc, 
// don't lose vert. space for nothing!

/**
  \class Fl_Clock_Output
  \brief This widget can be used to display a program-supplied time.
  
  The time shown on the clock is not updated. To display the current time,
  use Fl_Clock instead.

  \htmlonly <BR>  <table align=CENTER border=1 cellpadding=5 >  
  <caption align=bottom>type() FL_SQUARE_CLOCK and FL_ROUND_CLOCK </caption> <TR><TD> \endhtmlonly
  \image html clock.png  
  \htmlonly </TD> <TD> \endhtmlonly
  \image html round_clock.png 
  \htmlonly </TD> </TR> </table> \endhtmlonly
  \image latex clock.png "FL_SQUARE_CLOCK type" width=4cm
  \image latex round_clock.png "FL_ROUND_CLOCK type" width=4cm
 */
class FL_EXPORT Fl_Clock_Output : public Fl_Widget {
  int hour_, minute_, second_;
  ulong value_;
  void drawhands(Fl_Color,Fl_Color); // part of draw
protected:
  void draw();
  void draw(int X, int Y, int W, int H);
public:

  Fl_Clock_Output(int X, int Y, int W, int H, const char *L = 0);

  void value(ulong v);	// set to this Unix time
  
  void value(int H, int m, int s);

  /**
    Returns the displayed time.
    Returns the time in seconds since the UNIX epoch (January 1, 1970).
    \see value(ulong)
   */
  ulong value() const {return value_;}

  /**
    Returns the displayed hour (0 to 23).
    \see value(), minute(), second()
   */
  int hour() const {return hour_;}

  /**
    Returns the displayed minute (0 to 59).
    \see value(), hour(), second()
   */
  int minute() const {return minute_;}

  /**
    Returns the displayed second (0 to 60, 60=leap second).
    \see value(), hour(), minute()
   */
  int second() const {return second_;}
};

// a Fl_Clock displays the current time always by using a timeout:

/**
  \class Fl_Clock
  \brief This widget provides a round analog clock display.

  Fl_Clock is provided for Forms compatibility. 
  It installs a 1-second timeout callback using Fl::add_timeout().
  You can choose the rounded or square type of the clock with type(), see below.
  \htmlonly <BR>  <table align=CENTER border=1 cellpadding=5 >  
  <caption align=bottom>type() FL_SQUARE_CLOCK and FL_ROUND_CLOCK </caption> <TR><TD> \endhtmlonly
  \image html clock.png  
  \htmlonly </TD> <TD> \endhtmlonly
  \image html round_clock.png 
  \htmlonly </TD> </TR> </table> \endhtmlonly
  \image latex clock.png "FL_SQUARE_CLOCK type" width=4cm  
  \image latex round_clock.png "FL_ROUND_CLOCK type" width=4cm
 */
class FL_EXPORT Fl_Clock : public Fl_Clock_Output {
public:
  int handle(int);

  Fl_Clock(int X, int Y, int W, int H,  const char *L = 0);
  
  Fl_Clock(uchar t, int X, int Y, int W, int H, const char *L);
  
  ~Fl_Clock();
};

#endif

//
// End of "$Id: Fl_Clock.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $".
//
