//
// "$Id: Fl_Tree_Item.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $"
//

#ifndef FL_TREE_ITEM_H
#define FL_TREE_ITEM_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Image.H>
#include <FL/fl_draw.H>

#include <FL/Fl_Tree_Item_Array.H>
#include <FL/Fl_Tree_Prefs.H>

//////////////////////
// FL/Fl_Tree_Item.H
//////////////////////
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
/// \brief This file contains the definitions for Fl_Tree_Item
///

/// \brief Tree item
///
/// This class is a single tree item, and manages all of the item's attributes.
/// Fl_Tree_Item is used by Fl_Tree, which is comprised of many instances of Fl_Tree_Item.
///
/// Fl_Tree_Item is hierarchical; it dynamically manages an Fl_Tree_Item_Array of children
/// that are themselves instances of Fl_Tree_Item. Each item can have zero or more children.
/// When an item has children, close() and open() can be used to hide or show them.
///
/// Items have their own attributes; font size, face, color.
/// Items maintain their own hierarchy of children.
///
/// When you make changes to items, you'll need to tell the tree to redraw()
/// for the changes to show up.
///
class FL_EXPORT Fl_Tree_Item {
  const char             *_label;		// label (memory managed)
  Fl_Font                 _labelfont;		// label's font face
  Fl_Fontsize             _labelsize;		// label's font size
  Fl_Color                _labelfgcolor;	// label's fg color
  Fl_Color                _labelbgcolor;	// label's bg color (0xffffffff is 'transparent')
  enum {
    OPEN                = 1<<0,		///> item is open
    VISIBLE             = 1<<1,		///> item is visible
    ACTIVE              = 1<<2,		///> item is active
    SELECTED            = 1<<3,		///> item is selected
  };
#if FLTK_ABI_VERSION >= 10301
  // NEW
  unsigned short _flags;		// misc flags
#else /*FLTK_ABI_VERSION*/
  // OLD: this will go away after 1.3.x
  char                    _open;		// item is open?
  char                    _visible;		// item is visible?
  char                    _active;		// item activated?
  char                    _selected;		// item selected?
#endif /*FLTK_ABI_VERSION*/
  int                     _xywh[4];		// xywh of this widget (if visible)
  int                     _collapse_xywh[4];	// xywh of collapse icon (if visible)
  int                     _label_xywh[4];	// xywh of label
  Fl_Widget              *_widget;		// item's label widget (optional)
  Fl_Image               *_usericon;		// item's user-specific icon (optional)
  Fl_Tree_Item_Array      _children;		// array of child items
  Fl_Tree_Item           *_parent;		// parent item (=0 if root)
  void                   *_userdata;    	// user data that can be associated with an item
#if FLTK_ABI_VERSION >= 10301
  Fl_Tree_Item           *_prev_sibling;	// previous sibling (same level)
  Fl_Tree_Item           *_next_sibling;	// next sibling (same level)
#endif /*FLTK_ABI_VERSION*/
protected:
  void show_widgets();
  void hide_widgets();
  void draw_vertical_connector(int x, int y1, int y2, const Fl_Tree_Prefs &prefs);
  void draw_horizontal_connector(int x1, int x2, int y, const Fl_Tree_Prefs &prefs);
public:
  Fl_Tree_Item(const Fl_Tree_Prefs &prefs);	// CTOR
  ~Fl_Tree_Item();				// DTOR
  Fl_Tree_Item(const Fl_Tree_Item *o);		// COPY CTOR
  int x() const { return(_xywh[0]); }
  int y() const { return(_xywh[1]); }
  int w() const { return(_xywh[2]); }
  int h() const { return(_xywh[3]); }
  int calc_item_height(const Fl_Tree_Prefs &prefs) const;
  void draw(int X, int &Y, int W, Fl_Widget *tree, Fl_Tree_Item *itemfocus, const Fl_Tree_Prefs &prefs, int lastchild=1);
  void show_self(const char *indent = "") const;
  void label(const char *val);
  const char *label() const;

  /// Set a user-data value for the item.
  inline void user_data( void* data ) { _userdata = data; }

  /// Retrieve the user-data value that has been assigned to the item.
  inline void* user_data() const { return _userdata; }
  
