


#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "yyjson.h"
#include "R-yyjson-serialize.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Forward declaration
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *serialize_core(SEXP robj_, yyjson_mut_doc *doc, serialize_options *opt);


//===========================================================================
// Parse the options from R list into a C struct
//===========================================================================
serialize_options parse_serialize_options(SEXP serialize_opts_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Default options
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  serialize_options opt = {
    .data_frame        = DATAFRAME_BY_ROW,
    .factor            = FACTOR_AS_STR,
    .auto_unbox        = FALSE,
    .digits            = -1,
    .name_repair       = NAME_REPAIR_NONE,
    .num_specials      = NUM_SPECIALS_AS_NULL,
    .str_specials      = STR_SPECIALS_AS_NULL,
    .fast_numerics     = FALSE,
    .json_verbatim     = FALSE,
    .yyjson_write_flag = 0,
  };
  
  // Sanity check and get option names
  if (isNull(serialize_opts_) || length(serialize_opts_) == 0) {
    return opt;
  }
  
  if (!isNewList(serialize_opts_)) {
    error("'serialize_opts' must be a list");
  }
  
  SEXP nms_ = getAttrib(serialize_opts_, R_NamesSymbol);
  if (isNull(nms_)) {
    error("'serialize_opts' must be a named list");
  }
  
  // Iterate over R options to populate C options struct
  for (int i = 0; i < length(serialize_opts_); i++) {
    const char *opt_name = CHAR(STRING_ELT(nms_, i));
    SEXP val_ = VECTOR_ELT(serialize_opts_, i);
    
    if (strcmp(opt_name, "digits") == 0) {
      opt.digits = asInteger(val_);
    } else if (strcmp(opt_name, "dataframe") == 0) {
      const char *tmp = CHAR(STRING_ELT(val_, 0));
      opt.data_frame = strcmp(tmp, "rows") == 0 ? DATAFRAME_BY_ROW : DATAFRAME_BY_COL;
    } else if (strcmp(opt_name, "factor") == 0) {
      const char *tmp = CHAR(STRING_ELT(val_, 0));
      opt.factor = strcmp(tmp, "string") == 0 ? FACTOR_AS_STR : FACTOR_AS_INT;
    } else if (strcmp(opt_name, "pretty") == 0) {
      if (asLogical(val_)) {
        opt.yyjson_write_flag |= YYJSON_WRITE_PRETTY_TWO_SPACES;
      }
    } else if (strcmp(opt_name, "auto_unbox") == 0) {
      opt.auto_unbox = asLogical(val_);
    } else if (strcmp(opt_name, "name_repair") == 0) {
      const char *tmp = CHAR(STRING_ELT(val_, 0));
      opt.name_repair = strcmp(tmp, "none") == 0 ? NAME_REPAIR_NONE : NAME_REPAIR_MINIMAL;
    } else if (strcmp(opt_name, "yyjson_write_flag") == 0) {
      for (unsigned int idx = 0; idx < length(val_); idx++) {
        opt.yyjson_write_flag |= (unsigned int)INTEGER(val_)[idx];
      }
    } else if (strcmp(opt_name, "str_specials") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      opt.str_specials = strcmp(val, "string") == 0 ? STR_SPECIALS_AS_STRING : STR_SPECIALS_AS_NULL;
    } else if (strcmp(opt_name, "num_specials") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      opt.num_specials = strcmp(val, "string") == 0 ? NUM_SPECIALS_AS_STRING : NUM_SPECIALS_AS_NULL;
    } else if (strcmp(opt_name, "fast_numerics") == 0) {
      opt.fast_numerics = asLogical(val_);
    } else if (strcmp(opt_name, "json_verbatim") == 0) {
      opt.json_verbatim = asLogical(val_);
    } else {
      warning("Unknown option ignored: '%s'\n", opt_name);
    }
  }
  
  return opt;
}


//===========================================================================
//    ###                  ##                 
//   #   #                  #                 
//   #       ###    ###     #     ###   # ##  
//    ###   #   #      #    #        #  ##  # 
//       #  #       ####    #     ####  #     
//   #   #  #   #  #   #    #    #   #  #     
//    ###    ###    ####   ###    ####  # 
//===========================================================================


//===========================================================================
// Scalar LGLSXP to JSON value
//===========================================================================
yyjson_mut_val *scalar_logical_to_json_val(int32_t rlgl, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *val;
  
  if (rlgl == NA_INTEGER) {
    if (opt->num_specials == NUM_SPECIALS_AS_STRING) {
      val = yyjson_mut_str(doc, "NA");
    } else {  
      val = yyjson_mut_null(doc);
    }
  } else {
    val = yyjson_mut_bool(doc, rlgl);
  }
  
  return val;
}



//===========================================================================
// Scalar INTSXP to JSON value
//===========================================================================
yyjson_mut_val *scalar_integer_to_json_val(int32_t rint, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *val;
  
  if (rint == NA_INTEGER) {
    if (opt->num_specials == NUM_SPECIALS_AS_STRING) {
      val = yyjson_mut_str(doc, "NA");
    } else {
      val = yyjson_mut_null(doc);
    }
  } else {
    val = yyjson_mut_sint(doc, rint);
  }
  
  return val;
}

