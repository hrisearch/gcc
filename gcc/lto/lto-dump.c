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

/*Dump everything*/
void dump()
{
	fprintf(stderr, "\nHello World!\n");
}

/*Dump variables and functions used in IL*/
void dump_list()
{

	fprintf (stderr, "Call Graph:\n");
	cgraph_node *cnode;
	
	static const char * const symtab_type_names[] = {"symbol", "function", "variable"};
  	static const char * const visibility_types[] = {
    "default", "protected", "hidden", "internal" };
	FOR_EACH_FUNCTION (cnode)
	{
		fprintf (stderr, "\n%s (%s)", cnode->dump_asm_name (), cnode->name ());
		fprintf (stderr, "\n  Type: %s", symtab_type_names[cnode->type]);
		fprintf (stderr, "\n visibility: %s\n",
		visibility_types [DECL_VISIBILITY (cnode->decl)]);
	}

	fprintf (stderr, "\nVarpool:\n");
	varpool_node *vnode;
    FOR_EACH_VARIABLE (vnode)
    {
		fprintf (stderr, "\n%s (%s)", vnode->dump_asm_name (), vnode->name ());
		fprintf (stderr, "\n  Type: %s", symtab_type_names[vnode->type]);
		fprintf (stderr, "\n visibility:%s\n",
		visibility_types [DECL_VISIBILITY (vnode->decl)]);
	}
}

/*Dump specific variables and functions used in IL*/
void dump_list2()
{

	fprintf (stderr, "Call Graph:\n");
	cgraph_node *cnode;
	
	static const char * const symtab_type_names[] = {"symbol", "function", "variable"};
  	static const char * const visibility_types[] = {
    "default", "protected", "hidden", "internal" };
	FOR_EACH_FUNCTION (cnode)
	{
		if (!strcmp(flag_lto_dump_list2, cnode->name()))
		{
			fprintf (stderr, "\n%s (%s)", cnode->dump_asm_name (), cnode->name ());
			fprintf (stderr, "\n  Type: %s", symtab_type_names[cnode->type]);
			fprintf (stderr, "\n visibility: %s\n",
			visibility_types [DECL_VISIBILITY (cnode->decl)]);
		}
	}	
	fprintf (stderr, "\nVarpool:\n");
	varpool_node *vnode;
    FOR_EACH_VARIABLE (vnode)
    {
    	if (!strcmp(flag_lto_dump_list2, vnode->name()))
		{	
			fprintf (stderr, "\n%s (%s)", vnode->dump_asm_name (), vnode->name ());
			fprintf (stderr, "\n  Type: %s", symtab_type_names[vnode->type]);
			fprintf (stderr, "\n visibility:%s\n",
			visibility_types [DECL_VISIBILITY (vnode->decl)]);
		}
	}
}	