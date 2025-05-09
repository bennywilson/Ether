//
// "$Id: Fl_Tree_Item_Array.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//

#ifndef _FL_TREE_ITEM_ARRAY_H
#define _FL_TREE_ITEM_ARRAY_H

#include <FL/Fl.H>
#include "Fl_Export.H"

class FL_EXPORT Fl_Tree_Item;	// forward decl must *precede* first doxygen comment block
				// or doxygen will not document our class..

//////////////////////////
// FL/Fl_Tree_Item_Array.H
//////////////////////////
//
// Fl_Tree -- This file is part of the Fl_Tree widget for FLTK
// Copyright (C) 2009-2010 by Greg Ercolano.
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

///
/// \file
/// \brief This file defines a class that manages an array of Fl_Tree_Item pointers.
///

/// \brief Manages an array of Fl_Tree_Item pointers.
///
/// Because FLTK 1.x.x. has mandated that templates and STL not be used,
/// we use this class to dynamically manage the arrays.
///
/// None of the methods do range checking on index values; the caller
/// must be sure that index values are within the range 0<index<total()
/// (unless otherwise noted).
///

class FL_EXPORT Fl_Tree_Item_Array {
  Fl_Tree_Item **_items;	// items array
  int _total;			// #items in array
  int _size;			// #items *allocated* for array
  int _chunksize;		// #items to enlarge mem allocation
  void enlarge(int count);
public:
  Fl_Tree_Item_Array(int new_chunksize = 10);		// CTOR
  ~Fl_Tree_Item_Array();				// DTOR
  Fl_Tree_Item_Array(const Fl_Tree_Item_Array *o);	// COPY CTOR
  /// Return the item and index \p i.
  Fl_Tree_Item *operator[](int i) {
    return(_items[i]);
  }
  /// Const version of operator[](int i)
  const Fl_Tree_Item *operator[](int i) const {
    return(_items[i]);
  }
  /// Return the total items in the array, or 0 if empty.
  int total() const {
    return(_total);
  }
  /// Swap the two items at index positions \p ax and \p bx.
#if FLTK_ABI_VERSION >= 10301
  // NEW -- code moved to .cxx
  void swap(int ax, int bx);
#else /*FLTK_ABI_VERSION*/
  // OLD
  void swap(int ax, int bx) {
    Fl_Tree_Item *asave = _items[ax];
    _items[ax] = _items[bx];
    _items[bx] = asave;
  }
#endif /*FLTK_ABI_VERSION*/
  void clear();
  void add(Fl_Tree_Item *val);
  void insert(int pos, Fl_Tree_Item *new_item);
  void remove(int index);
  int  remove(Fl_Tree_Item *item);
};

#endif /*_FL_TREE_ITEM_ARRAY_H*/

//
// End of "$Id: Fl_Tree_Item_Array.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