//===========================================================================
// Scalar bit64::integer64 (stored in REALSXP) to JSON value
//===========================================================================
yyjson_mut_val *scalar_integer64_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *val;
  
  int64_t tmp = ((int64_t *)REAL(vec_))[idx];
  if (tmp == INT64_MIN) {
    if (opt->num_specials == NUM_SPECIALS_AS_STRING) {
      val = yyjson_mut_str(doc, "NA");
    } else {
      val = yyjson_mut_null(doc);
    }
  } else {
    val = yyjson_mut_sint(doc, tmp);
  }
  
  return val;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *scalar_date_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt) {
  char buf[50];
  
  double ndays = 0;
  if (isReal(vec_)) {
    ndays = REAL(vec_)[idx];
    
    if (!R_FINITE(ndays)) {
      return yyjson_mut_null(doc);
    }
  } else if (isInteger(vec_)) {
    uint32_t ndays_int = (uint32_t)INTEGER(vec_)[idx];
    if (ndays_int == INT32_MIN) { // NA
      return yyjson_mut_null(doc);
    }
    ndays = ndays_int;
  } else {
    error("scalar_date_to_json_val(): Nope");
  }
  
  // Convert days-since-epoch to seconds-since-epoch, then handle like posixct
  time_t tt = (time_t)(ndays * 24.0 * 60.0 * 60.0);
  struct tm *st = gmtime(&tt);
  strftime(buf, 50, "%Y-%m-%d", st);
  yyjson_mut_val *val = yyjson_mut_strcpy(doc, buf);
  
  return val;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *scalar_posixct_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt) {
  char buf[50];
  
  double seconds = 0;
  
  if (isReal(vec_)) {
    seconds = REAL(vec_)[idx];
    
    if (!R_FINITE(seconds)) {
      return yyjson_mut_null(doc);
    }
  } else if (isInteger(vec_)) {
    uint32_t seconds_int = (uint32_t)INTEGER(vec_)[idx];
    if (seconds_int == INT32_MIN) { // NA
      return yyjson_mut_null(doc);
    }
    seconds = seconds_int;
  } else {
    error("scalar_posixct_to_json_val(): Nope");
  }
  
  time_t tt = (time_t)seconds;
  struct tm *st = gmtime(&tt);
  strftime(buf, 50, "%Y-%m-%d %H:%M:%S", st);
  yyjson_mut_val *val = yyjson_mut_strcpy(doc, buf);
  
  return val;
}


//===========================================================================
// Scalar RAWSXP to JSON value
//===========================================================================
yyjson_mut_val *scalar_rawsxp_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *val;
  
  val = yyjson_mut_uint(doc, RAW(vec_)[idx]);
  
  return val;
}



//===========================================================================
// Scalar Factor to JSON value
//===========================================================================
yyjson_mut_val *scalar_factor_to_json_val(SEXP factor_, R_xlen_t idx,  yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *val ;
  int32_t factor = INTEGER(factor_)[idx];
  
  if (opt->factor == FACTOR_AS_INT) {
    val =  scalar_integer_to_json_val(factor, doc, opt);
  } else if (factor == NA_INTEGER) {
    val = yyjson_mut_null(doc);
  } else {
    SEXP nms_ = getAttrib(factor_, R_LevelsSymbol);
    const char *nm = CHAR(STRING_ELT(nms_, factor - 1));
    val = yyjson_mut_strcpy(doc, nm);
  }
  
  return val;
}


// Powers of 10 for rounding calculation
static double fac[20] = {1, 10, 100, 1000, 10000, 1e+05, 1e+06, 1e+07, 1e+08, 
                         1e+09, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 
                         1e+16, 1e+17, 1e+18, 1e+19};

//===========================================================================
// Scalar double to JSON value
//===========================================================================
yyjson_mut_val *scalar_double_to_json_val(double rdbl, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *val;
  
  if (isnan(rdbl)) {
    if (ISNA(rdbl)) {
      if (opt->num_specials == NUM_SPECIALS_AS_STRING) {
        val = yyjson_mut_str(doc, "NA");
      } else {
        val = yyjson_mut_null(doc);
      }
    } else {
      if (opt->num_specials == NUM_SPECIALS_AS_STRING) {
        val = yyjson_mut_str(doc, "NaN");
      } else {
        val = yyjson_mut_null(doc);
      }
    }
  } else if ( R_FINITE(rdbl) ) {
    if (opt->digits < 0) {
      val = yyjson_mut_real(doc, rdbl);
    } else if (opt->digits == 0) {
      // round to integer
      val = yyjson_mut_int(doc, (int64_t)round(rdbl));
    } else {
      // round to decimal places
      val = yyjson_mut_real(doc, round(rdbl * fac[opt->digits])/fac[opt->digits]);
    }
  } else {
    // Infinite
    if (opt->num_specials == NUM_SPECIALS_AS_NULL) {
      val = yyjson_mut_null(doc);
    } else {
      if (rdbl < 0) {
        val = yyjson_mut_str(doc, "-Inf");
      } else {
        val = yyjson_mut_str(doc, "Inf");
      }
    }
  }
  
  return val;
}


