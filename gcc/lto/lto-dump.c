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
#include "tree-cfg.h"
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
    	const char *x = strchr (node->asm_name (), '/');
    	if (flag_lto_dump_demangle)
			fprintf (stderr, "\n%20s", node->name ());
		else
			fprintf (stderr, "\n%20s", node->asm_name (), 
				node->asm_name ()-x);
		fprintf (stderr, "%20s", node->dump_type_name ());
		fprintf (stderr, "%20s", node->dump_visibility ());
	}
}

/* Dump specific variables and functions used in IL.  */
void
dump_symbol ()
{
	symtab_node *node;
    fprintf (stderr, "\t\tName \t\tType \t\tVisibility\n");
	FOR_EACH_SYMBOL (node)
		if (!strcmp (flag_lto_dump_symbol, node->name ()))
			node->debug ();
}

/* Dump gimple body of specific function.  */
void
dump_body ()
{
	cgraph_node *cnode;
	FOR_EACH_FUNCTION (cnode)
		if (!strcmp (cnode->name (), flag_lto_dump_body))
		{
			cnode->get_untransformed_body ();
			debug_function (cnode->decl, 0);
		}
}	