

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Connections.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <zlib.h>

#include "yyjson.h"
#include "R-yyjson-serialize.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP serialize_list_to_ndjson_file_(SEXP robj_, SEXP filename_, SEXP serialize_opts_) {
  
  serialize_options opt = parse_serialize_options(serialize_opts_);
  
  FILE *file = NULL;
  R_xlen_t nelems = xlength(robj_);
  
  const char *filename = CHAR(STRING_ELT(filename_, 0));
  file = fopen(filename, "w");
  if (file == NULL) {
    error("Couldn't open file: %s", filename);
  }
  
  for (R_xlen_t idx = 0; idx < nelems; idx++) {
    SEXP elem_ = VECTOR_ELT(robj_, idx);
    
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *val = serialize_core(elem_, doc, &opt);
    yyjson_mut_doc_set_root(doc, val);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Write to File Pointer
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    yyjson_write_err err;
    bool res = yyjson_mut_write_fp(file, doc, opt.yyjson_write_flag, NULL, &err);  
    if (!res) {
      error("Error writing to file at element %ld\n", (long)idx);
    }
    fputc('\n', file);
    
    yyjson_mut_doc_free(doc);
  }
  
  
  fclose(file);
  return R_NilValue;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP serialize_list_to_ndjson_str_(SEXP robj_, SEXP serialize_opts_) {
  serialize_options opt = parse_serialize_options(serialize_opts_);
  
  char **ndjson = NULL;
  R_xlen_t nelems = xlength(robj_);
  
  ndjson = (char **)calloc((unsigned long)nelems, sizeof(char *));      
  
  for (R_xlen_t idx = 0; idx < nelems; idx++) {
    SEXP elem_ = VECTOR_ELT(robj_, idx);
    
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *val = serialize_core(elem_, doc, &opt);
    yyjson_mut_doc_set_root(doc, val);
    ndjson[idx] = yyjson_mut_write(doc, opt.yyjson_write_flag, NULL);
    yyjson_mut_doc_free(doc);
  }
  
  
  // concatenate into single string for return to R
  unsigned int total_len = 1; // extra '1' for '\0' byte at end of string
  for (unsigned int idx = 0; idx < nelems; idx++) {
    total_len += (unsigned int)strlen(ndjson[idx]) + 1; // extra 1 for `\n' for each row.
    // Rprintf("Total length: %i\n", total_len);
  }
  char *total_str;
  total_str = (char *)calloc(total_len, sizeof(char));
  
  unsigned int pos = 0;
  for (unsigned int idx = 0; idx < nelems; idx++) {
    strcpy(total_str + pos, ndjson[idx]);
    pos += (unsigned int)strlen(ndjson[idx]);
    total_str[pos] = '\n';
    pos++;
  }
  total_str[total_len - 2] = '\0';
  
  SEXP ndjson_ = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(ndjson_, 0, mkChar(total_str));
  free(ndjson);
  UNPROTECT(1);
  return ndjson_;
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize list or data.frame to NDJSON
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP serialize_df_to_ndjson_file_(SEXP robj_, SEXP filename_, SEXP serialize_opts_) {
  
  serialize_options opt = parse_serialize_options(serialize_opts_);
  
  if (!inherits(robj_, "data.frame")) {
    error("serialize_ndjson_(): object must a list or data.frame");
  }
  
  // Get sexp_types of all columns
  // Get number of rows
  // for each row
  //   create document
  //   create {}-object
  //   for each col
  //      add value to object
  //   write string to file.  
  
  R_xlen_t ncols = xlength(robj_);
  R_xlen_t nrows = xlength(VECTOR_ELT(robj_, 0));
  SEXP nms_ = getAttrib(robj_, R_NamesSymbol);
  
  FILE *file = NULL;
  const char *filename = CHAR(STRING_ELT(filename_, 0));
  file = fopen(filename, "w");
  if (file == NULL) {
    error("Couldn't open file: %s", filename);
  }
  
  for (unsigned int row = 0; row < nrows; row++) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    for (unsigned int col = 0; col < ncols; col++) {
      const char *key_str = CHAR(STRING_ELT(nms_, col));
      yyjson_mut_val *key = yyjson_mut_str(doc, key_str);
      yyjson_mut_val *val;
      SEXP col_ = VECTOR_ELT(robj_, col);
      
      switch(TYPEOF(col_)) {
      case LGLSXP:
        val = scalar_logical_to_json_val(INTEGER(col_)[row], doc, &opt);
        break;
      case INTSXP:
        if (isFactor(col_)) {
          val = scalar_factor_to_json_val(col_, row, doc, &opt);
        } else if (inherits(col_, "Date")) {
          val = scalar_date_to_json_val(col_, row, doc, &opt);
        } else if (inherits(col_, "POSIXct")) {
          val = scalar_posixct_to_json_val(col_, row, doc, &opt);
        } else {
          val = scalar_integer_to_json_val(INTEGER(col_)[row], doc, &opt);
        }
        break;
      case REALSXP: {
        if (inherits(col_, "Date")) {
        val = scalar_date_to_json_val(col_, row, doc, &opt);
      } else if (inherits(col_, "POSIXct")) {
        val = scalar_posixct_to_json_val(col_, row, doc, &opt);
      } else if (inherits(col_, "integer64")) {
        val = scalar_integer64_to_json_val(col_, row, doc, &opt);
      } else {
        val = scalar_double_to_json_val(REAL(col_)[row], doc, &opt);
      }
      }
        break;
      case STRSXP: {
        val = scalar_strsxp_to_json_val(col_, row, doc, &opt);
      }
        break;
      case VECSXP:
        val = serialize_core(VECTOR_ELT(col_, row), doc, &opt);
        break;
      case RAWSXP:
        val = scalar_rawsxp_to_json_val(col_, row, doc, &opt);
        break;
      default:
        error("data_frame_to_json_array_of_objects(): Unhandled scalar SEXP: %s\n", type2char((SEXPTYPE)TYPEOF(col_)));
      }
      // Add value to row obj
      if (val != NULL) {
        yyjson_mut_obj_add(obj, key, val);
      }
    }
    yyjson_mut_doc_set_root(doc, obj);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Write to JSON string
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    yyjson_write_err err;
    bool res = yyjson_mut_write_fp(file, doc, opt.yyjson_write_flag, NULL, &err);  
    if (!res) {
      error("Error writing to file at row %i\n", row);
    }
    fputc('\n', file);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // tidy and return
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    yyjson_mut_doc_free(doc);
    
  }
  
  fclose(file);
  
  return R_NilValue;
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize list or data.frame to NDJSON
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP serialize_df_to_ndjson_str_(SEXP robj_, SEXP serialize_opts_) {
  
  serialize_options opt = parse_serialize_options(serialize_opts_);
  
  if (!inherits(robj_, "data.frame")) {
    error("serialize_ndjson_(): object must a list or data.frame");
  }
  
  // Get sexp_types of all columns
  // Get number of rows
  // for each row
  //   create document
  //   create {}-object
  //   for each col
  //      add value to object
  //   write string to file.  
  
  R_xlen_t ncols = xlength(robj_);
  R_xlen_t nrows = xlength(VECTOR_ELT(robj_, 0));
  SEXP nms_ = getAttrib(robj_, R_NamesSymbol);
  
  char **ndjson = NULL;
  ndjson = (char **)calloc((unsigned long)nrows, sizeof(char *));      
  
  for (R_xlen_t row = 0; row < nrows; row++) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    for (R_xlen_t col = 0; col < ncols; col++) {
      const char *key_str = CHAR(STRING_ELT(nms_, col));
      yyjson_mut_val *key = yyjson_mut_str(doc, key_str);
      yyjson_mut_val *val;
      SEXP col_ = VECTOR_ELT(robj_, col);
      
      switch(TYPEOF(col_)) {
      case LGLSXP:
        val = scalar_logical_to_json_val(INTEGER(col_)[row], doc, &opt);
        break;
      case INTSXP:
        if (isFactor(col_)) {
          val = scalar_factor_to_json_val(col_, row, doc, &opt);
        } else if (inherits(col_, "Date")) {
          val = scalar_date_to_json_val(col_, row, doc, &opt);
        } else if (inherits(col_, "POSIXct")) {
          val = scalar_posixct_to_json_val(col_, row, doc, &opt);
        } else {
          val = scalar_integer_to_json_val(INTEGER(col_)[row], doc, &opt);
        }
        break;
      case REALSXP: {
        if (inherits(col_, "Date")) {
        val = scalar_date_to_json_val(col_, row, doc, &opt);
      } else if (inherits(col_, "POSIXct")) {
        val = scalar_posixct_to_json_val(col_, row, doc, &opt);
      } else if (inherits(col_, "integer64")) {
        val = scalar_integer64_to_json_val(col_, row, doc, &opt);
      } else {
        val = scalar_double_to_json_val(REAL(col_)[row], doc, &opt);
      }
      }
        break;
      case STRSXP: {
        val = scalar_strsxp_to_json_val(col_, row, doc, &opt);
      }
        break;
      case VECSXP:
        val = serialize_core(VECTOR_ELT(col_, row), doc, &opt);
        break;
      case RAWSXP:
        val = scalar_rawsxp_to_json_val(col_, row, doc, &opt);
        break;
      default:
        error("data_frame_to_json_array_of_objects(): Unhandled scalar SEXP: %s\n", type2char((SEXPTYPE)TYPEOF(col_)));
      }
      // Add value to row obj
      if (val != NULL) {
        yyjson_mut_obj_add(obj, key, val);
      }
    }
    yyjson_mut_doc_set_root(doc, obj);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Write to JSON string
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ndjson[row] = yyjson_mut_write(doc, opt.yyjson_write_flag, NULL);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // tidy and return
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    yyjson_mut_doc_free(doc);
    
  }
  
  // concatenate into single string for return to R
  unsigned int total_len = 1; // extra '1' for '\0' byte at end of string
  for (unsigned int row = 0; row < nrows; row++) {
    total_len += (unsigned int)strlen(ndjson[row]) + 1; // extra 1 for `\n' for each row.
    // Rprintf("Total length: %i\n", total_len);
  }
  char *total_str;
  total_str = (char *)calloc(total_len, sizeof(char));
  
  unsigned int idx = 0;
  for (unsigned int row = 0; row < nrows; row++) {
    strcpy(total_str + idx, ndjson[row]);
    idx += (unsigned int)strlen(ndjson[row]);
    if (row == nrows - 1) {
      total_str[idx] = '\0';
    } else {
      total_str[idx] = '\n';
      idx++;
    }
  }
  
  SEXP ndjson_ = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(ndjson_, 0, mkChar(total_str));
  free(ndjson);
  UNPROTECT(1);
  return ndjson_;
}





