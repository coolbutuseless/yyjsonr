
#define MAX_LINE_LENGTH 131072
SEXP grow_list(SEXP oldlist);
int count_lines(const char *filename);
void truncate_list_of_vectors(SEXP df_, int data_length, int allocated_length);
SEXP promote_list_to_data_frame(SEXP df_, char **colname, int ncols);
int df_col_idx(SEXP df_, const char *nm);