//===========================================================================
// Scalar STRSRXP  to JSON value
//===========================================================================
yyjson_mut_val *scalar_strsxp_to_json_val(SEXP str_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *val;

  // if "json"-classed and json_verbatim, write raw json:
  if (Rf_inherits(str_, "json") && opt->json_verbatim) {
    val = yyjson_mut_rawcpy(doc, CHAR(STRING_ELT(str_, idx)));
    return val;
  }
  
  // otherwise write a json string:
  SEXP charsxp_ = STRING_ELT(str_, idx);
  if (charsxp_ == NA_STRING) {
    if (opt->str_specials == STR_SPECIALS_AS_STRING) {
      val = yyjson_mut_str(doc, "NA");  
    } else {  
      val = yyjson_mut_null(doc);
    }
  } else {
    val = yyjson_mut_strcpy(doc, CHAR(charsxp_));
  }
  return val;
}


//===========================================================================
//   #   #                 #                  
//   #   #                 #                  
//   #   #   ###    ###   ####    ###   # ##  
//    # #   #   #  #   #   #     #   #  ##  #
//    # #   #####  #       #     #   #  #     
//    # #   #      #   #   #  #  #   #  #     
//     #     ###    ###     ##    ###   #   
//===========================================================================


//===========================================================================
// Serialize LGLSXP to JSON []-array
//===========================================================================
yyjson_mut_val *vector_lglsxp_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  // Bool size in C is 1byte.  in R it is 4bytes, so can't do a 
  // fast conversion here.
  int *ptr = INTEGER(vec_);
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  for (int i = 0; i < length(vec_); i++) {
    yyjson_mut_arr_append(arr, scalar_logical_to_json_val(*ptr++, doc, opt));
  }
  
  return arr;
}


//===========================================================================
// Serialize factor to JSON []-array
//===========================================================================
yyjson_mut_val *vector_factor_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  for (unsigned int i = 0; i < length(vec_); i++) {
    yyjson_mut_arr_append(arr, scalar_factor_to_json_val(vec_, i, doc, opt));
  }
  
  return arr;
}


//===========================================================================
// Serialize RAWSXP to JSON []-array
//===========================================================================
yyjson_mut_val *vector_rawsxp_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  
  // Raw vectors can't have NA, so can use the fast method
  return yyjson_mut_arr_with_uint8(doc, RAW(vec_), (size_t)length(vec_));
}


//===========================================================================
// Serialize POSIXct to JSON []-array
//===========================================================================
yyjson_mut_val *vector_posixct_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  for (unsigned int i = 0; i < length(vec_); i++) {
    yyjson_mut_arr_append(arr, scalar_posixct_to_json_val(vec_, i, doc, opt));
  }
  
  return arr;
}


//===========================================================================
// Serialize Date stored in REALSXP or INTSXP to JSON []-array
//===========================================================================
yyjson_mut_val *vector_date_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  for (unsigned int i = 0; i < length(vec_); i++) {
    yyjson_mut_arr_append(arr, scalar_date_to_json_val(vec_, i, doc, opt));
  }
  
  return arr;
}


//===========================================================================
// Serialize integer64 stored in REALSXP to JSON []-array
//===========================================================================
yyjson_mut_val *vector_integer64_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  for (unsigned int i = 0; i < length(vec_); i++) {
    yyjson_mut_arr_append(arr, scalar_integer64_to_json_val(vec_, i, doc, opt));
  }
  
  return arr;
}


//===========================================================================
// Serialize INTSXP to JSON []-array
//===========================================================================
yyjson_mut_val *vector_intsxp_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  if (inherits(vec_, "Date")) {
    return vector_date_to_json_array(vec_, doc, opt);
  } else if (inherits(vec_, "POSIXct")) {
    return vector_posixct_to_json_array(vec_, doc, opt);
  } else if (opt->fast_numerics) {
    return yyjson_mut_arr_with_sint32(doc, INTEGER(vec_), (size_t)length(vec_));
  } else {
    
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    
    int32_t *ptr = INTEGER(vec_);
    for (int i = 0; i < length(vec_); i++) {
      yyjson_mut_arr_append(arr, scalar_integer_to_json_val(*ptr++, doc, opt));
    }
    
    return arr;
  }
}


//===========================================================================
// Serialize REALSXP to JSON []-array
//===========================================================================
yyjson_mut_val *vector_realsxp_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  
  if (inherits(vec_, "Date")) {
    return vector_date_to_json_array(vec_, doc, opt);
  } else if (inherits(vec_, "POSIXct")) {
    return vector_posixct_to_json_array(vec_, doc, opt);
  } else if (inherits(vec_, "integer64")) {
    return vector_integer64_to_json_array(vec_, doc, opt);
  } else if (opt->fast_numerics) {
    return yyjson_mut_arr_with_double(doc, REAL(vec_), (size_t)length(vec_));
  } else {
    
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    
    double *ptr = REAL(vec_);
    for (int i = 0; i < length(vec_); i++) {
      yyjson_mut_arr_append(arr, scalar_double_to_json_val(*ptr++, doc, opt));
    }
    
    return arr;
  }
}


//===========================================================================
// Serialize STRSXP to JSON []-array
//===========================================================================
yyjson_mut_val *vector_strsxp_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  for (unsigned int i = 0; i < length(vec_); i++) {
    yyjson_mut_arr_append(arr, scalar_strsxp_to_json_val(vec_, i, doc, opt));
  }
  
  return arr;
}


