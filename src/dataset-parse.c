
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Connections.h>
#include <Rversion.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <zlib.h>

#include "yyjson.h"
#include "R-yyjson-state.h"
#include "R-yyjson-parse.h"

#include "utils.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Dataset NDJSON types
// See cdisc spec: 
//    https://github.com/cdisc-org/DataExchange-DatasetJson/blob/master/doc/dataset-json1-1.md
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dataType (logical) 	JSON data type 	targetDataType 	Comment
// -------------------  --------------  --------------  ------------
//              string 	        string 		
//             integer 	       integer 		
//             decimal 	        string 	      decimal 	decimal is exchanged as a 
//                                                      string and uses "." as the decimal separator
//               float        	number 		
//              double        	number 		
//             boolean        	boolean 		
//            datetime         	string 		              ISO 8601 datetime as a string
//                date 	        string 	              	ISO 8601 date as a string
//                time 	        string 		              ISO 8601 time as a string
//            datetime         	string 	      integer 	ISO 8601 datetime as an integer (use case: ADaM)
//                date        	string      	integer 	ISO 8601 date as an integer (use case: ADaM)
//                time 	        string 	      integer 	ISO 8601 time as an integer (use case: ADaM)
//                 URI 	        string 		

#define DS_STRING   0
#define DS_INTEGER  1
#define DS_DECIMAL  2
#define DS_FLOAT    3
#define DS_DOUBLE   4
#define DS_BOOLEAN  5
#define DS_DATETIME 6
#define DS_DATE     7
#define DS_TIME     8
#define DS_URI      9

#define NDTYPES 10

const char *ref_dtype_str[NDTYPES] = {
  "string",
  "integer",
  "decimal",
  "float",
  "double", 
  "boolean",
  "datetime", 
  "date", 
  "time", 
  "uri"
};  

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// colspec example
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//         itemOID    name                     label dataType length keySequence
// 1 IT.DM.STUDYID STUDYID          Study Identifier   string     12           1
// 2  IT.DM.DOMAIN  DOMAIN       Domain Abbreviation   string      2          NA
// 3 IT.DM.USUBJID USUBJID Unique Subject Identifier   string      8           2
// 4     IT.DM.AGE     AGE                       Age  integer     NA          NA
// 5    IT.DM.AGEU    AGEU                 Age Units   string      5          NA


