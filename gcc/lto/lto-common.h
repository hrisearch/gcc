#ifndef LTO_COMMON_H
#define LTO_COMMON_H

void
lto_init (void);

void
read_cgraph_and_symbols (unsigned nfiles, const char **fnames);

extern int real_file_count;

//extern GTY((length ("real_file_count + 1"))) struct lto_file_decl_data **real_file_decl_data;
extern struct lto_file_decl_data **real_file_decl_data;
extern struct lto_file_decl_data **all_file_decl_data;

extern GTY(()) tree lto_eh_personality_decl;

extern GTY(()) vec <tree, va_gc> *tree_with_vars;

extern GTY(()) const unsigned char *lto_mode_identity_table;

extern GTY(()) tree first_personality_decl;


#endif