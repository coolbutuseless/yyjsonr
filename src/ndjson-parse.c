

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Connections.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <zlib.h>

#include "yyjson.h"
#include "R-yyjson-parse.h"
#include "R-yyjson-serialize.h"

#define MAX_LINE_LENGTH 131072
#define INIT_LIST_LENGTH 64


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Double the length of a list by
//   - allocating space for a list which is twice the length
//   - copy across all elements one-by-one
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP grow_list(SEXP oldlist) {
  R_xlen_t len = XLENGTH(oldlist);
  SEXP newlist = PROTECT(allocVector(VECSXP, 2 * len));
  for (R_xlen_t i=0; i < len; i++) {
    SET_VECTOR_ELT(newlist, i, VECTOR_ELT(oldlist, i));
  }
  UNPROTECT(1);
  return newlist;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Count the number of newlines in a file
// 'gz' lib handles compressed and uncompressed files.
//
// Two options for parsing streaming input from a file:
//   (1) calculate num of lines. Allocate this exactly. Parse file.
//       - PRO: Minimise re-allocation as data grows
//       - CON: have to traverse the file twice
//   (2) Start parsing and just double memory allocation whenever we run out
//     of room in the data.frame or list.
//       - PRO: Only traverse the file once
//       - CON: Have to spend effort re-allocating R object as it grows
//
//  For LIST objects, it's really easy to  groww their size. See 'grow_list_()'
//  and then just truncate it to the actual data size at the end.
//
// For data.frames, growing its size involves growing the size of every 
// column individually. For int/double, this is an easy re-allocation.
// For string STRSXP you'd have to grow them in the same manner as 'grow_list_()'
// grows VECSXP objects.  This seems like a lot of work in order to figure
// out if its any faster than just traversing the file twice in order to 
// count number of newlines.
//
// For now (2023-08-09), ndjson->list will use method 2 and 
// ndjson->data.frame will use method 1
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int count_lines(const char *filename) {
  char buf[MAX_LINE_LENGTH] = {0};
  int counter = 0;
  
  gzFile file = gzopen(filename, "r");
  
  for(;;) {
    size_t res = gzfread(buf, 1, MAX_LINE_LENGTH, file);
    
    int i;
    for(i = 0; i < res; i++) {
      if (buf[i] == '\n')
        counter++;
    }
    
    if (gzeof(file))
      break;
  }
  
  gzclose(file);
  return counter;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse ndjson as a list of R objects: one-r-object-per-line-of-input
// 
// Compared to parsing to data.frame
//   PRO: Simple
//   PRO: Can handle any type without worrying about data.frame column types
//        being consistent across multiple input lines
//   CON: Slower: Every object on every line gets allocated into an R object
//        Compared to data.frame which allocates all its space at once and
//        just slots values into this memory.
//
// @param filename filename containing ndjson data
// @param nread_limit number of lines to read
// @param nskip number of lines to skip before reading
// @param parse_opts list of options for parsing.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_ndjson_file_as_list_(SEXP filename_, SEXP nread_limit_, SEXP nskip_, SEXP parse_opts_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Buffer to read each line of the input file.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char buf[MAX_LINE_LENGTH] = {0};
  
  parse_options opt = create_parse_options(parse_opts_);
  
  int nread_limit = asInteger(nread_limit_);
  int nskip = asInteger(nskip_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check for file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *filename = (const char *)CHAR(STRING_ELT(filename_, 0));
  filename = R_ExpandFileName(filename);
  if (access(filename, R_OK) != 0) {
    error("Cannot read from file '%s'", filename);
  }
    
    
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Open file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gzFile input = gzopen(filename, "r");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (nskip > 0) {
    while (gzgets(input, buf, MAX_LINE_LENGTH) != 0) {
      nskip--;
      if (nskip == 0) break;
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocating a list with a default starting size to grow into.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  PROTECT_INDEX ipx;
  SEXP list_;
  PROTECT_WITH_INDEX(list_ = allocVector(VECSXP, 64), &ipx);
  R_xlen_t list_size = XLENGTH(list_);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over the file.  For each line
  //   - check if new data would overflow list
  //        - if so, then grow list
  //   - create a yyjson doc from this line
  //   - if document is NULL
  //        insert a NULL into list
  //   - otherwise 
  //        insert resulting robject into list
  //   - free the doc
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int nread_actual = 0;
  while (gzgets(input, buf, MAX_LINE_LENGTH) != 0) {
    
    if (nread_actual >= nread_limit) {
      break;
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Grow list if we need more room
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (nread_actual >= list_size) {
      REPROTECT(list_ = Rf_lengthgets(list_, 2 * list_size), ipx);
      list_size = XLENGTH(list_);
    }
    
    // ignore lines which are just a "\n".
    // might have to do something fancier for lines with just whitespace
    if (strlen(buf) <= 1) continue;
    
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(buf, strlen(buf), opt.yyjson_read_flag, NULL, &err);
    
    if (doc == NULL) {
      output_verbose_error(buf, err);
      warning("Couldn't parse NDJSON row %i. Inserting 'NULL'\n", nread_actual + 1);
      SET_VECTOR_ELT(list_, nread_actual, R_NilValue);
    } else {
      SET_VECTOR_ELT(list_, nread_actual, parse_json_from_str(buf, strlen(buf), &opt));
    }
    
    yyjson_doc_free(doc);
    
    nread_actual++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // 'list_' is oversized 
  // Need to copy list into a new list which contains just the valid elements
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  REPROTECT(list_ = Rf_lengthgets(list_, nread_actual), ipx);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Close input, tidy memory and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gzclose(input);
  UNPROTECT(1);
  return list_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse ndjson as a list of R objects: one-r-object-per-line-of-input
// 
// Compared to parsing to data.frame
//   PRO: Simple
//   PRO: Can handle any type without worrying about data.frame column types
//        being consistent across multiple input lines
//   CON: Slower: Every object on every line gets allocated into an R object
//        Compared to data.frame which allocates all its space at once and
//        just slots values into this memory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_ndjson_str_as_list_(SEXP str_, SEXP nread_, SEXP nskip_, SEXP parse_opts_) {
  
  parse_options opt = create_parse_options(parse_opts_);
  opt.yyjson_read_flag |= YYJSON_READ_STOP_WHEN_DONE;
  
  int nread = asInteger(nread_);
  int nskip = asInteger(nskip_);
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocating a list with a default starting size to grow into.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  PROTECT_INDEX ipx;
  SEXP list_;
  PROTECT_WITH_INDEX(list_ = allocVector(VECSXP, 64), &ipx);
  R_xlen_t list_size = XLENGTH(list_);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over the file.  For each line
  //   - check if new data would overflow list
  //        - if so, then grow list
  //   - create a yyjson doc from this line
  //   - if document is NULL
  //        insert a NULL into list
  //   - otherwise 
  //        insert resulting robject into list
  //   - free the doc
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *str = (char *)CHAR( STRING_ELT(str_, 0) );
  size_t str_size = strlen(str);
  size_t orig_str_size = strlen(str);
  size_t total_read = 0;
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while (nskip > 0 && total_read < orig_str_size) {
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
    size_t pos = yyjson_doc_get_read_size(doc);
    yyjson_doc_free(doc);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    total_read += pos + 1;
    str += pos + 1;
    str_size -= (pos + 1);
    
    nskip--;
  }
  
  
  unsigned int i = 0;
  while (total_read < orig_str_size) {
    
    if (i >= nread) {
      break;
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Grow list if we need more room
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (i >= list_size) {
      REPROTECT(list_ = Rf_lengthgets(list_, 2 * list_size), ipx);
      list_size = XLENGTH(list_);
    }
    
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
    size_t pos = yyjson_doc_get_read_size(doc);
    
    
    if (doc == NULL) {
      warning("Couldn't parse NDJSON row %i. Inserting 'NULL'\n", i + 1);
      SET_VECTOR_ELT(list_, i, R_NilValue);
    } else {
      SET_VECTOR_ELT(list_, i, parse_json_from_str(str, str_size, &opt));
    }
    i++;
    
    yyjson_doc_free(doc);
    
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    total_read += pos + 1;
    str += pos + 1;
    str_size -= (pos + 1);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // 'list_' is oversized 
  // Need to copy list into a new list which contains just the valid elements
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  REPROTECT(list_ = Rf_lengthgets(list_, i), ipx);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Close input, tidy memory and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  UNPROTECT(1);
  return list_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Truncate all the vectors in a list to the same length.
// This function operates in-situ
//
// @param df_ data.frame
// @param data_length the actual length of the data in the vector.
//        data_length <= allocated_length
// @param allocated_length the full length allocated for this vector
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void truncate_list_of_vectors(SEXP df_, int data_length, int allocated_length) {
  if (data_length != allocated_length) {
    for (int i=0; i < length(df_); i++) {
      SEXP trunc_ = PROTECT(Rf_lengthgets(VECTOR_ELT(df_, i), data_length));
      SET_VECTOR_ELT(df_, i, trunc_);
      UNPROTECT(1);
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Given a list of vectors (of all the same length), convert it to a 
// data.frame
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP promote_list_to_data_frame(SEXP df_, char **colname, int ncols) {
  
  int nprotect = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set colnames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(allocVector(STRSXP, ncols)); nprotect++;
  for (unsigned int i = 0; i < ncols; i++) {
    SET_STRING_ELT(nms_, i, mkChar(colname[i]));
  }
  Rf_setAttrib(df_, R_NamesSymbol, nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set empty rownames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int nrows = length(VECTOR_ELT(df_, 1));
  SEXP rownames = PROTECT(allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -nrows);
  setAttrib(df_, R_RowNamesSymbol, rownames);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 'data.frame' class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SET_CLASS(df_, mkString("data.frame"));
  
  UNPROTECT(nprotect);
  return df_;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse ndjson as a data.frame one-rorw-per-line-of-input
// 
// Compared to parsing to list
//   CON: Complex multi-column handling 
//   CON: in order to avoid re-allocation of memory as file is read, we have to
//        do an initial pass over the file to count the lines so we know the
//        number of rows.
//   CON: Have to probe the data.set to find out data types for each column.
//        This is done once at the start of the parse, and it is then
//        assumed all future types match the types seen so far.
//        This might not be true, but a comprimise I'm making for speed.
//   PRO: Faster. Data.frame allocation happens once, and data is slotted into 
//        it.  No re-allocation as we pre-determine the number of rows and 
//        type for each columnx
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_ndjson_file_as_df_(SEXP filename_, SEXP nread_, SEXP nskip_, SEXP nprobe_, SEXP parse_opts_) {
  
  int nprotect = 0;
  char buf[MAX_LINE_LENGTH] = {0};
  parse_options opt = create_parse_options(parse_opts_);
  const char *filename = (const char *)CHAR(STRING_ELT(filename_, 0));
  filename = R_ExpandFileName(filename);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check for file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (access(filename, R_OK) != 0) {
    error("Cannot read from file '%s'", filename);
  }
  
  
  int nread  = asInteger(nread_);
  int nskip  = asInteger(nskip_);
  int nprobe = asInteger(nprobe_);
  
  if (nread < 0) {
    nread = INT32_MAX;
  }
  
  if (nprobe < 0) {
    nprobe = INT32_MAX;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get the maximum possible number of json rows in this ndjson file.
  // Note: the actual number of rows to parse may be less than this due
  // to blank lines and/or errors.
  // 'nrows' controls the amount of memory pre-allocated for rows in the df.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int nrows = count_lines(filename);
  
  // Account for rows to be skipped
  nrows = nrows - nskip;
  if (nrows < 0) {
    nrows = 0;
  }
  
  // Ensure we don't read more than the user requested
  if (nrows > nread) {
    nrows = nread;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Accumulation of unique key-names in the objects
  // These will become the column names of the data.frame.
  // Each column also has a 'type_bitset' to keep track of the type of each
  // value across the different {}-objects
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *colname[MAX_DF_COLS] = { 0 };
  unsigned int type_bitset[MAX_DF_COLS] = {0};
  unsigned int sexp_type[MAX_DF_COLS] = {0};
  int ncols = 0;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Probe file for types
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gzFile input = gzopen(filename, "r");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (nskip > 0) {
    int nskip2 = nskip;
    while (gzgets(input, buf, MAX_LINE_LENGTH) != 0) {
      nskip2--;
      if (nskip2 == 0) break;
    }
  }
  
  for (unsigned int i = 0; i < nprobe; i++) {
    char *ret = gzgets(input, buf, MAX_LINE_LENGTH);
    if (ret == NULL) {
      break;
    }
    
    
    // ignore lines which are just a "\n".
    // might have to do something fancier for lines with just whitespace
    if (strlen(buf) <= 1) continue;
    
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(buf, strlen(buf), opt.yyjson_read_flag, NULL, &err);
    if (doc == NULL) {
      output_verbose_error(buf, err);
      error("Couldn't parse JSON during probe line %i\n", i + 1);
    }
    
    yyjson_val *obj = yyjson_doc_get_root(doc);
    yyjson_val *key;
    yyjson_obj_iter obj_iter = yyjson_obj_iter_with(obj); // MUST be an object
    
    while ((key = yyjson_obj_iter_next(&obj_iter))) {
      yyjson_val *val = yyjson_obj_iter_get_val(key);
      
      int name_idx = -1;
      for (int i = 0; i < ncols; i++) {
        if (yyjson_equals_str(key, colname[i])) {
          name_idx = i;
          break;
        }
      }
      if (name_idx < 0) {
        // Name has not been seen yet.
        // Need to copy the string as the 'doc' it is from is freed at the end of every loop
        name_idx = ncols;
        char *new_name = (char *)yyjson_get_str(key);
        colname[ncols] = calloc(strlen(new_name) + 1, 1);
        if (colname[ncols] == 0) Rf_error("Failed to allocate 'colname'");
        strncpy(colname[ncols], new_name, strlen(new_name));
        ncols++;
        if (ncols == MAX_DF_COLS) {
          error("Maximum columns for data.frame exceeded: %i", MAX_DF_COLS);
        }
      }
      
      type_bitset[name_idx] = update_type_bitset(type_bitset[name_idx], val, &opt);
    }
    
    yyjson_doc_free(doc);
  }
  
  gzclose(input);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a list (which will be promoted to a data.frame before returning)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP df_ = PROTECT(allocVector(VECSXP, ncols)); nprotect++;
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For each column name,
  //   - determine the best SEXP to represent the 'type_bitset'
  //   - Call a parse function which will
  //        - loop through the entire []-array, plucking the value from each
  //          {}-object
  //        - return an atomic vector or a list
  //   - place this vector as a column in the data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (unsigned int col = 0; col < ncols; col++) {
    sexp_type[col] = get_best_sexp_to_represent_type_bitset(type_bitset[col], &opt);
    
    // INT64SXP is actually contained in a REALSXP
    unsigned int alloc_type = sexp_type[col] == INT64SXP ? REALSXP : sexp_type[col];
    
    // Allocate memory for column
    SEXP vec_ = PROTECT(allocVector(alloc_type, nrows));
    if (sexp_type[col] == INT64SXP) {
      setAttrib(vec_, R_ClassSymbol, mkString("integer64"));
    }
    
    // place vector into data.frame
    SET_VECTOR_ELT(df_, col, vec_);
    UNPROTECT(1); // no longer needs protection once part of data.frame
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  input = gzopen(filename, "r");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (nskip > 0) {
    while (gzgets(input, buf, MAX_LINE_LENGTH) != 0) {
      nskip--;
      if (nskip == 0) break;
    }
  }
  
  // keep track of actual number of rows parsed.
  // This might not be the same as 'nrow' as we can skip rows that we 
  // can't parse.
  int row = 0;
  
  for (unsigned int i = 0; i < nrows; i++) {
    char *ret = gzgets(input, buf, MAX_LINE_LENGTH);
    if (ret == NULL) {
      error("Unexepcted end to data\n");
    }
    
    // ignore lines which are just a "\n".
    // might have to do something fancier for lines with just whitespace
    if (strlen(buf) <= 1) continue;
    
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(buf, strlen(buf), opt.yyjson_read_flag, NULL, &err);
    if (doc == NULL) {
      output_verbose_error(buf, err);
      error("Couldn't parse JSON on line %i\n", i + 1);
    }
    
    yyjson_val *obj = yyjson_doc_get_root(doc);
    if (yyjson_get_type(obj) != YYJSON_TYPE_OBJ) {
      error("parse_ndjson_as_df() only works if all lines represent JSON objects");
    }
    
    for (unsigned int col = 0; col < ncols; col++) {
      SEXP column_ = VECTOR_ELT(df_, col);
      
      yyjson_val *val = yyjson_obj_get(obj, colname[col]);
      
      switch(sexp_type[col]) {
      case LGLSXP:
        LOGICAL(column_)[row] = json_val_to_logical(val, &opt);
        break;
      case INTSXP:
        INTEGER(column_)[row] = json_val_to_integer(val, &opt);
        break;
      case INT64SXP: {
        long long tmp = json_val_to_integer64(val, &opt);
        ((long long *)(REAL(column_)))[row] = tmp;
      }
        break;
      case REALSXP:
        REAL(column_)[row] = json_val_to_double(val, &opt);
        break;
      case STRSXP:
        if (val == NULL) {
          SET_STRING_ELT(column_, row, NA_STRING);
        } else {
          SET_STRING_ELT(column_, row, json_val_to_charsxp(val, &opt));
        }
        break;
      case VECSXP:
        if (val == NULL) {
          SET_VECTOR_ELT(column_, row, opt.df_missing_list_elem);
        } else {
          SET_VECTOR_ELT(column_, row, json_as_robj(val, &opt));
        }
        break;
      default:
        error("parse_ndjson_file_as_df_(): Unknown type");
      } 
      
    }
    
    yyjson_doc_free(doc);
    row++;
  }
  
  gzclose(input);
  
  
  truncate_list_of_vectors(df_, row, nrows);
  SEXP df_final_ = PROTECT(promote_list_to_data_frame(df_, colname, ncols)); nprotect++;
  
  // 'colname' strings were allocated and copied from their original yyjson docs. FREE!
  for (int i = 0; i < ncols; i++) {
    free(colname[i]);
  }
  
  
  UNPROTECT(nprotect);
  return df_final_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse string into data.frame
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_ndjson_str_as_df_(SEXP str_, SEXP nread_, SEXP nskip_, SEXP nprobe_, SEXP parse_opts_) {
  
  int nprotect = 0;
  parse_options opt = create_parse_options(parse_opts_);
  opt.yyjson_read_flag |= YYJSON_READ_STOP_WHEN_DONE;
  
  int nread  = asInteger(nread_);
  int nskip  = asInteger(nskip_);
  int nprobe = asInteger(nprobe_);
  
  if (nread  <= 0) { nread  = INT32_MAX; }
  if (nprobe <= 0) { nprobe = INT32_MAX; }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over the file.  For each line
  //   - check if new data would overflow list
  //        - if so, then grow list
  //   - create a yyjson doc from this line
  //   - if document is NULL
  //        insert a NULL into list
  //   - otherwise 
  //        insert resulting robject into list
  //   - free the doc
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *str = (char *)CHAR( STRING_ELT(str_, 0) );
  size_t str_size = strlen(str);
  size_t orig_str_size = strlen(str);
  size_t total_read = 0;
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while (nskip > 0 && total_read < orig_str_size) {
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
    size_t pos = yyjson_doc_get_read_size(doc);
    yyjson_doc_free(doc);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    total_read += pos + 1;
    str += pos + 1;
    str_size -= (pos + 1);
    
    nskip--;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Accumulation of unique key-names in the objects
  // These will become the column names of the data.frame.
  // Each column also has a 'type_bitset' to keep track of the type of each
  // value across the different {}-objects
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *colname[MAX_DF_COLS] = { 0 };
  unsigned int type_bitset[MAX_DF_COLS] = {0};
  unsigned int sexp_type[MAX_DF_COLS] = {0};
  int ncols = 0;
  int nrows = 0;
  
  
  char *mark_str = str;
  size_t mark_str_size = str_size;
  size_t mark_total_read = total_read;
  
  
  while (nprobe > 0 && total_read < orig_str_size) {
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
    size_t pos = yyjson_doc_get_read_size(doc);
    if (doc == NULL) {
      // output_verbose_error(buf, err);
      error("Couldn't parse JSON during probe line %i\n", nrows + 1);
    }
    
    yyjson_val *obj = yyjson_doc_get_root(doc);
    yyjson_val *key;
    yyjson_obj_iter obj_iter = yyjson_obj_iter_with(obj); // MUST be an object
    
    while ((key = yyjson_obj_iter_next(&obj_iter))) {
      yyjson_val *val = yyjson_obj_iter_get_val(key);
      
      int name_idx = -1;
      for (int i = 0; i < ncols; i++) {
        if (yyjson_equals_str(key, colname[i])) {
          name_idx = i;
          break;
        }
      }
      if (name_idx < 0) {
        // Name has not been seen yet
        name_idx = ncols;
        char *new_name = (char *)yyjson_get_str(key);
        colname[ncols] = calloc(strlen(new_name) + 1, 1);
        strncpy(colname[ncols], new_name, strlen(new_name));
        ncols++;
        if (ncols == MAX_DF_COLS) {
          error("Maximum columns for data.frame exceeded: %i", MAX_DF_COLS);
        }
      }
      
      type_bitset[name_idx] = update_type_bitset(type_bitset[name_idx], val, &opt);
    }
    
    yyjson_doc_free(doc);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    total_read += pos + 1;
    str += pos + 1;
    str_size -= (pos + 1);
    
    
    nrows++;    
    nprobe--; 
  }
  // Rprintf("Step X0: nrows = %i\n", nrows);
  
  // json <- write_ndjson_str(head(mtcars)); read_ndjson_str(json)
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read the rest of the string to figure out how many rows there are in total
  // TODO: Just count "\n" here
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (total_read < orig_str_size) {
    for (size_t sp = 0; sp < str_size; sp++) {
      if (str[sp] == '\n') {
        nrows++;
      }
    }
    if (str[str_size =1] != '\n') {
      // STring does not end in newline, so need to manually count the last row
      nrows++;
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many rows does the user want to read vs how many do we have
  // and how many they want to skip.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  nrows = nrows > nread ? nread : nrows;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a list to hold vectors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP df_ = PROTECT(allocVector(VECSXP, ncols)); nprotect++;
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For each column name,
  //   - determine the best SEXP to represent the 'type_bitset'
  //   - Call a parse function which will
  //        - loop through the entire []-array, plucking the value from each
  //          {}-object
  //        - return an atomic vector or a list
  //   - place this vector as a column in the data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (unsigned int col = 0; col < ncols; col++) {
    sexp_type[col] = get_best_sexp_to_represent_type_bitset(type_bitset[col], &opt);
    
    // INT64SXP is actually contained in a REALSXP
    unsigned int alloc_type = sexp_type[col] == INT64SXP ? REALSXP : sexp_type[col];
    
    // Allocate memory for column
    SEXP vec_ = PROTECT(allocVector(alloc_type, nrows));
    if (sexp_type[col] == INT64SXP) {
      setAttrib(vec_, R_ClassSymbol, mkString("integer64"));
    }
    
    // place vector into list
    SET_VECTOR_ELT(df_, col, vec_);
    UNPROTECT(1); // no longer needs protection once part of data.frame
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  str        = mark_str;
  str_size   = mark_str_size;
  total_read = mark_total_read;
  
  // keep track of actual number of rows parsed.
  // This might not be the same as 'nrow' as we can skip rows that we 
  // can't parse.
  int row = 0;
  
  for (unsigned int i = 0; i < nrows; i++) {
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
    size_t pos = yyjson_doc_get_read_size(doc);
    if (doc == NULL) {
      // output_verbose_error(buf, err);
      error("Couldn't parse JSON on line %i\n", i + 1);
    }
    
    yyjson_val *obj = yyjson_doc_get_root(doc);
    if (yyjson_get_type(obj) != YYJSON_TYPE_OBJ) {
      error("parse_ndjson_as_df() only works if all lines represent JSON objects");
    }
    
    for (unsigned int col = 0; col < ncols; col++) {
      SEXP column_ = VECTOR_ELT(df_, col);
      
      yyjson_val *val = yyjson_obj_get(obj, colname[col]);
      
      switch(sexp_type[col]) {
      case LGLSXP:
        LOGICAL(column_)[row] = json_val_to_logical(val, &opt);
        break;
      case INTSXP:
        INTEGER(column_)[row] = json_val_to_integer(val, &opt);
        break;
      case INT64SXP: {
        long long tmp = json_val_to_integer64(val, &opt);
        ((long long *)(REAL(column_)))[row] = tmp;
      }
        break;
      case REALSXP:
        REAL(column_)[row] = json_val_to_double(val, &opt);
        break;
      case STRSXP:
        if (val == NULL) {
          SET_STRING_ELT(column_, row, NA_STRING);
        } else {
          SET_STRING_ELT(column_, row, json_val_to_charsxp(val, &opt));
        }
        break;
      case VECSXP:
        if (val == NULL) {
          SET_VECTOR_ELT(column_, row, opt.df_missing_list_elem);
        } else {
          SET_VECTOR_ELT(column_, row, json_as_robj(val, &opt));
        }
        break;
      default:
        error("parse_ndjson_file_as_df_(): Unknown type");
      } 
      
    }
    
    yyjson_doc_free(doc);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    total_read += pos + 1;
    str += pos + 1;
    str_size -= (pos + 1);
    
    
    row++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Promote the 'list' of accumulated vectors to be a real 'data.frame'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  truncate_list_of_vectors(df_, row, nrows);
  df_ = PROTECT(promote_list_to_data_frame(df_, colname, ncols));  nprotect++;
  
  // Free the 'colnames' we copied out of JSON docs when probing
  for (int i = 0; i < ncols; i++) {
    free(colname[i]);
  }
  
  
  UNPROTECT(nprotect);
  return df_;
}
