/* Functions for LTO dump tool.
   Copyright (C) 2018 Free Software Foundation, Inc.

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
#include "tm.h"
#include "function.h"
#include "basic-block.h"
#include "tree.h"
#include "gimple.h"
#include "cfg.h"
#include "tree-cfg.h"
#include "tree-pass.h"
#include "tree-streamer.h"
#include "cgraph.h"
#include "opts.h"
#include "debug.h"
#include "lto-partition.h"
#include "tree-pretty-print.h"
#include "lto-common.h"

/* Stores details of symbols for dumping symbol list.  */

struct symbol_entry
{
  symtab_node *node;
  symbol_entry (symtab_node *node_): node (node_)
  {}
  char* get_name ()
  {
    if (flag_lto_dump_demangle)
      return xstrdup (node->name ());
    else
      return xstrdup (node->asm_name ());

  }

  virtual size_t get_size () = 0;
  virtual void dump () = 0;
};

/* Stores variable specific details of symbols for dumping symbol list.  */

struct variable_entry: public symbol_entry
{
  variable_entry (varpool_node *node_): symbol_entry (node_)
  {}
  virtual size_t get_size ()
  {
    varpool_node *vnode = (varpool_node*)node;
    if (DECL_SIZE (vnode->decl) && tree_fits_shwi_p (DECL_SIZE (vnode->decl)))
      return tree_to_shwi (DECL_SIZE (vnode->decl));
    return 0;
  }
  virtual void dump ()
  {
    const char *name = get_name ();
    const char *type_name = node->dump_type_name ();
    const char *visibility = node->dump_visibility ();
    size_t sz = get_size ();
    varpool_node *vnode = (varpool_node*)node;
    vnode->get_constructor ();
    tree value_tree = DECL_INITIAL (vnode->decl);
    fprintf (stdout,"%10s %10s %10s %10zu\t", name, type_name, visibility, sz);
    if (flag_lto_print_value && value_tree)
      debug_generic_expr (value_tree);
    else
      fprintf (stdout, "\n");
  }
};

/* Stores function specific details of symbols for dumping symbol list.  */

struct function_entry: public symbol_entry
{
  function_entry (cgraph_node *node_): symbol_entry (node_)
  {}

  virtual size_t get_size ()
  {
    cgraph_node *cnode = dyn_cast<cgraph_node *> (node);
    gcc_assert (cnode);

    return (cnode->definition)
	   ? n_basic_blocks_for_fn (DECL_STRUCT_FUNCTION (cnode->decl))
	   : 0;
  }
  void dump ()
  {
    const char *name = get_name ();
    const char *type_name = node->dump_type_name ();
    const char *visibility = node->dump_visibility ();
    size_t sz = get_size ();
    fprintf (stdout,"%10s %10s %10s %10zu\n", name, type_name, visibility, sz);
  }
};

/* Comparing symbols based on size.  */

int size_compare (const void *a, const void *b)
{
  symbol_entry *e1 = *(symbol_entry **) a;
  symbol_entry *e2 = *(symbol_entry **) b;

  return e1->get_size () - e2->get_size ();
}

/* Comparing symbols based on name.  */

int name_compare (const void *a, const void *b)
{
  symbol_entry *e1 = *(symbol_entry **) a;
  symbol_entry *e2 = *(symbol_entry **) b;

  return strcmp (e1->get_name (), e2->get_name ());
}

/* Dump list of functions and their details.  */

void dump_list_functions (void)
{
  auto_vec<symbol_entry *> v;

  cgraph_node *cnode;
  FOR_EACH_FUNCTION (cnode)
  {
    if (cnode->definition)
      cnode->get_untransformed_body ();
    symbol_entry *e = new function_entry (cnode);
    if (!flag_lto_dump_defined || cnode->definition)
      v.safe_push (e);
  }

  if (!flag_lto_no_sort)
  {
    if (flag_lto_size_sort)
      v.qsort (size_compare);
    else if (flag_lto_name_sort)
      v.qsort (name_compare);
  }
  if (flag_lto_reverse_sort)
    v.reverse ();

  fprintf (stdout, "\n     Name     Type     Visibility      Size");
  if (flag_lto_print_value)
    fprintf (stdout, "    Value");
  fprintf (stdout, "\n\n");

  int i=0;
  symbol_entry* e;
  FOR_EACH_VEC_ELT (v, i, e)
    e->dump ();
}

/* Dump list of variables and their details.  */

void dump_list_variables (void)
{
  auto_vec<symbol_entry *> v;

  varpool_node *vnode;
  FOR_EACH_VARIABLE (vnode)
  {
    symbol_entry *e = new variable_entry (vnode);
    if (!flag_lto_dump_defined || vnode->definition)
      v.safe_push (e);
  }

  if (!flag_lto_no_sort)
  {
    if (flag_lto_size_sort)
      v.qsort (size_compare);
    else if (flag_lto_name_sort)
      v.qsort (name_compare);
  }


  if (flag_lto_reverse_sort)
    v.reverse ();

  fprintf (stdout, "\n");
  int i=0;
  symbol_entry* e;
  FOR_EACH_VEC_ELT (v, i, e)
    e->dump ();
}