  /// Set item's label font face.
  void labelfont(Fl_Font val) {
    _labelfont = val; 
  }
  /// Get item's label font face.
  Fl_Font labelfont() const {
    return(_labelfont);
  }
  /// Set item's label font size.
  void labelsize(Fl_Fontsize val) {
    _labelsize = val; 
  }
  /// Get item's label font size.
  Fl_Fontsize labelsize() const {
    return(_labelsize);
  }
  /// Set item's label foreground text color.
  void labelfgcolor(Fl_Color val) {
    _labelfgcolor = val; 
  }
  /// Set item's label text color.
  void labelcolor(Fl_Color val) {
    _labelfgcolor = val; 
  }
  /// Return item's label text color.
  Fl_Color labelcolor() const {
    return(_labelfgcolor); 
  }
  /// Return item's label foreground text color.
  Fl_Color labelfgcolor() const {
    return(_labelfgcolor); 
  }
  /// Set item's label background color.
  /// A special case is made for color 0xffffffff which is treated as 'transparent'.
  void labelbgcolor(Fl_Color val) {
    _labelbgcolor = val; 
  }
  /// Return item's background text color.
  /// If the color is 0xffffffff, it is 'transparent'.
  Fl_Color labelbgcolor() const {
    return(_labelbgcolor); 
  }
  /// Assign an FLTK widget to this item.
  void widget(Fl_Widget *val) {
    _widget = val; 
  }
  /// Return FLTK widget assigned to this item.
  Fl_Widget *widget() const {
    return(_widget); 
  }
  /// Return the number of children this item has.
  int children() const {
    return(_children.total()); 
  }
  /// Return the child item for the given 'index'.
  Fl_Tree_Item *child(int index) {
    return(_children[index]); 
  }
  /// Return the const child item for the given 'index'.
  const Fl_Tree_Item *child(int t) const;
  /// See if this item has children.
  int has_children() const {
    return(children()); 
  }
  int find_child(const char *name);
  int find_child(Fl_Tree_Item *item);
  int remove_child(Fl_Tree_Item *item);
  int remove_child(const char *new_label);
  void clear_children();
  void swap_children(int ax, int bx);
  int swap_children(Fl_Tree_Item *a, Fl_Tree_Item *b);
  const Fl_Tree_Item *find_child_item(char **arr) const;	// const
        Fl_Tree_Item *find_child_item(char **arr);		// non-const
  const Fl_Tree_Item *find_item(char **arr) const;		// const
        Fl_Tree_Item *find_item(char **arr);			// non-const
  //////////////////
  // Adding items
  //////////////////
  Fl_Tree_Item *add(const Fl_Tree_Prefs &prefs, const char *new_label);
  Fl_Tree_Item *add(const Fl_Tree_Prefs &prefs, char **arr);
  Fl_Tree_Item *insert(const Fl_Tree_Prefs &prefs, const char *new_label, int pos=0);
  Fl_Tree_Item *insert_above(const Fl_Tree_Prefs &prefs, const char *new_label);
  int depth() const;
  Fl_Tree_Item *prev();
  Fl_Tree_Item *next();
  Fl_Tree_Item *next_sibling();
  Fl_Tree_Item *prev_sibling();
  void update_prev_next(int index);
  Fl_Tree_Item *next_displayed(Fl_Tree_Prefs &prefs);
  Fl_Tree_Item *prev_displayed(Fl_Tree_Prefs &prefs);
  
