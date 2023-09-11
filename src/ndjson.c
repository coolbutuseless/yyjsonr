

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

#define MAX_LINE_LENGTH 10000
#define INIT_LIST_LENGTH 64


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Double the length of a list by
//   - allocating space for a list which is twice the length
//   - copy across all elements one-by-one
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP grow_list(SEXP oldlist) {
  R_len_t len = XLENGTH(oldlist);
  SEXP newlist = PROTECT(allocVector(VECSXP, 2 * len));
  for (R_len_t i=0; i < len; i++) {
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
//   (2) Start parsing and just double memory allocation whenver we run out
//     of room in the data.frame or list.
//       - PRO: Only traverse the file once
//       - CON: Have to spend effort re-allocating R object as it grows
//
//  For LIST objects, it's really easy to  gorw their size. See 'grow_list_()'
//  and then just truncate it to the actual data size at the end.
//
// For data.frames, growing its size involves growing the size of every 
// column individually. For int/double, this is an easy re-allocation.
// For string STRSXP you'd have to grow them in the same manner as 'grow_list_()'
// grows VECSXP objects.  This seems like a lot of work in order to figure
// out if its any faster than just traverseing the file twice in order to 
// count number of newlines.
//
// For now (2023-08-09), ndjson->list will use method (2) and 
// ndjson->data.frame will use method (10)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUF_SIZE 65536
int count_lines(const char *filename)
{
  char buf[BUF_SIZE];
  int counter = 0;
  
  gzFile file = gzopen(filename, "r");
  
  for(;;) {
    size_t res = gzfread(buf, 1, BUF_SIZE, file);
    
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
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_ndjson_file_as_list_(SEXP filename_, SEXP nread_, SEXP nskip_, SEXP parse_opts_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Buffer to read each line of the input file.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char buf[MAX_LINE_LENGTH];
  
  parse_options opt = create_parse_options(parse_opts_);
  
  unsigned int nread = asInteger(nread_);
  unsigned int nskip = asInteger(nskip_);
  
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
  SEXP list_ = PROTECT(allocVector(VECSXP, 64));
  R_len_t list_size = XLENGTH(list_);
  
  
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
  unsigned int i = 0;
  while (gzgets(input, buf, MAX_LINE_LENGTH) != 0) {
    
    if (i >= nread) {
      break;
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Grow list if we need more room
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (i >= list_size) {
      UNPROTECT(1);
      list_ = PROTECT(grow_list(list_));
      list_size = XLENGTH(list_);
    }
    
    // ignore lines which are just a "\n".
    // might have to do something fancier for lines with just whitespace
    if (strlen(buf) <= 1) continue;
    
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts(buf, strlen(buf), opt.yyjson_read_flag, NULL, &err);
    
    if (doc == NULL) {
      output_verbose_error(buf, err);
      warning("Couldn't parse NDJSON row %i. Inserting 'NULL'\n", i);
      SET_VECTOR_ELT(list_, i, R_NilValue);
    } else {
      SET_VECTOR_ELT(list_, i, parse_json_from_str(buf, &opt));
    }
    
    yyjson_doc_free(doc);
    
    i++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // In-situ faux truncation of a VECSXP object.
  // This just hides the trailing elements from R
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SETLENGTH(list_, i);
  SET_TRUELENGTH(list_, list_size);
  SET_GROWABLE_BIT(list_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Close input, tidy memory and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gzclose(input);
  UNPROTECT(1);
  return list_;
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
  
  unsigned int nprotect = 0;
  char buf[MAX_LINE_LENGTH];
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
  unsigned int nskip  = asInteger(nskip_);
  unsigned int nprobe = asInteger(nprobe_);
  
  if (nread < 0) {
    nread = INT32_MAX;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get the maximum possible number of json rows in this ndjson file.
  // Note: the actual number of rows to parse may be less than this due
  // to blank lines and/or errors.
  // 'nrows' controls the amount of memory pre-allocated for rows in the df.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int nrows = count_lines(filename);
  
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
  char *colname[MAX_DF_COLS];
  unsigned int type_bitset[MAX_DF_COLS] = {0};
  unsigned int sexp_type[MAX_DF_COLS] = {0};
  unsigned int ncols = 0;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Probe file for types
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gzFile input = gzopen(filename, "r");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (nskip > 0) {
    unsigned int nskip2 = nskip;
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
      error("Couldn't parse JSON during probe line %i\n", i);
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
        colname[ncols] = (char *)yyjson_get_str(key);
        ncols++;
        if (ncols == MAX_DF_COLS) {
          error("Maximum columns for data.frame exceeded: %i", MAX_DF_COLS);
        }
      }
      
      type_bitset[name_idx] = update_type_bitset(type_bitset[name_idx], val, &opt);
    }
    
  }
  
  gzclose(input);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a data.frame.
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
  // cna't parse.
  unsigned int row = 0;
  
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
      error("Couldn't parse JSON on line %i\n", i);
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
          if (opt.missing_list_elem) {
            SET_VECTOR_ELT(column_, row, ScalarLogical(INT32_MIN));
          } else {
            SET_VECTOR_ELT(column_, row, R_NilValue);
          }
        } else {
          SET_VECTOR_ELT(column_, row, json_as_robj(val, &opt));
        }
        break;
      default:
        error("parse_ndjson_file_as_df_(): Unknown type");
      } 
      
    }
    
    row++;
  }
  
  gzclose(input);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set colnames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(allocVector(STRSXP, ncols)); nprotect++;
  for (unsigned int i = 0; i < ncols; i++) {
    SET_STRING_ELT(nms_, i, mkChar(colname[i]));
  }
  Rf_setAttrib(df_, R_NamesSymbol, nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Resize each data.frame column vector to match the actual data length
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (nrows != row) {
    unsigned int allocated_length = nrows;
    unsigned int data_length = row;
    for (int i=0; i < length(df_); i++) {
      SETLENGTH(VECTOR_ELT(df_, i), data_length);
      SET_TRUELENGTH(VECTOR_ELT(df_, i), allocated_length);
      SET_GROWABLE_BIT(VECTOR_ELT(df_, i));
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set empty rownames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rownames = PROTECT(allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -row);
  setAttrib(df_, R_RowNamesSymbol, rownames);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 'data.frame' class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SET_CLASS(df_, mkString("data.frame"));
  
  UNPROTECT(nprotect);
  return df_;
}

