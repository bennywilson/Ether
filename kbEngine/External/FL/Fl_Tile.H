//
// "$Id: Fl_Tile.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//
// Tile header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Tile widget . */

#ifndef Fl_Tile_H
#define Fl_Tile_H

#include "Fl_Group.H"

/**
  The Fl_Tile class lets you resize the children by dragging
  the border between them:
  
  <P ALIGN=CENTER>\image html Fl_Tile.png </P> 
  \image latex  Fl_Tile.png "Fl_Tile" width=4cm
  
  <P>For the tiling to work correctly, the children of an 
  Fl_Tile must cover the entire area of the widget, but not
  overlap. This means that all children must touch each 
  other at their edges, and no gaps can't be left inside the 
  Fl_Tile.
  
  <P>Fl_Tile does not normailly draw any graphics of its own.
  The "borders" which can be seen in the snapshot above
  are actually part of the children. Their boxtypes have been set
  to FL_DOWN_BOX creating the impression of 
  "ridges" where the boxes touch. What you see are 
  actually two adjacent FL_DOWN_BOX's drawn next to each 
  other. All neighboring widgets share the same edge - the widget's 
  thick borders make it appear as though the widgets aren't actually
  touching, but they are. If the edges of adjacent widgets do not
  touch, then it will be impossible to drag the corresponding
  edges.</P>
  
  <P>Fl_Tile allows objects to be resized to zero dimensions.
  To prevent this you can use the resizable() to limit where
  corners can be dragged to.</P>
  
  <P>Even though objects can be resized to zero sizes, they must
  initially have non-zero sizes so the Fl_Tile can figure out
  their layout. If desired, call position() after creating the
  children but before displaying the window to set the borders where you
  want.

  <P>Note on resizable(Fl_Widget &w) :
  The "resizable" child widget (which should be invisible) limits where the
  border can be dragged to.  If you don't set it, it will be possible to
  drag the borders right to the edge, and thus resize objects on the edge
  to zero width or height.  The resizable() widget is not
  resized by dragging any borders. See also void Fl_Group::resizable(Fl_Widget &w)

*/
class FL_EXPORT Fl_Tile : public Fl_Group {
public:
  int handle(int);
  /**
    Creates a new Fl_Tile widget using the given position, size,
    and label string. The default boxtype is FL_NO_BOX.
    
    <P>The destructor <I>also deletes all the children</I>. This allows a
    whole tree to be deleted at once, without having to keep a pointer to
    all the children in the user code. A kludge has been done so the 
    Fl_Tile and all of it's children can be automatic (local)
    variables, but you must declare the Fl_Tile <I>first</I>, so
    that it is destroyed last.
  */
  Fl_Tile(int X,int Y,int W,int H,const char*l=0);
  void resize(int, int, int, int);
  void position(int, int, int, int);
};

#endif

//
// End of "$Id: Fl_Tile.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
