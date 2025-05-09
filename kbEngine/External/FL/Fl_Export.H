/*
 * "$Id: Fl_Export.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $"
 *
 * WIN32 DLL export .
 *
 * Copyright 1998-2010 by Bill Spitzak and others.
 *
 * This library is free software. Distribution and use rights are outlined in
 * the file "COPYING" which should have been included with this file.  If this
 * file is missing or damaged, see the license at:
 *
 *     http://www.fltk.org/COPYING.php
 *
 * Please report all bugs and problems on the following page:
 *
 *     http://www.fltk.org/str.php
 */

#ifndef Fl_Export_H
#  define Fl_Export_H

/*
 * The following is only used when building DLLs under WIN32...
 */

#  if defined(FL_DLL)
#    ifdef FL_LIBRARY
#      define FL_EXPORT	__declspec(dllexport)
#    else
#      define FL_EXPORT	__declspec(dllimport)
#    endif /* FL_LIBRARY */
#  else
#    define FL_EXPORT
#  endif /* FL_DLL */

#endif /* !Fl_Export_H */

/*
 * End of "$Id: Fl_Export.H,v 1.1.1.1 2015/09/05 18:54:16 benandbon Exp $".
 */