//===========================================================================
// Serialize an R atomic vector into a JSON []-array
//
// Note we cannot do any "bulk import" of an R vector 
//      e.g. yyjson_mut_arr_with_sint().  This is because we want to explicitly
//      handle any special values e.g. NA, NaN, Inf 
//===========================================================================
yyjson_mut_val *vector_to_json_array(SEXP vec_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  yyjson_mut_val *arr;
  
  switch(TYPEOF(vec_)) {
  case LGLSXP:
    arr = vector_lglsxp_to_json_array(vec_, doc, opt);
    break;
  case INTSXP:
    if (isFactor(vec_)) {
      arr = vector_factor_to_json_array(vec_, doc, opt);
    } else {
      arr = vector_intsxp_to_json_array(vec_, doc, opt);
    }
    break;
  case REALSXP:
    arr = vector_realsxp_to_json_array(vec_, doc, opt);
    break;
  case STRSXP:
    arr = vector_strsxp_to_json_array(vec_, doc, opt);
    break;
  case RAWSXP:
    arr = vector_rawsxp_to_json_array(vec_, doc, opt);
    break;
  default:
    error("serialize_array(): Unknown array type: %s", type2char((SEXPTYPE)TYPEOF(vec_)));
  }
  
  return arr;
}



//===========================================================================
//   #   #          #              #          
//   #   #          #                         
//   ## ##   ###   ####   # ##    ##    #   # 
//   # # #      #   #     ##  #    #     # #  
//   #   #   ####   #     #        #      #   
//   #   #  #   #   #  #  #        #     # #  
//   #   #   ####    ##   #       ###   #   # 
//
// Write a matrix as an array of arrays in column-major order
//===========================================================================
yyjson_mut_val *matrix_to_col_major_array(SEXP mat_, unsigned int offset, yyjson_mut_doc *doc, serialize_options *opt) {
  
  SEXP dims_ = getAttrib(mat_, R_DimSymbol);
  unsigned int nrow = (unsigned int)INTEGER(dims_)[0];
  unsigned int ncol = (unsigned int)INTEGER(dims_)[1];
  
  // 'Offset' is used for array processing where we want to start processing
  // elements at an index other than 0. i.e. the n-th layer of the 3-d array
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate an array within the document
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  switch(TYPEOF(mat_)) {
  case LGLSXP: {
    int32_t *ptr = INTEGER(mat_) + offset;
    for (unsigned int row = 0; row < nrow; row++) {
      yyjson_mut_val *row_arr = yyjson_mut_arr(doc);
      for (unsigned int col = 0; col < ncol; col++) {
        yyjson_mut_arr_append(row_arr, scalar_logical_to_json_val(ptr[row + col * nrow], doc, opt));
      }
      yyjson_mut_arr_append(arr, row_arr);
    }
  }
    break;
  case INTSXP: {
    int32_t *ptr = INTEGER(mat_) + offset;
    for (unsigned int row = 0; row < nrow; row++) {
      yyjson_mut_val *row_arr = yyjson_mut_arr(doc);
      for (unsigned int col = 0; col < ncol; col++) {
        yyjson_mut_arr_append(row_arr, scalar_integer_to_json_val(ptr[row + col * nrow], doc, opt));
      }
      yyjson_mut_arr_append(arr, row_arr);
    }
  }
    break;
  case REALSXP: {
    double *ptr = REAL(mat_) + offset;
    for (unsigned int row = 0; row < nrow; row++) {
      yyjson_mut_val *row_arr = yyjson_mut_arr(doc);
      for (unsigned int col = 0; col < ncol; col++) {
        yyjson_mut_arr_append(row_arr, scalar_double_to_json_val(ptr[row + col * nrow], doc, opt));
      }
      yyjson_mut_arr_append(arr, row_arr);
    }
  }
    break;
  case STRSXP: {
    for (unsigned int row = 0; row < nrow; row++) {
    yyjson_mut_val *row_arr = yyjson_mut_arr(doc);
    for (unsigned int col = 0; col < ncol; col++) {
      yyjson_mut_arr_append(row_arr, scalar_strsxp_to_json_val(mat_, offset + row + col * nrow, doc, opt));
    }
    yyjson_mut_arr_append(arr, row_arr);
  }
  }
    break;
  default:
    error("matrix_to_col_major_array(). Unhandled type: %s", type2char((SEXPTYPE)TYPEOF(mat_)));
  }
  
  
  return arr;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 3d array
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *dim3_matrix_to_col_major_array(SEXP mat_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  SEXP dims_ = getAttrib(mat_, R_DimSymbol);
  unsigned int nrow   = (unsigned int)INTEGER(dims_)[0];
  unsigned int ncol   = (unsigned int)INTEGER(dims_)[1];
  unsigned int nlayer = (unsigned int)INTEGER(dims_)[2];
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate an array within the document
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  for (unsigned int layer = 0; layer < nlayer; layer++) {
    unsigned int offset = layer * (nrow * ncol);
    yyjson_mut_val *inner = matrix_to_col_major_array(mat_, offset, doc, opt);
    yyjson_mut_arr_append(arr, inner);
  }
  
  return arr;
}








//===========================================================================
//  #####               
//  #                   
//  #      # ##   #   # 
//  ####   ##  #  #   # 
//  #      #   #   # #  
//  #      #   #   # #  
//  #####  #   #    #   
//===========================================================================
yyjson_mut_val *env_to_json_object(SEXP env_, yyjson_mut_doc *doc, serialize_options *opt) {
  if (!isEnvironment(env_)) {
    error("env_to_json_object(): Expected environment. got %s", type2char((SEXPTYPE)TYPEOF(env_)));
  }
  
  int nprotect = 0;
  
  yyjson_mut_val *obj = yyjson_mut_obj(doc);
  
  // List of variables in an environment
  SEXP nms_ = PROTECT(R_lsInternal(env_, TRUE)); nprotect++;
  
  for (int i = 0; i < length(nms_); i++) {
    const char *varname = CHAR(STRING_ELT(nms_, i));
    SEXP elem_ = PROTECT(Rf_findVarInFrame(env_, installChar(mkChar(varname))));
    if (elem_ != R_UnboundValue) {  
      yyjson_mut_val *key = yyjson_mut_strcpy(doc, varname);
      yyjson_mut_val *val = serialize_core(elem_, doc, opt);
      yyjson_mut_obj_add(obj, key, val);
    }
    UNPROTECT(1);
  }
  
  UNPROTECT(nprotect);
  return obj;
}



//===========================================================================
//    #   #                                         #  #        #            #    
//    #   #                                         #  #                     #    
//    #   #  # ##   # ##    ###   ## #    ###    ## #  #       ##     ###   ####  
//    #   #  ##  #  ##  #      #  # # #  #   #  #  ##  #        #    #       #    
//    #   #  #   #  #   #   ####  # # #  #####  #   #  #        #     ###    #    
//    #   #  #   #  #   #  #   #  # # #  #      #  ##  #        #        #   #  # 
//     ###   #   #  #   #   ####  #   #   ###    ## #  #####   ###   ####     ## 
//
// Serialize an unnamed list to a JSON []-array
//===========================================================================
yyjson_mut_val *unnamed_list_to_json_array(SEXP list_, yyjson_mut_doc *doc, serialize_options *opt) {
  if (!isNewList(list_)) {
    error("unnamed_list_to_json_array(): Expected list. got %s", type2char((SEXPTYPE)TYPEOF(list_)));
  }
  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  for (int i = 0; i < length(list_); i++) {
    SEXP elem_ = VECTOR_ELT(list_, i);
    yyjson_mut_val *val = serialize_core(elem_, doc, opt);
    yyjson_mut_arr_append(arr, val);
  }
  
  return arr;
}


//===========================================================================
//   #   #                           #  #        #            #    
//   #   #                           #  #                     #    
//   ##  #   ###   ## #    ###    ## #  #       ##     ###   ####  
//   # # #      #  # # #  #   #  #  ##  #        #    #       #    
//   #  ##   ####  # # #  #####  #   #  #        #     ###    #    
//   #   #  #   #  # # #  #      #  ##  #        #        #   #  # 
//   #   #   ####  #   #   ###    ## #  #####   ###   ####     ##  
//
// Serialize a named list to a JSON {}-object
//===========================================================================
yyjson_mut_val *named_list_to_json_object(SEXP list_, yyjson_mut_doc *doc, serialize_options *opt) {
  if (!isNewList(list_)) {
    error("named_list_to_json_object(): Expected list. got %s", type2char((SEXPTYPE)TYPEOF(list_)));
  }
  
  yyjson_mut_val *obj = yyjson_mut_obj(doc);
  
  SEXP nms_ = PROTECT(getAttrib(list_, R_NamesSymbol));
  
  for (int i = 0; i < length(list_); i++) {
    SEXP elem_ = VECTOR_ELT(list_, i);
    yyjson_mut_val *key;
    char *str = (char *)CHAR(STRING_ELT(nms_, i));
    if ((opt->name_repair == NAME_REPAIR_MINIMAL) && (strlen(str) == 0)) {
      // Use the index of the item in the list as its name
      char buf[20];
      snprintf(buf, 20, "%i", i + 1);
      key = yyjson_mut_strcpy(doc, buf);
    } else {
      key = yyjson_mut_strcpy(doc, str);
    }
    yyjson_mut_val *val = serialize_core(elem_, doc, opt);
    yyjson_mut_obj_add(obj, key, val);
  }
  
  UNPROTECT(1);
  return obj;
}


//===========================================================================
// TODO: Expand names check to search for missing or blank names
//   Are duplicate key names in a JSON Object valid?
//===========================================================================
int is_named_list(SEXP list_) {
  SEXP nms_ = getAttrib(list_, R_NamesSymbol);
  return (TYPEOF(list_) == VECSXP) && !isNull(nms_);
}


//===========================================================================
//       #          #                   ##                              
//       #          #                  #  #                             
//    ## #   ###   ####    ###         #     # ##    ###   ## #    ###  
//   #  ##      #   #         #       ####   ##  #      #  # # #  #   # 
//   #   #   ####   #      ####        #     #       ####  # # #  ##### 
//   #  ##  #   #   #  #  #   #        #     #      #   #  # # #  #     
//    ## #   ####    ##    ####   #    #     #       ####  #   #   ###  
//
// Serialize data.frame by rows to a JSON []-array of {}-objects
//
// Create an array
// get names
// for each row in data.frame
//    create json obj
//    for each col in data.frame
//      add key/value to obj
//    add obj to array
//===========================================================================
unsigned int *detect_data_frame_types(SEXP df_, serialize_options *opt) {
  
  unsigned int ncols = (unsigned int)length(df_);
  unsigned int *col_type;
  
  col_type = (unsigned int *)malloc(ncols * sizeof(unsigned int));
  if (col_type == NULL) {
    error("Couldn't allocate in detect_data_frame_types()");
  }
  
  for (int col = 0; col < ncols; col++) {
    SEXP col_ = VECTOR_ELT(df_, col);
    
    switch(TYPEOF(col_)) {
    case LGLSXP:
      col_type[col] = LGLSXP;
      break;
    case INTSXP:
      if (isFactor(col_)) {
        col_type[col] = INTSXP_FACTOR;
      } else if (inherits(col_, "Date")) {
        col_type[col] = INTSXP_DATE;
      } else if (inherits(col_, "POSIXct")) {
        col_type[col] = INTSXP_POSIXCT;
      } else {
        col_type[col] = INTSXP;
      }
      break;
    case REALSXP: {
      if (inherits(col_, "Date")) {
      col_type[col] = REALSXP_DATE;
    } else if (inherits(col_, "POSIXct")) {
      col_type[col] = REALSXP_POSIXCT;
    } else if (inherits(col_, "integer64")) {
      col_type[col] = REALSXP_INT64;
    } else {
      col_type[col] = REALSXP;
    }
    }
      break;
    case STRSXP: {
      col_type[col] = STRSXP;
    }
      break;
    case VECSXP: {
      if (inherits(col_, "data.frame")) {
      col_type[col] = VECSXP_DF;
    } else {
      col_type[col] = VECSXP;
    }
    }
      break;
    case RAWSXP:
      col_type[col] = RAWSXP;
      break;
    default:
      error("detect_data_frame_types(): Unhandled scalar SEXP: %s\n", type2char((SEXPTYPE)TYPEOF(col_)));
    }
  }
  
  return col_type;
}



yyjson_mut_val *data_frame_row_to_json_object(SEXP df_, unsigned int *col_type, unsigned int row, int skip_col, yyjson_mut_doc *doc, serialize_options *opt) {
  
  // get data.frame names
  SEXP nms_ = PROTECT(getAttrib(df_, R_NamesSymbol));
  unsigned int ncols = (unsigned int)length(df_);
  
  yyjson_mut_val *obj = yyjson_mut_obj(doc);
  
  for (int col = 0; col < ncols; col++) {
    if (col == skip_col) continue;
    const char *key_str = CHAR(STRING_ELT(nms_, col));
    yyjson_mut_val *key = yyjson_mut_str(doc, key_str);
    yyjson_mut_val *val;
    SEXP col_ = VECTOR_ELT(df_, col);
    
    switch(col_type[col]) {
    case LGLSXP:
      val = scalar_logical_to_json_val(INTEGER(col_)[row], doc, opt);
      break;
    case INTSXP:
      val = scalar_integer_to_json_val(INTEGER(col_)[row], doc, opt);
      break;
    case INTSXP_FACTOR:
      val = scalar_factor_to_json_val(col_, row, doc, opt);
      break;
    case INTSXP_DATE:
      val = scalar_date_to_json_val(col_, row, doc, opt);
      break;
    case INTSXP_POSIXCT:
      val = scalar_posixct_to_json_val(col_, row, doc, opt);
      break;
    case REALSXP: 
      val = scalar_double_to_json_val(REAL(col_)[row], doc, opt);
      break;
    case REALSXP_DATE:
      val = scalar_date_to_json_val(col_, row, doc, opt);
      break;
    case REALSXP_POSIXCT:
      val = scalar_posixct_to_json_val(col_, row, doc, opt);
      break;
    case REALSXP_INT64:
      val = scalar_integer64_to_json_val(col_, row, doc, opt);
      break;
    case STRSXP: 
      val = scalar_strsxp_to_json_val(col_, row, doc, opt);
      break;
    case VECSXP: 
      val = serialize_core(VECTOR_ELT(col_, row), doc, opt);
      break;
    case VECSXP_DF:
      val = data_frame_row_to_json_object(col_, col_type, row, -1, doc, opt);
      break;
    case RAWSXP:
      val = scalar_rawsxp_to_json_val(col_, row, doc, opt);
      break;
    default:
      error("data_frame_row_to_json_object(): Unhandled scalar SEXP/col_type: %s [%i]\n", type2char((SEXPTYPE)TYPEOF(col_)), col_type[col]);
    }
    // Add value to row obj
    if (val != NULL) {
      yyjson_mut_obj_add(obj, key, val);
    }
  }
  
  UNPROTECT(1);
  return obj;
}


//===========================================================================
yyjson_mut_val *data_frame_row_to_json_array(SEXP df_, unsigned int *col_type, unsigned int row, int skip_col, yyjson_mut_doc *doc, serialize_options *opt) {
  
  // get data.frame names
  unsigned int ncols = (unsigned int)length(df_);
  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  for (int col = 0; col < ncols; col++) {
    if (col == skip_col) continue;
    yyjson_mut_val *val;
    SEXP col_ = VECTOR_ELT(df_, col);
    
    switch(col_type[col]) {
    case LGLSXP:
      val = scalar_logical_to_json_val(INTEGER(col_)[row], doc, opt);
      break;
    case INTSXP:
      val = scalar_integer_to_json_val(INTEGER(col_)[row], doc, opt);
      break;
    case INTSXP_FACTOR:
      val = scalar_factor_to_json_val(col_, row, doc, opt);
      break;
    case INTSXP_DATE:
      val = scalar_date_to_json_val(col_, row, doc, opt);
      break;
    case INTSXP_POSIXCT:
      val = scalar_posixct_to_json_val(col_, row, doc, opt);
      break;
    case REALSXP: 
      val = scalar_double_to_json_val(REAL(col_)[row], doc, opt);
      break;
    case REALSXP_DATE:
      val = scalar_date_to_json_val(col_, row, doc, opt);
      break;
    case REALSXP_POSIXCT:
      val = scalar_posixct_to_json_val(col_, row, doc, opt);
      break;
    case REALSXP_INT64:
      val = scalar_integer64_to_json_val(col_, row, doc, opt);
      break;
    case STRSXP: 
      val = scalar_strsxp_to_json_val(col_, row, doc, opt);
      break;
    case VECSXP: 
      val = serialize_core(VECTOR_ELT(col_, row), doc, opt);
      break;
    case VECSXP_DF:
      val = data_frame_row_to_json_object(col_, col_type, row, -1, doc, opt);
      break;
    case RAWSXP:
      val = scalar_rawsxp_to_json_val(col_, row, doc, opt);
      break;
    default:
      error("data_frame_row_to_json_object(): Unhandled scalar SEXP/col_type: %s [%i]\n", type2char((SEXPTYPE)TYPEOF(col_)), col_type[col]);
    }
    // Add value to row obj
    if (val != NULL) {
      yyjson_mut_arr_add_val(arr, val);
    }
  }
  
  return arr;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Special case for an unnamed-data.frame conversion to an 
// array of arrays
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *data_frame_to_json_array_of_arrays(SEXP df_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!Rf_inherits(df_, "data.frame")) {
    error("data_frame_to_json_array_of_arrays(). Not a data.frame!! %s", type2char((SEXPTYPE)TYPEOF(df_)));
  }
  
  
  // Create an array  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  // get size of data.frame
  unsigned int nrows = (unsigned int)length(VECTOR_ELT(df_, 0)); // length of first column
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For each row
  //   create an array
  //   for each column
  //      add the value to the array
  //   add the array to the overall array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int *col_type = detect_data_frame_types(df_, opt);
  for (unsigned int row = 0; row < nrows; row++) {
    
    yyjson_mut_val *obj = data_frame_row_to_json_array(df_, col_type, row, -1, doc, opt);
    
    // Add row obj to array
    yyjson_mut_arr_append(arr, obj);
  }
  
  free(col_type);
  
  // Return the array of row objects
  return arr;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *data_frame_to_json_array_of_objects(SEXP df_, yyjson_mut_doc *doc, serialize_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!Rf_inherits(df_, "data.frame")) {
    error("data_frame_to_json_array_of_objects(). Not a data.frame!! %s", type2char((SEXPTYPE)TYPEOF(df_)));
  }
  
  
  if (isNull(getAttrib(df_, R_NamesSymbol))) {
    return data_frame_to_json_array_of_arrays(df_, doc, opt);
  }
  
  // Create an array  
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  
  // get size of data.frame
  unsigned int nrows = (unsigned int)length(VECTOR_ELT(df_, 0)); // length of first column
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For each row
  //   create an object
  //   for each column
  //      add the value to the object
  //   add the object to the overall array
  //
  // TODO: It should be possible to cache the appropriate "scalar_x_to_json_val"
  //   function for each column, so the iteration over the rows doesn't
  //   have to do as much for each row. 
  //   Could add some functions (e.g. scalar_real_sxp_to_json_val()) such that
  //   all scalar serialization has the signature f(SEXP, idx, doc, opt)
  //   Need a solid benchmark before I start this.
  //   Mike 2023-08-13
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int *col_type = detect_data_frame_types(df_, opt);
  
  // for (int i = 0; i < length(df_); i++) {
  //   Rprintf("Col %i: %i\n", i, col_type[i]);
  // }
  
  for (unsigned int row = 0; row < nrows; row++) {
    
    yyjson_mut_val *obj = data_frame_row_to_json_object(df_, col_type, row, -1, doc, opt);
    
    // Add row obj to array
    yyjson_mut_arr_append(arr, obj);
  }
  
  free(col_type);
  
  // Return the array of row objects
  return arr;
}


//===========================================================================
//    ###                       
//   #   #                      
//   #       ###   # ##    ###  
//   #      #   #  ##  #  #   # 
//   #      #   #  #      ##### 
//   #   #  #   #  #      #     
//    ###    ###   #       ###  
//
// Recursive serialization
//===========================================================================
yyjson_mut_val *serialize_core(SEXP robj_, yyjson_mut_doc *doc, serialize_options *opt) {
  yyjson_mut_val *val;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Recursively serialize the R object into JSON
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_inherits(robj_, "data.frame")) {
    if (opt->data_frame == DATAFRAME_BY_ROW) {
      val = data_frame_to_json_array_of_objects(robj_, doc, opt);
    } else {
      val = named_list_to_json_object(robj_, doc, opt);
    }
  } else if (is_named_list(robj_)) {
    val = named_list_to_json_object(robj_, doc, opt);
  } else if (isNewList(robj_)) {
    val = unnamed_list_to_json_array(robj_, doc, opt);
  } else if (isEnvironment(robj_)) {
    val = env_to_json_object(robj_, doc, opt);
  } else if (isMatrix(robj_)) {
    val = matrix_to_col_major_array(robj_, 0, doc, opt);
  } else if (isArray(robj_)) {
    SEXP dims_ = getAttrib(robj_, R_DimSymbol);
    if (length(dims_) > 3) {
      error("multidimensional arrays with ndims > 3 not yet handled");
    }
    val = dim3_matrix_to_col_major_array(robj_, doc, opt);
  } else if (opt->auto_unbox && isVectorAtomic(robj_) && length(robj_) == 1) {
    if (inherits(robj_, "AsIs")) {
      val = vector_to_json_array(robj_, doc, opt);
    } else {
      switch(TYPEOF(robj_)) {
      case LGLSXP:
        val = scalar_logical_to_json_val(asLogical(robj_), doc, opt);
        break;
      case INTSXP:
        if (isFactor(robj_)) {
          val = scalar_factor_to_json_val(robj_, 0, doc, opt);
        } else if (inherits(robj_, "Date")) {
          val = scalar_date_to_json_val(robj_, 0, doc, opt);
        } else if (inherits(robj_, "POSIXct")) {
          val = scalar_posixct_to_json_val(robj_, 0, doc, opt);
        } else {
          val = scalar_integer_to_json_val(asInteger(robj_), doc, opt);
        }
        break;
      case REALSXP:
        if (inherits(robj_, "Date")) {
          val = scalar_date_to_json_val(robj_, 0, doc, opt);
        } else if (inherits(robj_, "POSIXct")) {
          val = scalar_posixct_to_json_val(robj_, 0, doc, opt);
        } else if (inherits(robj_, "integer64")) {
          val = scalar_integer64_to_json_val(robj_, 0, doc, opt);
        } else {
          val = scalar_double_to_json_val(asReal(robj_), doc, opt);
        }
        break;
      case STRSXP:
        val = scalar_strsxp_to_json_val(robj_, 0, doc, opt);
        break;
      case RAWSXP:
        val = scalar_rawsxp_to_json_val(robj_, 0, doc, opt);
        break;
      default:
        error("Unhandled scalar SEXP: %s\n", type2char((SEXPTYPE)TYPEOF(robj_)));
      }
    }
  } else if (isVectorAtomic(robj_)) {
    val = vector_to_json_array(robj_, doc, opt);
  } else if (isNull(robj_)) {
    val = yyjson_mut_null(doc);
  } else {
    warning("serialize_core(): Unhandled SEXP: %s\n", type2char((SEXPTYPE)TYPEOF(robj_)));
    val = yyjson_mut_null(doc);
  }
  
  return val;
}