#define SRC_STRING 0
#define SRC_RAW    1
#define SRC_FILE   2


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_dataset_ndjson_as_df_(SEXP src_, SEXP colspec_, SEXP nskip_, SEXP parse_opts_, SEXP input_type_) {
  
  int nprotect = 0;
  
  char buf[MAX_LINE_LENGTH] = {0};
  gzFile file;
  
  parse_options opt = create_parse_options(parse_opts_);
  opt.yyjson_read_flag |= YYJSON_READ_STOP_WHEN_DONE;
  int nskip  = Rf_asInteger(nskip_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check the input type
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int input_type = Rf_asInteger(input_type_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the colspec for some sanity
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
#if R_VERSION < R_Version(4, 5, 0)
  if (!Rf_isFrame(colspec_)) Rf_error("parse_dataset_ndjson_str_as_df_(): 'colspec' must be a data.frame");
#else
  if (!Rf_isDataFrame(colspec_)) Rf_error("parse_dataset_ndjson_str_as_df_(): 'colspec' must be a data.frame");
#endif
  int nm_idx = df_col_idx(colspec_, "name");
  if (nm_idx < 0) Rf_error("parse_dataset_ndjson_str_as_df_(): 'name' not found in colspec");
  int dt_idx = df_col_idx(colspec_, "dataType");
  if (dt_idx < 0) Rf_error("parse_dataset_ndjson_str_as_df_(): 'dataType' not found in colspec");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise input
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *str            = NULL;
  char *filename       = NULL;
  size_t str_size      = 0;      
  size_t orig_str_size = 0; 
  size_t total_read    = 0;    
  
  switch(input_type) {
  case SRC_STRING:
    str           = (char *)CHAR( STRING_ELT(src_, 0) );
    str_size      = strlen(str);
    orig_str_size = strlen(str);
    if (str_size == 0) {
      UNPROTECT(nprotect);
      return(R_NilValue);
    }
    break;
  case SRC_RAW:
    str           = (char *)RAW(src_);
    str_size      = (size_t)Rf_length(src_);
    orig_str_size = (size_t)Rf_length(src_);
    if (str_size == 0) {
      UNPROTECT(nprotect);
      return(R_NilValue);
    }
    break;
  case SRC_FILE:
    filename = (char *)CHAR(STRING_ELT(src_, 0));
    filename = (char *)R_ExpandFileName(filename);
    if (access(filename, R_OK) != 0) {
      Rf_error("Cannot read from file '%s'", filename);
    }
    file = gzopen(filename, "r");
    break;
  default:
    Rf_error("Impossible");
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t nrows = 0;
  
  if (input_type == SRC_STRING || input_type == SRC_RAW) {
    while (nskip > 0 && total_read < orig_str_size) {
      yyjson_read_err err;
      state_t *state = create_state();
      state->doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
      size_t pos = yyjson_doc_get_read_size(state->doc);
      destroy_state(state);
      
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Advance string 
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      total_read += pos + 1;
      str        += pos + 1;
      str_size   -= (pos + 1);
      
      nskip--;
    }
  } else {
    nrows = count_lines(filename);
    
    // Account for rows to be skipped
    if (nrows <= nskip) 
      nrows = 0;
    else 
      nrows -= nskip;
    
    if (nskip > 0) {
      while (gzgets(file, buf, MAX_LINE_LENGTH) != 0) {
        nskip--;
        if (nskip == 0) break;
      }
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many rows in string?  i.e. count the "\n"
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (input_type == SRC_STRING || input_type == SRC_RAW) {
    if (total_read < orig_str_size) {
      for (size_t sp = 0; sp < str_size; sp++) {
        if (str[sp] == '\n') {
          nrows++;
        }
      }
      if (str_size > 0 && str[str_size - 1] != '\n') {
        // String does not end in newline, so need to manually count the last row
        nrows++;
      }
    }
  }
  
  // Rprintf("nrows: %i\n", (int)nrows);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the datatypes in the colspec
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dtypes_ = VECTOR_ELT(colspec_, dt_idx);
  int colspec_nrows = (int)Rf_length(dtypes_);
  
  int *dtype   = malloc(colspec_nrows * sizeof(int));
  if (dtype == NULL) Rf_error("parse_dataset_ndjson_str_as_df_(): Failed to alloc 'dtypes'");
  
  for (int i = 0; i < colspec_nrows; i++) {
    const char *dtype_str = CHAR(STRING_ELT(dtypes_, i));
    
    bool found = false;
    for (int d = 0; d < NDTYPES; d++) {
      if (strcmp(dtype_str, ref_dtype_str[d]) == 0) {
        dtype[i] = d;
        found = true;
        break;
      }
    }
    
    if (!found) {
      free(dtype);
      Rf_error("parse_dataset_ndjson_str_as_df_(): Unknown dataType in colspec: '%s'", dtype_str);
    }
    
  }
  
  // Dump the datatype spec
  // for (int i = 0; i < colspec_nrows; i++) {
  //   Rprintf("%i %i %s\n", i, dtype[i], ref_dtype_str[dtype[i]]);
  // }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Prepare the data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP df_  = PROTECT(Rf_allocVector(VECSXP, colspec_nrows)); nprotect++;
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, colspec_nrows)); nprotect++;
  Rf_setAttrib(df_, R_NamesSymbol, nms_);
  
  SEXP colspec_nms_ = VECTOR_ELT(colspec_, nm_idx);
  
  for (int i = 0; i < colspec_nrows; i++) {
    SET_STRING_ELT(nms_, i, STRING_ELT(colspec_nms_, i));
    
    SEXP col_;
    switch (dtype[i]) {
    case DS_STRING:
    case DS_DECIMAL:
    case DS_DATETIME:
    case DS_DATE:
    case DS_TIME:
    case DS_URI:
      col_ = PROTECT(Rf_allocVector(STRSXP, nrows));
      break;
    case DS_INTEGER:
      col_ = PROTECT(Rf_allocVector(INTSXP, nrows));
      break;
    case DS_BOOLEAN:
      col_ = PROTECT(Rf_allocVector(LGLSXP, nrows));
      break;
    case DS_FLOAT:
    case DS_DOUBLE:
      col_ = PROTECT(Rf_allocVector(REALSXP, nrows));
      break;
    default:
      Rf_error("parse_dataset_ndjson_str_as_df_(): Unknown dtype when creating df");
    }
    
    
    SET_VECTOR_ELT(df_, i, col_);
    UNPROTECT(1);
  }
  
  // Set rownames on data.frame
  SEXP rownames = PROTECT(Rf_allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -(int)nrows);
  Rf_setAttrib(df_, R_RowNamesSymbol, rownames);
  
  // Set 'data.frame' class
  SET_CLASS(df_, Rf_mkString("data.frame"));
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse data row-by-row
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  state_t *state = create_state();
  for (int row = 0; row < nrows; row++) {
    yyjson_read_err err;
    
    if (input_type == SRC_FILE) {
      str = gzgets(file, buf, MAX_LINE_LENGTH);
      if (str == NULL) {
        Rf_error("Unexpected end to data at row: %i\n", row);
      }
      str_size = strlen(str);
    }
    
    state->doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
    size_t pos = yyjson_doc_get_read_size(state->doc);
    if (state->doc == NULL) {
      free(dtype);
      error_and_destroy_state(state, "Couldn't parse Dataset-NDJSON on line %i\n", row + 1);
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Each data row *MUST* be an array
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    yyjson_val *arr = yyjson_doc_get_root(state->doc);
    if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
      free(dtype);
      error_and_destroy_state(state, "Couldn't parse Dataset-NDJSON row %i - JSON type = '%s' not 'array'\n", 
                              row + 1, yyjson_get_type_desc(arr));
    }
    

    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check the number of items in the array matches expectations
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int nitems = yyjson_get_len(arr);
    if (nitems != colspec_nrows) {
      free(dtype);
      error_and_destroy_state(
        state, 
        "Dataset-NDJSON row %i - has %i elements. Expecting %i\n", 
        row + 1, nitems, colspec_nrows);
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Iterate over array for this row, assigning values into the
    // final data.frame
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int col = 0;
    
    yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
    yyjson_val *val;
    while ((val = yyjson_arr_iter_next(&iter))) {
      
      switch(dtype[col]) {
      case DS_STRING:
      case DS_DECIMAL:
      case DS_URI:
        SET_STRING_ELT(VECTOR_ELT(df_, col), row, Rf_mkChar(yyjson_get_str(val)));
        break;
      case DS_BOOLEAN:
        if (yyjson_is_null(val)) {
          LOGICAL(VECTOR_ELT(df_, col))[row] = NA_LOGICAL;
        } else {
          LOGICAL(VECTOR_ELT(df_, col))[row] = yyjson_get_bool(val);
        }
        break;
      case DS_INTEGER:
        if (yyjson_is_null(val)) {
          INTEGER(VECTOR_ELT(df_, col))[row] = NA_INTEGER;
        } else {
          INTEGER(VECTOR_ELT(df_, col))[row] = yyjson_get_int(val);
        }
        break;
      case DS_FLOAT:
      case DS_DOUBLE:
        if (yyjson_is_null(val)) {
          REAL(VECTOR_ELT(df_, col))[row] = NA_REAL;
        } else {
          REAL(VECTOR_ELT(df_, col))[row] = yyjson_get_real(val);
        }
        break;
      case DS_DATETIME:
      case DS_DATE:
      case DS_TIME:
      {
        // Always read these as strings. Let the user sort it out?
        SEXP chr_ = PROTECT(json_val_to_charsxp(val, &opt)); 
        SET_STRING_ELT(VECTOR_ELT(df_, col), row, chr_);
        UNPROTECT(1);
      }
        break;
      default:
        Rf_error("parse_dataset_ndjson_str_as_df_(): impossible %i", dtype[col]);
      }
      
      col++;
    }
    
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (input_type != SRC_FILE) {
      total_read +=  pos + 1;
      str        +=  pos + 1;
      str_size   -= (pos + 1);
    }
  }
  
  
  free(dtype);
  destroy_state(state);
  if (input_type == SRC_FILE) {
    gzclose(file);
  }
  UNPROTECT(nprotect);
  return df_;
}