  /// Return the parent for this item. Returns NULL if we are the root.
  Fl_Tree_Item *parent() {
    return(_parent);
  }
  /// Return the const parent for this item. Returns NULL if we are the root.
  const Fl_Tree_Item *parent() const {
    return(_parent);
  }
  /// Set the parent for this item.
  /// Should only be used by Fl_Tree's internals.
  ///
  void parent(Fl_Tree_Item *val) {
    _parent = val;
  }
  //////////////////
  // State
  //////////////////
  void open();
  void close();
  /// See if the item is 'open'.
  int is_open() const {
    return(is_flag(OPEN));
  }
  /// See if the item is 'closed'.
  int is_close() const {
    return(is_flag(OPEN)?0:1);
  }
  /// Toggle the item's open/closed state.
  void open_toggle() {
    is_open()?close():open();
  }
  /// Change the item's selection state to the optionally specified 'val'.
  /// If 'val' is not specified, the item will be selected.
  ///
  void select(int val=1) {
    set_flag(SELECTED, val);
  }
  /// Toggle the item's selection state.
  void select_toggle() {
    if ( is_selected() ) {
      deselect();	// deselect if selected
    } else {
      select();		// select if deselected
    }
  }
  /// Select item and all its children.
  ///     Returns count of how many items were in the 'deselected' state,
  ///     ie. how many items were "changed".
  ///
  int select_all() {
    int count = 0;
    if ( ! is_selected() ) {
      select();
      ++count;
    }
    for ( int t=0; t<children(); t++ ) {
      count += child(t)->select_all();
    }
    return(count);
  }
  /// Disable the item's selection state.
  void deselect() {
    set_flag(SELECTED, 0);
  }
  /// Deselect item and all its children.
  ///     Returns count of how many items were in the 'selected' state,
  ///     ie. how many items were "changed".
  ///
  int deselect_all() {
    int count = 0;
    if ( is_selected() ) {
      deselect();
      ++count;
    }
    for ( int t=0; t<children(); t++ ) {
      count += child(t)->deselect_all();
    }
    return(count);
  }
  /// See if the item is selected.
  char is_selected() const {
    return(is_flag(SELECTED));
  }
  /// Change the item's activation state to the optionally specified 'val'.
  ///
  /// When deactivated, the item will be 'grayed out'; the callback() 
  /// won't be invoked if the user clicks on the label. If the item
  /// has a widget() associated with the item, its activation state
  /// will be changed as well.
  ///
  /// If 'val' is not specified, the item will be activated.
  ///
  void activate(int val=1) {
    set_flag(ACTIVE,val);
    if ( _widget && val != (int)_widget->active() ) {
      if ( val ) {
	_widget->activate();
      } else {
	_widget->deactivate();
      }
      _widget->redraw();
    }
  }
  /// Deactivate the item; the callback() won't be invoked when clicked.
  /// Same as activate(0)
  ///
  void deactivate() {
    activate(0);
  }
  /// See if the item is activated.
  char is_activated() const {
    return(is_flag(ACTIVE));
  }
  /// See if the item is activated.
  char is_active() const {
    return(is_activated());
  }
  /// See if the item is visible. Alias for is_visible().
  int visible() const {
    return(is_visible());
  }
  /// See if the item is visible.
  int is_visible() const {
    return(is_flag(VISIBLE));
  }
  int visible_r() const;

  /// Set the item's user icon to an Fl_Image. '0' will disable.
  void usericon(Fl_Image *val) {
    _usericon = val;
  }
  /// Get the item's user icon as an Fl_Image. Returns '0' if disabled.
  Fl_Image *usericon() const {
    return(_usericon);
  }
  //////////////////
  // Events
  //////////////////
  const Fl_Tree_Item *find_clicked(const Fl_Tree_Prefs &prefs) const;
  Fl_Tree_Item *find_clicked(const Fl_Tree_Prefs &prefs);
  int event_on_collapse_icon(const Fl_Tree_Prefs &prefs) const;
  int event_on_label(const Fl_Tree_Prefs &prefs) const;
  /// Is this item the root of the tree?
  int is_root() const {
    return(_parent==0?1:0);
  }

  // Protected methods
protected:
#if FLTK_ABI_VERSION >= 10301
  /// Set a flag to an on or off value. val is 0 or 1.
  inline void set_flag(unsigned short flag,int val) {
    if ( val ) _flags |= flag; else _flags &= ~flag;
  }
  /// See if flag set. Returns 0 or 1.
  inline int is_flag(unsigned short val) const {
    return(_flags & val ? 1 : 0);
  }
#else /*FLTK_ABI_VERSION*/
  /// Set a flag to an on or off value. val is 0 or 1.
  void set_flag(unsigned short flag,int val) {
    switch (flag) {
      case     OPEN: _open     = val; break;
      case  VISIBLE: _visible  = val; break;
      case   ACTIVE: _active   = val; break;
      case SELECTED: _selected = val; break;
    }
  }
  /// See if flag set. Returns 0 or 1.
  int is_flag(unsigned short flag) const {
    switch (flag) {
      case     OPEN: return(_open ? 1 : 0);
      case  VISIBLE: return(_visible ? 1 : 0);
      case   ACTIVE: return(_active ? 1 : 0);
      case SELECTED: return(_selected ? 1 : 0);
      default:       return(0);
    }
  }
#endif /*FLTK_ABI_VERSION*/

};

#endif /*FL_TREE_ITEM_H*/

//
// End of "$Id: Fl_Tree_Item.H,v 1.1.1.1 2015/09/05 18:54:17 benandbon Exp $".
//
