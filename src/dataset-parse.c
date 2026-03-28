
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Connections.h>

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

SEXP parse_dataset_ndjson_str_as_df_(SEXP str_, SEXP colspec_, SEXP nskip_, SEXP parse_opts_) {
  
  int nprotect = 0;
  parse_options opt = create_parse_options(parse_opts_);
  opt.yyjson_read_flag |= YYJSON_READ_STOP_WHEN_DONE;
  
  int nskip  = Rf_asInteger(nskip_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the colspec for some sanity
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!Rf_isDataFrame(colspec_)) Rf_error("parse_dataset_ndjson_str_as_df_(): 'colspec' must be a data.frame");
  int nm_idx = df_col_idx(colspec_, "name");
  if (nm_idx < 0) Rf_error("parse_dataset_ndjson_str_as_df_(): 'name' not found in colspec");
  int dt_idx = df_col_idx(colspec_, "dataType");
  if (dt_idx < 0) Rf_error("parse_dataset_ndjson_str_as_df_(): 'dataType' not found in colspec");
  
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
  char *str;
  size_t str_size;      
  size_t orig_str_size; 
  size_t total_read = 0;    
  
  if (TYPEOF(str_) == RAWSXP) {
    str           = (char *)RAW(str_);
    str_size      = (size_t)Rf_length(str_);
    orig_str_size = (size_t)Rf_length(str_);
  } else {
    str           = (char *)CHAR( STRING_ELT(str_, 0) );
    str_size      = strlen(str);
    orig_str_size = strlen(str);
  }
  
  if (str_size == 0) {
    UNPROTECT(nprotect);
    return(R_NilValue);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Skip lines if requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
    str += pos + 1;
    str_size -= (pos + 1);
    
    nskip--;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many rows in string?  i.e. count the "\n"
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t nrows = 0;
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
  Rprintf("nrows of data: %i\n", (int)nrows);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the datatypes in the colspec
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dtypes_ = VECTOR_ELT(colspec_, dt_idx);
  int colspec_nrows = (int)Rf_length(dtypes_);
  
  int *dtype   = malloc(colspec_nrows * sizeof(int));
  if (dtype == NULL) Rf_error("parse_dataset_ndjson_str_as_df_(): Failed to alloc 'dtypes'");
  
  Rprintf("dtypes: %i %s\n", colspec_nrows, Rf_type2char(TYPEOF(dtypes_)));
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
  
  for (int i = 0; i < colspec_nrows; i++) {
    Rprintf("%i %i %s\n", i, dtype[i], ref_dtype_str[dtype[i]]);
  }
  
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
    
    Rprintf("[%i] %s\n", row, yyjson_get_type_desc(arr));

    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Parse items out of row
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int nitems = yyjson_get_len(arr);
    Rprintf("Row items: %i/%i\n", nitems, colspec_nrows);
    
    int col = 0;
    
    yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
    yyjson_val *val;
    while ((val = yyjson_arr_iter_next(&iter))) {
      Rprintf(">>> [%i] [%i] %i %s\n", row, col, dtype[col], yyjson_get_type_desc(val));
      
      switch(dtype[col]) {
      case DS_STRING:
        SET_STRING_ELT(VECTOR_ELT(df_, col), row, Rf_mkChar(yyjson_get_str(val)));
        break;
      case DS_INTEGER:
        INTEGER(VECTOR_ELT(df_, col))[row] = yyjson_get_int(val);
        break;
      }
      
      col++;
    }
    
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    total_read += pos + 1;
    str        += pos + 1;
    str_size   -= (pos + 1);
  }
  
  
  free(dtype);
  destroy_state(state);
  UNPROTECT(nprotect);
  return df_;
}

