
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
  int colspec_rows = Rf_nrows(colspec_);
  int nm_idx = df_col_idx(colspec_, "name");
  if (nm_idx < 0) Rf_error("parse_dataset_ndjson_str_as_df_(): 'name' not found in colspec");
  int dt_idx = df_col_idx(colspec_, "dataType");
  if (dt_idx < 0) Rf_error("parse_dataset_ndjson_str_as_df_(): 'dataType' not found in colspec");
  
  Rprintf("Colspec nrows %i, nm: %i, dataType: %i\n", colspec_rows, nm_idx, dt_idx);
  
  
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
  // Parse data row-by-row
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  state_t *state = create_state();
  for (int i = 0; i < nrows; i++) {
    yyjson_read_err err;
    state->doc = yyjson_read_opts(str, str_size, opt.yyjson_read_flag, NULL, &err);
    size_t pos = yyjson_doc_get_read_size(state->doc);
    if (state->doc == NULL) {
      error_and_destroy_state(state, "Couldn't parse Dataset-NDJSON on line %i\n", i + 1);
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Each data row *MUST* be an array
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    yyjson_val *row = yyjson_doc_get_root(state->doc);
    if (yyjson_get_type(row) != YYJSON_TYPE_ARR) {
      error_and_destroy_state(state, "Couldn't parse Dataset-NDJSON row %i - JSON type = '%s' not 'array'\n", 
                              i + 1, yyjson_get_type_desc(row));
    }
    
    Rprintf("[%i] %s\n", i, yyjson_get_type_desc(row));

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Advance string 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    total_read += pos + 1;
    str        += pos + 1;
    str_size   -= (pos + 1);
  }
  
  
  
  destroy_state(state);
  UNPROTECT(nprotect);
  return R_NilValue;
}

