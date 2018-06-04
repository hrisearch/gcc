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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "target.h"
#include "function.h"
#include "basic-block.h"
#include "tree.h"
#include "gimple.h"
#include "cgraph.h"
#include "lto-streamer.h"
#include "ipa-utils.h"
#include "builtins.h"
#include "alias.h"
#include "lto-symtab.h"
#include "stringpool.h"
#include "attribs.h"
#include "stdio.h"
#include "lto.h"

/* Dump everything.  */
void 
dump ()
{
	fprintf(stderr, "\nHello World!\n");
}
	
/* Dump variables and functions used in IL.  */
void
dump_list ()
{

	fprintf (stderr, "Symbol Table\n");
    symtab_node *node;
    fprintf (stderr, "\t\tName \t\tType \t\tVisibility\n");
	FOR_EACH_SYMBOL (node)
	{
		fprintf (stderr, "\n%20s",(flag_lto_dump_demangle) 
			? node->name (): node->dump_asm_name ());
		fprintf (stderr, "%20s", node->dump_type_name ());
		fprintf (stderr, "%20s\n", node->dump_visibility ());
	}
}

/* Dump specific variables and functions used in IL.  */
void
dump_symbol ()
{
	symtab_node *node;
    fprintf (stderr, "\t\tName \t\tType \t\tVisibility\n");
	FOR_EACH_SYMBOL (node)
	{
		if (!strcmp(flag_lto_dump_symbol, node->name()))
		{
			fprintf (stderr, "\n%20s",(flag_lto_dump_demangle) 
				? node->name (): node->dump_asm_name ());
		fprintf (stderr, "%20s", node->dump_type_name ());
		fprintf (stderr, "%20s\n", node->dump_visibility ());
		}
	}	
}	