//===========================================================================
//    #              ###   ###    ###   #   # 
//    #                #  #   #  #   #  #   # 
//   ####    ###       #  #      #   #  ##  # 
//    #     #   #      #   ###   #   #  # # # 
//    #     #   #      #      #  #   #  #  ## 
//    #  #  #   #  #   #  #   #  #   #  #   # 
//     ##    ###    ###    ###    ###   #   # 
//
// Serialize R object to JSON string.  Callable from R
//===========================================================================
SEXP serialize_to_str_(SEXP robj_, SEXP serialize_opts_) {
  
  
  serialize_options opt = parse_serialize_options(serialize_opts_);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a mutable document.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set the serialized object as the root node of the JSON document
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_val *obj = serialize_core(robj_, doc, &opt);
  yyjson_mut_doc_set_root(doc, obj);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write to JSON string
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_write_err err;
  char *json = yyjson_mut_write_opts(doc, opt.yyjson_write_flag, NULL, NULL, &err);
  if (json == NULL) {
    yyjson_mut_doc_free(doc);
    error("Write to string error: %s code: %u\n", err.msg, err.code);
  }
  
  SEXP res_ = PROTECT(mkString(json));
  free(json);
  yyjson_mut_doc_free(doc);
  UNPROTECT(1);
  return res_;
  
}



//===========================================================================
// Serialize data to file using 'yyjson_mut_write_file()'
//===========================================================================
SEXP serialize_to_file_(SEXP robj_, SEXP filename_, SEXP serialize_opts_) {
  
  serialize_options opt = parse_serialize_options(serialize_opts_);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a mutable document.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set the serialized object as the root node of the JSON document
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_val *obj = serialize_core(robj_, doc, &opt);
  yyjson_mut_doc_set_root(doc, obj);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write to JSON file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *filename = CHAR(STRING_ELT(filename_, 0));
  yyjson_write_err err;
  bool success = yyjson_mut_write_file(filename, doc, opt.yyjson_write_flag, NULL, &err);
  if (!success) {
    yyjson_mut_doc_free(doc);
    error("Write to file error '%s': %s code: %u\n", filename, err.msg, err.code);
  }
  yyjson_mut_doc_free(doc);
  return R_NilValue;
  
}








