/* Top-level LTO routines.
   Copyright (C) 2009-2018 Free Software Foundation, Inc.
   Contributed by CodeSourcery, Inc.

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
#include "cfghooks.h"
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
#include "lto-common.h"

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
