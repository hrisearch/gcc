/* Top-level LTO routines.
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
#include "tm.h"
#include "function.h"
#include "bitmap.h"
#include "basic-block.h"
#include "tree.h"
#include "gimple.h"
#include "cfg.h"
#include "cfghooks.h"
#include "tree-cfg.h"
#include "alloc-pool.h"
#include "tree-pass.h"
#include "tree-streamer.h"
#include "cgraph.h"
#include "opts.h"
#include "toplev.h"
#include "stor-layout.h"
#include "symbol-summary.h"
#include "tree-vrp.h"
#include "ipa-prop.h"
#include "common.h"
#include "debug.h"
#include "lto.h"
#include "lto-section-names.h"
#include "splay-tree.h"
#include "lto-partition.h"
#include "context.h"
#include "pass_manager.h"
#include "ipa-fnsummary.h"
#include "params.h"
#include "ipa-utils.h"
#include "gomp-constants.h"
#include "lto-symtab.h"
#include "stringpool.h"
#include "fold-const.h"
#include "attribs.h"
#include "builtins.h"
#include "tree-pretty-print.h"
#include "lto-common.h"

struct symbol_entry
{
  symtab_node *node;
  symbol_entry (symtab_node *node_): node (node_)
  {}
  char* get_name ()
  {
    if (flag_lto_dump_demangle)
      return xstrdup (node->name ());
    else if (flag_lto_dump_no_demangle)
      return xstrdup (node->asm_name ());
    else
      return xstrdup (node->asm_name ());

  }

  virtual size_t get_size () = 0;
  virtual void dump () = 0;
};

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
    fprintf (stderr,"%10s %10s %10s %10zu\t", name, type_name, visibility, sz);
    if (flag_lto_print_value && value_tree)
      debug_generic_expr (value_tree);
    else
      fprintf (stderr, "\n");
  }
};


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
    fprintf (stderr,"%10s %10s %10s %10zu\n", name, type_name, visibility, sz);
  }
};


int size_compare (const void *a, const void *b)
{
  symbol_entry *e1 = *(symbol_entry **) a;
  symbol_entry *e2 = *(symbol_entry **) b;

  return e1->get_size () - e2->get_size ();
}

int alpha_compare (const void *a, const void *b)
{
  symbol_entry *e1 = *(symbol_entry **) a;
  symbol_entry *e2 = *(symbol_entry **) b;

  return strcmp (e1->get_name (), e2->get_name ());
}


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
    else if (flag_lto_alpha_sort)
      v.qsort (alpha_compare);
  }
  if (flag_lto_reverse_sort)
    v.reverse ();

  fprintf (stderr, "\n\tName\tType\tVisibility\tSize");
  if (flag_lto_print_value)
    fprintf (stderr, "\tValue");
  fprintf (stderr, "\n\n");

  int i=0;
  symbol_entry* e;
  FOR_EACH_VEC_ELT (v, i, e)
    e->dump ();
}


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
    else if (flag_lto_alpha_sort)
      v.qsort (alpha_compare);
  }


  if (flag_lto_reverse_sort)
    v.reverse ();

  fprintf (stderr, "\n");
  int i=0;
  symbol_entry* e;
  FOR_EACH_VEC_ELT (v, i, e)
    e->dump ();
}

void dump_list (void)
{
  dump_list_functions ();
  dump_list_variables ();
}

/* Dump specific variables and functions used in IL.  */
void
dump_symbol ()
{
  symtab_node *node;
  fprintf (stderr, "Symbol:\t%s\n", flag_lto_dump_symbol);
  FOR_EACH_SYMBOL (node)
    if (!strcmp (flag_lto_dump_symbol, node->name ()))
      node->debug ();
  fprintf (stderr, "\n");
}

/* Dump specific gimple body of specified function.  */
void
dump_body ()
{
  dump_flags_t flags;

  char buf[100];
  sprintf (buf, "-level=%s", flag_dump_level);
  flags = parse_dump_option (buf);

  cgraph_node *cnode;
  FOR_EACH_FUNCTION (cnode)
  {
    if (cnode->definition && !strcmp (cnode->name (), flag_dump_body))
    {
      fprintf (stderr, "Gimple body of function: %s\n", cnode->name ());
      cnode->get_untransformed_body ();
      debug_function (cnode->decl, flags);
    }
  }
}

/* Main entry point for the GIMPLE front end.  This front end has
   three main personalities:

   - LTO (-flto).  All the object files on the command line are
     loaded in memory and processed as a single translation unit.
     This is the traditional link-time optimization behavior.

   - WPA (-fwpa).  Only the callgraph and summary information for
     files in the command file are loaded.  A single callgraph
     (without function bodies) is instantiated for the whole set of
     files.  IPA passes are only allowed to analyze the call graph
     and make transformation decisions.  The callgraph is
     partitioned, each partition is written to a new object file
     together with the transformation decisions.

   - LTRANS (-fltrans).  Similar to -flto but it prevents the IPA
     summary files from running again.  Since WPA computed summary
     information and decided what transformations to apply, LTRANS
     simply applies them.  */

void
lto_main (void)
{
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
  }

  /* Dump tree statistics.  */
  if (flag_lto_tree_stats)
  {
    fprintf (stderr, "Tree Statistics\n");
    dump_tree_statistics ();
  }

  /* Dump specific gimple body of specified function.  */
  if (flag_dump_level && flag_dump_body)
    dump_body ();

  timevar_stop (TV_PHASE_STREAM_IN);

  if (!seen_error ())
    {
      offload_handle_link_vars ();

      /* If WPA is enabled analyze the whole call graph and create an
	 optimization plan.  Otherwise, read in all the function
	 bodies and continue with optimization.  */
      if (flag_wpa)
	do_whole_program_analysis ();
      else
	{
	  timevar_start (TV_PHASE_OPT_GEN);

	  materialize_cgraph ();
	  if (!flag_ltrans)
	    lto_promote_statics_nonwpa ();

	  /* Annotate the CU DIE and mark the early debug phase as finished.  */
	  debug_hooks->early_finish ("<artificial>");

	  /* Let the middle end know that we have read and merged all of
	     the input files.  */ 
	  symtab->compile ();

	  timevar_stop (TV_PHASE_OPT_GEN);

	  /* FIXME lto, if the processes spawned by WPA fail, we miss
	     the chance to print WPA's report, so WPA will call
	     print_lto_report before launching LTRANS.  If LTRANS was
	     launched directly by the driver we would not need to do
	     this.  */
	  if (flag_lto_report || (flag_wpa && flag_lto_report_wpa))
	    print_lto_report_1 ();
	}
    }

  /* Here we make LTO pretend to be a parser.  */
  timevar_start (TV_PHASE_PARSING);
  timevar_push (TV_PARSE_GLOBAL);
}

#include "gt-lto-lto.h"