/* Dump symbol list.  */

void dump_list (void)
{
  dump_list_functions ();
  dump_list_variables ();
  exit (0);
}

/* Dump specific variables and functions used in IL.  */
void dump_symbol ()
{
  symtab_node *node;
  fprintf (stdout, "Symbol: %s\n", flag_lto_dump_symbol);
  FOR_EACH_SYMBOL (node)
    if (!strcmp (flag_lto_dump_symbol, node->name ()))
      node->debug ();
  fprintf (stdout, "\n");
  exit (0);
}

/* Dump specific gimple body of specified function.  */
void dump_body ()
{
  dump_flags_t flags;
  flags = (flag_dump_level)
	 ? parse_dump_option (flag_dump_level, 0, 0)
	 : TDF_NONE;
  cgraph_node *cnode;
  FOR_EACH_FUNCTION (cnode)
  {
    if (cnode->definition && !strcmp (cnode->name (), flag_dump_body))
    {
      fprintf (stdout, "Gimple Body of Function: %s\n", cnode->name ());
      cnode->get_untransformed_body ();
      debug_function (cnode->decl, flags);
    }
  }
    exit (0);
}

/* List of command line options for dumping.  */
void dump_tool_help ()
{
  fprintf (stdout, "\nLTO dump tool command line options.\n\n");
  fprintf (stdout, "-list : Dump the symbol list.\n");
  fprintf (stdout, "    -demangle : Dump the demangled output.\n");
  fprintf (stdout, "    -defined-only : Dump only the defined symbols.\n");
  fprintf (stdout, "    -print-value : Dump initial values of the variables.\n");
  fprintf (stdout, "    -name-sort : Sort the symbols alphabetically.\n");
  fprintf (stdout, "    -size-sort : Sort the symbols according to size.\n");
  fprintf (stdout, "    -reverse-sort : Dump the symbols in reverse order.\n");
  fprintf (stdout, "    -no-sort : Dump the symbols in order of occurence.\n");
  fprintf (stdout, "-symbol= : Dump the details of specific symbol.\n");
  fprintf (stdout, "-objects= : Dump the details of LTO objects.\n");
  fprintf (stdout, "-type-stats : Dump statistics of tree types.\n");
  fprintf (stdout, "-tree-stats : Dump statistics of trees.\n");
  fprintf (stdout, "-gimple-stats : Dump statistics of gimple statements.\n");
  fprintf (stdout, "-dump-level= : Deciding the optimization level of body.\n");
  fprintf (stdout, "-dump-body= : Dump the specific gimple body.\n");
  fprintf (stdout, "-help : Display the dump tool help.\n");
  exit (0);
}

/* Functions for dumping various details in LTO dump tool are called
   in lto_main(). The purpose of this dump tool is to analyze the LTO
   object files.  */

void
lto_main (void)
{
  quiet_flag = true;
  if (flag_lto_dump_tool_help)
    dump_tool_help ();

  /* LTO is called as a front end, even though it is not a front end.
     Because it is called as a front end, TV_PHASE_PARSING and
     TV_PARSE_GLOBAL are active, and we need to turn them off while
     doing LTO.  Later we turn them back on so they are active up in
     toplev.c.  */
  timevar_pop (TV_PARSE_GLOBAL);
  timevar_stop (TV_PHASE_PARSING);

  timevar_start (TV_PHASE_SETUP);

  /* Initialize the LTO front end.  */
  lto_init ();

  timevar_stop (TV_PHASE_SETUP);
  timevar_start (TV_PHASE_STREAM_IN);

  /* Read all the symbols and call graph from all the files in the
     command line.  */
  read_cgraph_and_symbols (num_in_fnames, in_fnames);

  /* Dump symbol list.  */
  if (flag_lto_dump_list)
    dump_list ();

  /* Dump specific variables and functions used in IL.  */
  if (flag_lto_dump_symbol)
    dump_symbol ();

  /* Dump gimple statement statistics.  */
  if (flag_lto_gimple_stats)
  {
    cgraph_node *node;
    FOR_EACH_DEFINED_FUNCTION (node)
    node->get_untransformed_body ();
    dump_gimple_statistics ();
    exit (0);
  }

  /* Dump tree statistics.  */
  if (flag_lto_tree_stats)
  {
    fprintf (stdout, "Tree Statistics\n");
    dump_tree_statistics ();
    exit (0);
  }

  /* Dump specific gimple body of specified function.  */
  if (flag_dump_body)
    dump_body ();

  timevar_stop (TV_PHASE_STREAM_IN);

  /* Here we make LTO pretend to be a parser.  */
  timevar_start (TV_PHASE_PARSING);
  timevar_push (TV_PARSE_GLOBAL);

}
