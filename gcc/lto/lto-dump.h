/* LTO dump tool
   Copyright (C) 2009-2018 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_LTO_DUMP_H_
#define GCC_LTO_DUMP_H_

/* Dump everything.  */
void dump ();

/*Dump variables and function names used in IL.  */
void dump_list ();

/*Dump specific variable or function used in IL.  */
void dump_symbol ();

/*Dump gimple body of specific function.  */
void dump_body ();

#endif