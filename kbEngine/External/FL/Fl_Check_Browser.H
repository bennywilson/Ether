//
// "$Id: Fl_Check_Browser.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $"
//
// Fl_Check_Browser header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Check_Browser widget . */

#ifndef Fl_Check_Browser_H
#define Fl_Check_Browser_H

#include "Fl.H"
#include "Fl_Browser_.H"

/**
  The Fl_Check_Browser widget displays a scrolling list of text
  lines that may be selected and/or checked by the user.
*/
class FL_EXPORT Fl_Check_Browser : public Fl_Browser_ {
  /* required routines for Fl_Browser_ subclass: */

  void *item_first() const;
  void *item_next(void *) const;
  void *item_prev(void *) const;
  int item_height(void *) const;
  int item_width(void *) const;
  void item_draw(void *, int, int, int, int) const;
  void item_select(void *, int);
  int item_selected(void *) const;

  /* private data */

  public: // IRIX 5.3 C++ compiler doesn't support private structures...

#ifndef FL_DOXYGEN
  /** For internal use only. */
  struct cb_item {
	  cb_item *next;	/**< For internal use only. */
	  cb_item *prev;	/**< For internal use only. */
	  char checked;		/**< For internal use only. */
	  char selected;	/**< For internal use only. */
	  char *text;		/**< For internal use only. */
  };
#endif // !FL_DOXYGEN

  private:

  cb_item *first;
  cb_item *last;
  cb_item *cache;
  int cached_item;
  int nitems_;
  int nchecked_;
  cb_item *find_item(int) const;
  int lineno(cb_item *) const;

  public:

  Fl_Check_Browser(int x, int y, int w, int h, const char *l = 0);
   /** The destructor deletes all list items and destroys the browser. */
  ~Fl_Check_Browser() { clear(); }
  int add(char *s);               // add an (unchecked) item
  int add(char *s, int b);        // add an item and set checked
				  // both return the new nitems()
  int remove(int item);           // delete an item. Returns nitems()

  // inline const char * methods to avoid breaking binary compatibility...
   /** See int Fl_Check_Browser::add(char *s) */
  int add(const char *s) { return add((char *)s); }
  /** See int Fl_Check_Browser::add(char *s) */
  int add(const char *s, int b) { return add((char *)s, b); }

  void clear();                   // delete all items
  /**
    Returns how many lines are in the browser.  The last line number is equal to
    this.
  */
  int nitems() const { return nitems_; }
  /**    Returns how many items are currently checked.  */
  int nchecked() const { return nchecked_; }
  int checked(int item) const;
  void checked(int item, int b);
  /**    Equivalent to Fl_Check_Browser::checked(item, 1).  */
  void set_checked(int item) { checked(item, 1); }
  void check_all();
  void check_none();
  int value() const;              // currently selected item
  char *text(int item) const;     // returns pointer to internal buffer

  protected:

  int handle(int);
};

#endif // Fl_Check_Browser_H

//
// End of "$Id: Fl_Check_Browser.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $".
//

