


#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "yyjson.h"
#include "R-yyjson-parse.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Forward declarations
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP json_array_of_objects_to_data_frame(yyjson_val *arr, parse_options *opt);
SEXP json_as_robj(yyjson_val *val, parse_options *opt);


//===========================================================================
// Pare the R list of options into the 'parse_options' struct
//
// @param parse_opts_ An R named list of options. Passed in from the user.
//===========================================================================
parse_options create_parse_options(SEXP parse_opts_) {
  
  // Set default options
  parse_options opt = {
    .int64                 = INT64_AS_STR,
    .df_missing_list_elem  = R_NilValue,
    .obj_of_arrs_to_df     = true,
    .arr_of_objs_to_df     = true,
    .length1_array_asis    = false,
    .str_specials          = STR_SPECIALS_AS_STRING,
    .num_specials          = NUM_SPECIALS_AS_SPECIAL,
    .promote_num_to_string = false,
    .yyjson_read_flag      = 0
  };
  
  // Sanity check and extract option names from the named list
  if (isNull(parse_opts_) || length(parse_opts_) == 0) {
    return opt;
  }
  
  if (!isNewList(parse_opts_)) {
    error("'parse_opts' must be a list");
  }
  
  SEXP nms_ = getAttrib(parse_opts_, R_NamesSymbol);
  if (isNull(nms_)) {
    error("'parse_opts' must be a named list");
  }
  
  // Loop over options in R named list and assign to C struct
  for (int i = 0; i < length(parse_opts_); i++) {
    const char *opt_name = CHAR(STRING_ELT(nms_, i));
    SEXP val_ = VECTOR_ELT(parse_opts_, i);
    
    if (strcmp(opt_name, "length1_array_asis") == 0) {
      opt.length1_array_asis = asLogical(val_);
    } else if (strcmp(opt_name, "int64") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      if (strcmp(val, "double") == 0) {
        opt.int64 = INT64_AS_DBL;
      } else if (strcmp(val, "bit64") == 0) {
        opt.int64 = INT64_AS_BIT64;
      } else {
        opt.int64 = INT64_AS_STR;
      }
    } else if (strcmp(opt_name, "df_missing_list_elem") == 0) {
      opt.df_missing_list_elem = val_;
    } else if (strcmp(opt_name, "yyjson_read_flag") == 0) {
      for (unsigned int idx = 0; idx < length(val_); idx++) {
        opt.yyjson_read_flag |= (unsigned int)INTEGER(val_)[idx];
      }
    } else if (strcmp(opt_name, "obj_of_arrs_to_df") == 0) {
      opt.obj_of_arrs_to_df = asLogical(val_);
    } else if (strcmp(opt_name, "arr_of_objs_to_df") == 0) {
      opt.arr_of_objs_to_df = asLogical(val_);
    } else if (strcmp(opt_name, "str_specials") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      opt.str_specials = strcmp(val, "string") == 0 ? STR_SPECIALS_AS_STRING : STR_SPECIALS_AS_SPECIAL;
    } else if (strcmp(opt_name, "num_specials") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      opt.num_specials = strcmp(val, "string") == 0 ? NUM_SPECIALS_AS_STRING : NUM_SPECIALS_AS_SPECIAL;
    } else if (strcmp(opt_name, "promote_num_to_string") == 0) {
      opt.promote_num_to_string = asLogical(val_);
    } else {
      warning("Unknown option ignored: '%s'\n", opt_name);
    }
  }

  return opt;
}



//===========================================================================
//   ###                  ##                 
//  #   #                  #                 
//  #       ###    ###     #     ###   # ##  
//   ###   #   #      #    #        #  ##  # 
//      #  #       ####    #     ####  #     
//  #   #  #   #  #   #    #    #   #  #      
//   ###    ###    ####   ###    ####  #     
//
// The following functions parse a single yyjson JSON value to a 
// scalar R value
//===========================================================================


//===========================================================================
// Convert JSON value to logical 
// 
// @return value valid for inclusion in LGLSXP vector
//===========================================================================
int32_t json_val_to_logical(yyjson_val *val, parse_options *opt) {
  
  if (val == NULL) {
    return NA_LOGICAL;
  }
  
  switch (yyjson_get_type(val)) {
  case YYJSON_TYPE_BOOL:
    return yyjson_get_bool(val);
    break;
  case YYJSON_TYPE_NULL:
    return NA_LOGICAL;
    break;
  case YYJSON_TYPE_STR: {
    if (yyjson_equals_str(val, "NA")) {
    return NA_INTEGER;
  } else {
    warning("json_val_to_logical(): Unhandled string: %s", yyjson_get_str(val));
  }
  }
    break;
  default:
    // This shouldn't happen if the type checking done elsewhere is correct!
    warning("json_val_to_logical(). Unhandled type: %s\n", yyjson_get_type_desc(val));
  }
  
  return NA_LOGICAL;
}



//===========================================================================
// Convert JSON value to integer 
//
// @return value valid for inclusion in INTSXP vector
//===========================================================================
int32_t json_val_to_integer(yyjson_val *val, parse_options *opt) {
  
  if (val == NULL) {
    return NA_INTEGER;
  }
  
  switch (yyjson_get_type(val)) {
  case YYJSON_TYPE_NUM:
    switch (yyjson_get_subtype(val)) {
    case YYJSON_SUBTYPE_UINT:
      return (int32_t)yyjson_get_uint(val);
      break;
    case YYJSON_SUBTYPE_SINT:
      return (int32_t)yyjson_get_sint(val);
      break;
    default:
      warning("json_val_to_integer(). Unhandled numeric type: %i\n", yyjson_get_subtype(val));
    }
    break;
  case YYJSON_TYPE_NULL: 
    return NA_INTEGER;
    break;
  case YYJSON_TYPE_STR: {
    if (yyjson_equals_str(val, "NA")) {
      return NA_INTEGER;
    } else {
      // This shouldn't happen if the type checking done elsewhere is correct!
      warning("json_val_to_integer(): Unhandled string: %s", yyjson_get_str(val));
    }
  }
    break;
  default:
    warning("json_val_to_integer(). Unhandled type: %s\n", yyjson_get_type_desc(val));
  }
  
  return NA_INTEGER;
}


//===========================================================================
// Convert JSON value to DOUBLE 
//
// @return value valid for inclusion in REALSXP vector
//===========================================================================
double json_val_to_double(yyjson_val *val, parse_options *opt) {
  
  if (val == NULL) {
    return NA_REAL;
  }
  
  switch (yyjson_get_type(val)) {
  case YYJSON_TYPE_NUM:
    switch (yyjson_get_subtype(val)) {
    case YYJSON_SUBTYPE_UINT:
      return yyjson_get_uint(val);
      break;
    case YYJSON_SUBTYPE_SINT:
      return yyjson_get_sint(val);
      break;
    case YYJSON_SUBTYPE_REAL:
      return yyjson_get_real(val);
      break;
    default:
      warning("json_val_to_double(). Unhandled numeric type: %i\n", yyjson_get_subtype(val));
    }
    break;
  case YYJSON_TYPE_STR:
  {
    if (yyjson_equals_str(val, "NA")) {
      return NA_REAL;
    } else if (yyjson_equals_str(val, "-Inf")) {
      return -INFINITY;
    }  else if (yyjson_equals_str(val, "Inf"))  {
      return INFINITY;
    } else if (yyjson_equals_str(val, "NaN"))  {
      return R_NaN;
    }
  }
    break;
  case YYJSON_TYPE_NULL:
    return NA_REAL;
    break;
  default:
    // This shouldn't happen if the type checking done elsewhere is correct!
    warning("json_val_to_double(). Unhandled type: %s\n", yyjson_get_type_desc(val));
  }
  
  return NA_REAL;
}




//===========================================================================
// Convert JSON value to DOUBLE for Integet64 vector
//
// @return value valid for inclusion in REALSXP vector
//         This vector will be classed as bit64::integer64 object
//===========================================================================
long long json_val_to_integer64(yyjson_val *val, parse_options *opt) {
  
  if (val == NULL) {
    return INT64_MIN;
  }
  
  switch (yyjson_get_type(val)) {
  case YYJSON_TYPE_NUM:
    switch (yyjson_get_subtype(val)) {
    case YYJSON_SUBTYPE_UINT:
      return (long long)yyjson_get_uint(val);
      break;
    case YYJSON_SUBTYPE_SINT:
      return yyjson_get_sint(val);
      break;
    default:
      error("json_val_to_int64(). Unhandled numeric type: %i\n", yyjson_get_subtype(val));
    }
    break;
  case YYJSON_TYPE_STR:
  {
    if (yyjson_equals_str(val, "NA")) {
    return INT64_MIN;
  } else {
    error("json_val_to_int64(): Unahndled string value %s", yyjson_get_str(val));
  }
  break;
  }
  case YYJSON_TYPE_NULL:
    return INT64_MIN; // Equivalent to NA in integer64
    break;
  default:
    // This shouldn't happen if the type checking done elsewhere is correct!
    warning("json_val_to_integer64(). Unhandled type: %s\n", yyjson_get_type_desc(val));
  }
  
  return INT64_MIN; // Equivalent to NA in integer64
}


//===========================================================================
// Convert JSON value to CHARSXP
//
// @return value valid for inclusion in STRSXP vector
//===========================================================================
SEXP json_val_to_charsxp(yyjson_val *val, parse_options *opt) {

  if (val == NULL) {
    return NA_STRING;
  }
  
  char buf[128] = "";
  static char *bool_str[2] = {"FALSE", "TRUE"};
  
  switch (yyjson_get_type(val)) {
  case YYJSON_TYPE_NULL:
    return NA_STRING;
    break;
  case YYJSON_TYPE_BOOL:
  {
    int tmp = yyjson_get_bool(val);
    return mkChar(bool_str[tmp]);
  }
    break;
  case YYJSON_TYPE_NUM:
    switch(yyjson_get_subtype(val)) {
    case YYJSON_SUBTYPE_UINT:
#if defined(__APPLE__) || defined(_WIN32)
      snprintf(buf, 128, "%llu", yyjson_get_uint(val));
#else
      snprintf(buf, 128, "%lu", yyjson_get_uint(val));
#endif
      return mkChar(buf);
      break;
    case YYJSON_SUBTYPE_SINT:
#if defined(__APPLE__) || defined(_WIN32)
      snprintf(buf, 128, "%lld", yyjson_get_sint(val));
#else
      snprintf(buf, 128, "%ld", yyjson_get_sint(val));
#endif
      return mkChar(buf);
      break;
    case YYJSON_SUBTYPE_REAL:
      snprintf(buf, 128, "%f", yyjson_get_real(val));
      return mkChar(buf);
      break;
    default:
      warning("json_val_to_charsxp unhandled numeric type %s\n", yyjson_get_type_desc(val));
    }
    break;
  case YYJSON_TYPE_STR:
    if (opt->str_specials == STR_SPECIALS_AS_SPECIAL && yyjson_equals_str(val, "NA")) {
      return NA_STRING;
    } else {  
      return mkChar(yyjson_get_str(val));
      }
    break;
  default:
    // This shouldn't happen if the type checking done elsewhere is correct!
    warning("json_val_to_charsxp(). Unhandled type: %s\n", yyjson_get_type_desc(val));
  }
  
  return NA_STRING;
}


//===========================================================================
//  #####                          ####     #     #                    #    
//    #                             #  #          #                    #    
//    #    #   #  # ##    ###       #  #   ##    ####    ###    ###   ####  
//    #    #   #  ##  #  #   #      ###     #     #     #      #   #   #    
//    #    #  ##  ##  #  #####      #  #    #     #      ###   #####   #    
//    #     ## #  # ##   #          #  #    #     #  #      #  #       #  # 
//    #        #  #       ###      ####    ###     ##   ####    ###     ##  
//         #   #  #                                                         
//          ###   #        
//
// JSON []-arrays and {}-objects can hold any other JSON type.
// When trying to match a JSON []-array or {}-object to an R type, we first iterate
// over the JSON []-array or {}-object and use a bitset to keep track of
// which JSON types have been seen.
//
// In simple cases only a single bit in the bitset will be turned on - 
// which indicates that the []-array of {}-object only contains a single 
// type of value and is thus easily matched to an R vector.
//
// In more complex cases, the bitset has multiple bit sets indicating that
// there are multiple types within the container.  Then the bitset must
// be examined to determine what R container should be used.
// e.g. if a []-array contains integers and doubles we could either
//      (1) promote all the integers to doubles and return an R REALSXP
//      (2) use a list to store the values so that their original types are 
//          maintained.
//===========================================================================

//===========================================================================
// Helper function: dump out the contents of a `type_bitset` 
// Used only for debugging.
//===========================================================================
void dump_type_bitset(unsigned int type_bitset) {
  // #define VAL_NONE    1 << 0
  // #define VAL_RAW     1 << 1
  // #define VAL_NULL    1 << 2
  // #define VAL_BOOL    1 << 3
  // #define VAL_INT     1 << 4
  // #define VAL_REAL    1 << 5
  // #define VAL_STR     1 << 6
  // #define VAL_STR_INT 1 << 7 // Integer promoted to string
  // #define VAL_ARR     1 << 8
  // #define VAL_OBJ     1 << 9
  // #define VAL_INT64   1 << 10
  // VAL_NONE | VAL_RAW | VAL_NULL | VAL_BOOL | VAL_INT | VAL_REAL | VAL_STR | VAL_STR_INT | VAL_ARR | VAL_OBJ | VAL_INT64

  char *valname[11] = {"VAL_NONE", "VAL_RAW", "VAL_NULL", "VAL_BOOL",
                       "VAL_INT", "VAL_REAL", "VAL_STR", "VAL_STR_INT",
                       "VAL_ARR", "VAL_OBJ", "VAL_INT64"};
  for (int i = 0; i < 11; i++) {
    if (type_bitset & (1 << i)) {
      Rprintf(":: %s\n", valname[i]);
    }
  }
}


//===========================================================================
// Convert a bitset of all types (seen in a JSON []-array or {}-object) and
// give the best SEXP type which could contain that type.
//
// Getting this function fully correct will be an iterative process.
//
// If this function is correct, then we know the exact type needed to store
// the values in a JSON []-array or JSON {}-object. This means:
//   - we can reduce memory allocations
//   - we can use the "json_val_to_X()" functions knowing that the type of
//     value is able to be contained within that R atomic vector or VECSXP
//     (this means we don't have to switch types in the middle of a parsing 
//      operation - which would mean unnecessary allocation/copying)
//===========================================================================
unsigned int get_best_sexp_to_represent_type_bitset(unsigned int type_bitset, parse_options *opt) {
  
  unsigned int sexp_type = 0;
    
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Integer64 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  
  if (type_bitset & VAL_INT64) {
    if (! (type_bitset & (VAL_OBJ | VAL_ARR | VAL_STR | VAL_STR_INT | VAL_REAL)) ) {
      return INT64SXP;
    } else {
      return VECSXP;
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // String
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if ((type_bitset & VAL_STR) | (type_bitset & VAL_STR_INT)) {
    if ( opt->promote_num_to_string && (type_bitset & (VAL_REAL | VAL_INT | VAL_BOOL)) && !(type_bitset & (VAL_NONE | VAL_RAW | VAL_ARR | VAL_OBJ))) {
      return STRSXP;
    } else if (type_bitset & (VAL_NONE | VAL_RAW | VAL_BOOL | VAL_INT | VAL_REAL | VAL_ARR | VAL_OBJ | VAL_INT64)) {
      return VECSXP;
    } else {
      return STRSXP;
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // bitset contains ARRAY
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if ((type_bitset & (VAL_ARR | VAL_OBJ))) {
    return VECSXP;
  }
  
  
  if (type_bitset & VAL_BOOL) {
    if (type_bitset == VAL_BOOL) {
      sexp_type = LGLSXP;
    } else {
      // BOOL and anything else will be an R-list
      sexp_type = VECSXP;
    }
  } else if ((type_bitset & VAL_STR) | (type_bitset & VAL_STR_INT)) {
    sexp_type = STRSXP;
  } else if (type_bitset & VAL_REAL) {
    sexp_type = REALSXP;
  } else if (type_bitset & VAL_INT) {
    sexp_type = INTSXP;
  } else if (type_bitset & VAL_BOOL) {
    sexp_type = LGLSXP;
  } else if ((type_bitset & VAL_ARR) | (type_bitset & VAL_OBJ)) {
    sexp_type = VECSXP;
  } else if (type_bitset == 0) {
    sexp_type = VECSXP;
  } else {
    warning("get_best_sexp_to_represent_type_bitset(): unhandled type_bitset %i\n.", type_bitset);
    sexp_type = VECSXP;
  }
  
  return sexp_type;
}


//===========================================================================
// Update type bitset with a new json value
//===========================================================================
unsigned int update_type_bitset(unsigned int type_bitset, yyjson_val *val, parse_options *opt) {

  switch(yyjson_get_type(val)) {
  case YYJSON_TYPE_BOOL:
    type_bitset |= VAL_BOOL;
    break;
  case YYJSON_TYPE_NUM:
    switch (yyjson_get_subtype(val)) {
    case YYJSON_SUBTYPE_UINT: 
    {
      uint64_t tmp = yyjson_get_uint(val);
      if (tmp > INT32_MAX) {
        if (opt->int64 == INT64_AS_DBL) {
          type_bitset |= VAL_REAL;
        } else if (opt->int64 == INT64_AS_BIT64) {
          // Signed INT32_MAX
          // Signed INT64_MAX =  2^63-1 =  9223372036854775807
          // Signed INT64_MIN = -2^63   = -9223372036854775808
          if (tmp > INT64_MAX) {
            warning("64bit unsigned integer values exceed capacity of unsigned 64bit container (bit64::integer64). Expect overflow");
          }
          type_bitset |= VAL_INT64;
        } else {
          type_bitset |= VAL_STR_INT;
        }
      } else {
        type_bitset |= VAL_INT;
      }
    }
      break;
    case YYJSON_SUBTYPE_SINT:
    {
      int64_t tmp = yyjson_get_sint(val);
      if (tmp < INT32_MIN || tmp > INT32_MAX) {
        if (opt->int64 == INT64_AS_DBL) {
          type_bitset |= VAL_REAL;
        } else if (opt->int64 == INT64_AS_BIT64) {
          type_bitset |= VAL_INT64;
        } else {
          type_bitset |= VAL_STR_INT;
        }
      } else {
        type_bitset |= VAL_INT;
      }
    }
      break;
    case YYJSON_SUBTYPE_REAL:
      type_bitset |= VAL_REAL;
      break;
    default:
      error("get_array_element_type_bitset(): Unknown subtype in : %i\n", yyjson_get_subtype(val));
    }
    break;
  case YYJSON_TYPE_STR:
  {
    if (yyjson_equals_str(val, "NA")  || yyjson_equals_str(val, "NaN") || yyjson_equals_str(val, "Inf") || yyjson_equals_str(val, "-Inf")) {
      // don't adjust current type if there are any of these special strings
      if (opt->num_specials == NUM_SPECIALS_AS_STRING) {
        type_bitset |= VAL_STR;
      }
    } else {
      type_bitset |= VAL_STR;
    }
  }
    break;
  case YYJSON_TYPE_ARR:
    type_bitset |= VAL_ARR;
    break;
  case YYJSON_TYPE_OBJ:
    type_bitset |= VAL_OBJ;
    break;
  case YYJSON_TYPE_NULL:
    // Don't do anything with JSON 'null'
    break;
  default:
    error("get_array_element_type_bitset(); Unhandled type: %i -> %s\n", yyjson_get_type(val), 
          yyjson_get_type_desc(val));
  }
  
  return type_bitset;
}


//===========================================================================
// Find the best SEXP type to represent values in a non-nested array.
// Non-nested means that it is known a-priori that this array does 
// not contain any JSON []-arrays or {}-objects.
//
// Arrays which contain only arrays could be a matrix (otherwise a list)
// Arrays which contain only objects could be a data.frame (otherwise a list)
// Arrays which contain a mixture should be parsed as a list.
//
// Use 'init_type_bitset' to pass in an initial type.  Usually 0, but 
// if accumulating type from other arrays, pass in the result for the prior array
// to this function.  e.g. this would be done if trying to work out if an
// array-of-arrays was of a single consistent type.
//===========================================================================
unsigned int get_type_bitset_for_json_array(yyjson_val *arr, unsigned int init_type_bitset, parse_options *opt) {
  
  unsigned int type_bitset = init_type_bitset;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over the array, keep track of all the types encountered
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_val *val;
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  
  while ((val = yyjson_arr_iter_next(&iter))) {
    type_bitset = update_type_bitset(type_bitset, val, opt);
  }
  
  return type_bitset;
}


//===========================================================================
// Accumulate a bitset of all container types within the given array
//===========================================================================
unsigned int get_json_array_sub_container_types(yyjson_val *arr, parse_options *opt) {
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  yyjson_val *val;
  unsigned int ctn_bitset = 0;
  
  while ((val = yyjson_arr_iter_next(&iter))) {
    if (yyjson_is_obj(val)) {
      ctn_bitset |= CTN_OBJ;
    } else if (yyjson_is_arr(val)) {
      ctn_bitset |= CTN_ARR;
    } else {
      ctn_bitset |= CTN_NONE;
    }
  }
  
  return ctn_bitset;
}


//===========================================================================
// Could this non-nested array be parsed as a matrix?
//   * Prior to this call, this []-Array has already been shown 
//     to only contain other []-arrays.
//   * Now check in this function:
//     * all array lengths are the same
//     * all types can be contained in an atomic vector
//
// Accumulate a bitset of all container types within the given array
//
// Return:
//    0 if not a matrix
//    SEXP otherwise
//===========================================================================
unsigned int get_best_sexp_type_for_matrix(yyjson_val *arr, parse_options *opt) {
  
  // For each array, get the length
  // if all lengths match, then POSSIBLY an array.
  // if any lengths different then DEFINITELY NOT an array
  
  size_t first_len = yyjson_get_len( yyjson_arr_get_first(arr) );
  
  
  yyjson_val *val;
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check lengths
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while ((val = yyjson_arr_iter_next(&iter))) {
    size_t len = yyjson_get_len( val );
    if (len != first_len) {
      return 0;
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check that there are no sub-containers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  iter = yyjson_arr_iter_with( arr );
  while ((val = yyjson_arr_iter_next(&iter))) {
    // Currently only accepting 2-D matrices, without nested sub-arrays
    // So all elements of the top-level sub-array must not be containers
    unsigned int ctn_bitset = get_json_array_sub_container_types( val, opt );
    if (ctn_bitset != CTN_NONE) {
      return 0;
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check that type is INT, REAL, BOOL or STR only.  No VECSXP
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  iter = yyjson_arr_iter_with( arr );
  unsigned int accum_type_bitset = 0;
  while ((val = yyjson_arr_iter_next(&iter))) {
    accum_type_bitset = get_type_bitset_for_json_array(val, accum_type_bitset, opt);
  }
  
  unsigned int sexp_type = get_best_sexp_to_represent_type_bitset(accum_type_bitset, opt);
  if (sexp_type == VECSXP) {
    return 0;
  }
  
  return sexp_type;
}



//===========================================================================
//    #                               
//   # #                              
//  #   #  # ##   # ##    ###   #   # 
//  #   #  ##  #  ##  #      #  #   # 
//  #####  #      #       ####  #  ## 
//  #   #  #      #      #   #   ## # 
//  #   #  #      #       ####      # 
//                              #   # 
//                               ###  
//
// The following functions parse JSON []-arrays to 
// R lists or atomic vectors.
//===========================================================================

//===========================================================================
// Parse a JSON array known to only contain logical values
// Prerequisite: already know all types can be cast to LGLSXP
//===========================================================================
SEXP json_array_as_lglsxp(yyjson_val *arr, parse_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
    error("Error in json_array_as_lglsxp(): type = %s", yyjson_get_type_desc(arr));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create R LGLSXP vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t N = yyjson_arr_size(arr);
  SEXP res_ = PROTECT(allocVector(LGLSXP, (R_xlen_t)N));
  int32_t *res = INTEGER(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over JSON []-array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  yyjson_val *val;
  while ((val = yyjson_arr_iter_next(&iter))) {
    *res++ = json_val_to_logical(val, opt);
  }
  
  UNPROTECT(1);
  return res_;
}


//===========================================================================
// Parse a JSON []-array to INTSXP atomic vector
// Prerequisite: already know all types can be cast to INTSXP
//===========================================================================
SEXP json_array_as_intsxp(yyjson_val *arr, parse_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
    error("Error in json_array_as_intsxp(): type = %s", yyjson_get_type_desc(arr));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create R INTSXP vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t N = yyjson_arr_size(arr);
  SEXP res_ = PROTECT(allocVector(INTSXP, (R_xlen_t)N));
  int32_t *res = INTEGER(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over JSON []-array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  yyjson_val *val;
  while ((val = yyjson_arr_iter_next(&iter))) {
    *res++ = json_val_to_integer(val, opt);
  }
  
  UNPROTECT(1);
  return res_;
}


//===========================================================================
// Parse a JSON []-array to REALSXP atomic vector
// Prerequisite: already know all types can be cast to REALSXP
//===========================================================================
SEXP json_array_as_realsxp(yyjson_val *arr, parse_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
    error("Error in json_array_as_realsxp(): type = %s", yyjson_get_type_desc(arr));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create R REALSXP vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t N = yyjson_arr_size(arr);
  SEXP res_ = PROTECT(allocVector(REALSXP, (R_xlen_t)N));
  double *res = REAL(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  yyjson_val *val;
  while ((val = yyjson_arr_iter_next(&iter))) {
    *res++ = json_val_to_double(val, opt);
  }
  
  UNPROTECT(1);
  return res_;
}



//===========================================================================
// Parse a JSON []-array to REALSXP atomic vector of class bit64::integer64
// Prerequisite: already know all types can be cast to REALSXP/bit64::integer64
//===========================================================================
SEXP json_array_as_integer64(yyjson_val *arr, parse_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
    error("Error in json_array_as_realsxp(): type = %s", yyjson_get_type_desc(arr));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create R REALSXP vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t N = yyjson_arr_size(arr);
  SEXP res_ = PROTECT(allocVector(REALSXP, (R_xlen_t)N));
  long long *res = (long long *)REAL(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  yyjson_val *val;
  while ((val = yyjson_arr_iter_next(&iter))) {
    *res++ = json_val_to_integer64(val, opt);
  }
  
  setAttrib(res_, R_ClassSymbol, mkString("integer64"));
  
  UNPROTECT(1);
  return res_;
}


//===========================================================================
// Parse a JSON []-array as STRSXP 
// Prerequisite: already know all types can be cast to STRSXP
//===========================================================================
SEXP json_array_as_strsxp(yyjson_val *arr, parse_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
    error("Error in json_array_as_strsxp(): type = %s", yyjson_get_type_desc(arr));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create R STRSXP vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t N = yyjson_arr_size(arr);
  SEXP res_ = PROTECT(allocVector(STRSXP, (R_xlen_t)N));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  yyjson_val *val;
  unsigned int idx = 0;
  while ((val = yyjson_arr_iter_next(&iter))) {
    SET_STRING_ELT(res_, idx, json_val_to_charsxp(val, opt));
    idx++;
  }
  
  UNPROTECT(1);
  return res_;
}


//===========================================================================
// Parse a JSON []-array to a VECSXP (list)
// This can handle any json values 
//===========================================================================
SEXP json_array_as_vecsxp(yyjson_val *arr, parse_options *opt) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (yyjson_get_type(arr) != YYJSON_TYPE_ARR) {
    error("Error in json_array_as_vecsxp(): type = %s", yyjson_get_type_desc(arr));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create R VECSXP vector (i.e. a list)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(allocVector(VECSXP, (R_xlen_t)yyjson_arr_size(arr)));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over array and insert items into list
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter iter = yyjson_arr_iter_with( arr );
  yyjson_val *val;
  unsigned int idx = 0;
  while ((val = yyjson_arr_iter_next(&iter))) {
    SET_VECTOR_ELT(res_, idx, json_as_robj(val, opt));
    ++idx;
  }
  
  UNPROTECT(1);
  return res_;
}


//===========================================================================
// 
//===========================================================================
SEXP json_array_as_lglsxp_matrix(yyjson_val *arr, parse_options *opt) {
  
  size_t nrow  = yyjson_get_len(arr);
  size_t ncol  = yyjson_get_len(yyjson_arr_get_first(arr));
  
  SEXP mat_ = PROTECT(allocVector(LGLSXP, (R_xlen_t)(nrow * ncol)));
  int32_t *matp = INTEGER(mat_);
  
  yyjson_arr_iter iter1 = yyjson_arr_iter_with( arr );
  yyjson_val *inner_arr;
  
  unsigned int row = 0;
  while ((inner_arr = yyjson_arr_iter_next(&iter1))) {
    
    yyjson_arr_iter iter2 = yyjson_arr_iter_with( inner_arr );
    yyjson_val *val;
    
    unsigned int col = 0;
    while ((val = yyjson_arr_iter_next(&iter2))) {
      matp[col * nrow + row] = yyjson_get_bool(val);
      col++;
    }    
    
    row++;
  }
  
  UNPROTECT(1);
  return mat_;
}



//===========================================================================
// 
//===========================================================================
SEXP json_array_as_intsxp_matrix(yyjson_val *arr, parse_options *opt) {
  
  size_t nrow  = yyjson_get_len(arr);
  size_t ncol  = yyjson_get_len(yyjson_arr_get_first(arr));
  
  SEXP mat_ = PROTECT(allocVector(INTSXP, (R_xlen_t)(nrow * ncol)));
  int32_t *matp = INTEGER(mat_);
  
  yyjson_arr_iter iter1 = yyjson_arr_iter_with( arr );
  yyjson_val *inner_arr;
  
  unsigned int row = 0;
  while ((inner_arr = yyjson_arr_iter_next(&iter1))) {
    
    yyjson_arr_iter iter2 = yyjson_arr_iter_with( inner_arr );
    yyjson_val *val;
    
    unsigned int col = 0;
    while ((val = yyjson_arr_iter_next(&iter2))) {
      matp[col * nrow + row] = json_val_to_integer(val, opt);
      col++;
    }    
    
    row++;
  }
  
  UNPROTECT(1);
  return mat_;
}



//===========================================================================
// 
//===========================================================================
SEXP json_array_as_realsxp_matrix(yyjson_val *arr, parse_options *opt) {
  
  size_t nrow  = yyjson_get_len(arr);
  size_t ncol  = yyjson_get_len(yyjson_arr_get_first(arr));
  
  SEXP mat_ = PROTECT(allocVector(REALSXP, (R_xlen_t)(nrow * ncol)));
  double *matp = REAL(mat_);
  
  yyjson_arr_iter iter1 = yyjson_arr_iter_with( arr );
  yyjson_val *inner_arr;
  
  unsigned int row = 0;
  while ((inner_arr = yyjson_arr_iter_next(&iter1))) {
    
    yyjson_arr_iter iter2 = yyjson_arr_iter_with( inner_arr );
    yyjson_val *val;
    
    unsigned int col = 0;
    while ((val = yyjson_arr_iter_next(&iter2))) {
      matp[col * nrow + row] = json_val_to_double(val, opt);
      col++;
    }    
    
    row++;
  }
  
  UNPROTECT(1);
  return mat_;
}



//===========================================================================
// 
//===========================================================================
SEXP json_array_as_strsxp_matrix(yyjson_val *arr, parse_options *opt) {
  
  size_t nrow  = yyjson_get_len(arr);
  size_t ncol  = yyjson_get_len(yyjson_arr_get_first(arr));
  
  SEXP mat_ = PROTECT(allocVector(STRSXP, (R_xlen_t)(nrow * ncol)));
  
  yyjson_arr_iter iter1 = yyjson_arr_iter_with( arr );
  yyjson_val *inner_arr;
  
  unsigned int row = 0;
  while ((inner_arr = yyjson_arr_iter_next(&iter1))) {
    
    yyjson_arr_iter iter2 = yyjson_arr_iter_with( inner_arr );
    yyjson_val *val;
    
    unsigned int col = 0;
    while ((val = yyjson_arr_iter_next(&iter2))) {
      SET_STRING_ELT(mat_, (R_xlen_t)(col * nrow + row), json_val_to_charsxp(val, opt));
      col++;
    }    
    
    row++;
  }
  
  UNPROTECT(1);
  return mat_;
}


//===========================================================================
// 
//===========================================================================
SEXP json_array_as_matrix(yyjson_val *arr, unsigned int sexp_type, parse_options *opt) {
  
  int nprotect = 0;
  SEXP mat_ = R_NilValue;
  
  switch(sexp_type) {
  case LGLSXP:
    mat_ = PROTECT(json_array_as_lglsxp_matrix(arr, opt)); nprotect++;
    break;
  case INTSXP:
    mat_ = PROTECT(json_array_as_intsxp_matrix(arr, opt)); nprotect++;
    break;
  case REALSXP:
    mat_ = PROTECT(json_array_as_realsxp_matrix(arr, opt)); nprotect++;
    break;
  case STRSXP:
    mat_ = PROTECT(json_array_as_strsxp_matrix(arr, opt)); nprotect++;
    break;
  default:
    error("Could not parse matrix of type: %i -> %s\n", sexp_type, type2char(sexp_type));
  }
  
  if (!isNull(mat_)) {
    size_t ncol  = yyjson_get_len(arr);
    size_t nrow  = yyjson_get_len(yyjson_arr_get_first(arr));
    
    SEXP dims_ = PROTECT(allocVector(INTSXP, 2)); nprotect++;
    INTEGER(dims_)[0] = (int32_t)ncol; // JSON arrays are column-major
    INTEGER(dims_)[1] = (int32_t)nrow;
    
    setAttrib(mat_, R_DimSymbol, dims_);
  }
  
  UNPROTECT(nprotect);
  return mat_;
}





//===========================================================================
// Parse JSON []-array to R object
// 
// Possible outputs:
//   - Atomic vector: lgl, int, real, str, integer64
//   - List
//   - Matrix
//   - 3D array
//   - Data.frame
//===========================================================================
SEXP json_array_as_robj(yyjson_val *arr, parse_options *opt) {
  
  int nprotect = 0;
  SEXP res_ = R_NilValue;
  
  if (!yyjson_is_arr(arr)) {
    error("json_array_() got passed something NOT a json array");
  }
  
  
  size_t len = yyjson_get_len(arr);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Empty []-array becomes an empty list
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (len == 0) {
    res_ = PROTECT(allocVector(VECSXP, 0)); nprotect++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find what sort of containers exists within this array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int ctn_bitset = get_json_array_sub_container_types(arr, opt);
  
  if (ctn_bitset == CTN_NONE) {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // There are no containers within the array.
    // Process as an atomic vector or list.
    // Use the 'type_bitset' of all the elements to determine best SEXP
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    unsigned int type_bitset = get_type_bitset_for_json_array(arr, 0, opt);
    unsigned int sexp_type = get_best_sexp_to_represent_type_bitset(type_bitset, opt);
    
    switch(sexp_type) {
    case LGLSXP:
      res_ = PROTECT(json_array_as_lglsxp(arr, opt)); nprotect++;
      break;
    case INTSXP:
      res_ = PROTECT(json_array_as_intsxp(arr, opt)); nprotect++;
      break;
    case REALSXP:
      res_ = PROTECT(json_array_as_realsxp(arr, opt)); nprotect++;
      break;
    case STRSXP:
      res_ = PROTECT(json_array_as_strsxp(arr, opt)); nprotect++;
      break;
    case VECSXP:
      res_ = PROTECT(json_array_as_vecsxp(arr, opt)); nprotect++;
      break;
    case INT64SXP:
      res_ = PROTECT(json_array_as_integer64(arr, opt)); nprotect++;
      break;
    default:
      error("json_array_as_robj(). Ooops\n");
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Tag a length-1 array as class = 'AsIs'
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (opt->length1_array_asis && length(res_) == 1 && !inherits(res_, "Integer64")) {
      setAttrib(res_, R_ClassSymbol, mkString("AsIs"));
    }
    
  } else if (ctn_bitset == CTN_ARR) {
    unsigned int sexp_type = get_best_sexp_type_for_matrix(arr, opt);
    if (sexp_type != 0) {
      res_ = PROTECT(json_array_as_matrix(arr, sexp_type, opt)); nprotect++;
    } else {
      res_ = PROTECT(json_array_as_vecsxp(arr, opt)); nprotect++;
      
      // Check if compatible sub-matrices to make a 3d matrix
      //  i.e. 
      //    all members are matrices
      //    all members have the same dimension
      //    all matrices have the same type.
      //      Note: in future could check for compatible type e.g. int/real
      //      and promote all types to that for the final 3d matrix.
      //      For now, just keeping it basic.  Mike 2023-08-12
      bool is_3d_matrix = true;
      int dim0 = 0;
      int dim1 = 0;
      int nlayer = length(res_);
      unsigned int sexp_type = 0;
      
      if (nlayer > 1) {
        for (unsigned int layer = 0; layer < nlayer; layer++) {
          
          // check is matrix
          SEXP elem_ = VECTOR_ELT(res_, layer);
          if (!isMatrix(elem_)) {
            is_3d_matrix = false;
            break;
          }
          
          // Check dims
          SEXP dims_ = getAttrib(elem_, R_DimSymbol);
          if (layer == 0) {
            dim0 = INTEGER(dims_)[0];
            dim1 = INTEGER(dims_)[1];
          } else {
            if (INTEGER(dims_)[0] != dim0 || INTEGER(dims_)[1] != dim1) {
              is_3d_matrix = false;
              break;
            }
          }
          
          // check type
          if (layer == 0) {
            sexp_type = (unsigned int)TYPEOF(elem_);
          } else {
            if (TYPEOF(elem_) != sexp_type) {
              is_3d_matrix = false;
              break;
            }
          }
        }
        
        if (is_3d_matrix) {
          SEXP arr_ = R_NilValue;
          
          R_xlen_t N = nlayer * dim0 * dim1;
          switch(sexp_type) {
          case LGLSXP: {
            arr_ = PROTECT(allocVector(LGLSXP, N)); nprotect++;
            int *ptr = INTEGER(arr_);
            for (unsigned int layer = 0; layer < nlayer; layer++) {
              memcpy(ptr, INTEGER(VECTOR_ELT(res_, layer)), (size_t)dim0 * (size_t)dim1 * sizeof(int));
              ptr += dim0 * dim1;
            }
          }
            break;
          case INTSXP: {
            arr_ = PROTECT(allocVector(INTSXP, N)); nprotect++;
            int *ptr = INTEGER(arr_);
            for (unsigned int layer = 0; layer < nlayer; layer++) {
              memcpy(ptr, INTEGER(VECTOR_ELT(res_, layer)), (size_t)dim0 * (size_t)dim1 * sizeof(int));
              ptr += dim0 * dim1;
            }
          }
            break;
          case REALSXP: {
            arr_ = PROTECT(allocVector(REALSXP, N)); nprotect++;
            double *ptr = REAL(arr_);
            for (unsigned int layer = 0; layer < nlayer; layer++) {
              memcpy(ptr, REAL(VECTOR_ELT(res_, layer)), (size_t)dim0 * (size_t)dim1 * sizeof(double));
              ptr += dim0 * dim1;
            }
          }
            break;
          case STRSXP: {
            arr_ = PROTECT(allocVector(STRSXP, N)); nprotect++;
            unsigned int arr_idx = 0;
            for (unsigned int layer = 0; layer < nlayer; layer++) {
              SEXP mat_ = VECTOR_ELT(res_, layer);
              for (unsigned int idx = 0; idx < dim0 * dim1; idx++) {
                SET_STRING_ELT(arr_, arr_idx, STRING_ELT(mat_, idx));        
                arr_idx++;
              }
            }
          }
            break;
          default:
            warning("Warning: Unhandled 3d matrix type: %i (%s)\n", sexp_type, type2char(sexp_type));
          }
          
          // Set dims on new 3d array.
          SEXP dims_ = PROTECT(allocVector(INTSXP, 3)); nprotect++;
          INTEGER(dims_)[0] = dim0;
          INTEGER(dims_)[1] = dim1;
          INTEGER(dims_)[2] = nlayer;
          setAttrib(arr_, R_DimSymbol, dims_);
          
          res_ = arr_;
          
        }
      }

    }    
  } else if (ctn_bitset == CTN_OBJ && opt->arr_of_objs_to_df) {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // []-array ONLY contains {}-objects!
    // Parse as a data.frame
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    res_ = json_array_of_objects_to_data_frame(arr, opt);
  } else {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // This array contains a mixture of container types
    // Parse as list
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    res_ = PROTECT(json_array_as_vecsxp(arr, opt)); nprotect++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  UNPROTECT(nprotect);
  return res_;
}




//===========================================================================
//  ####           #                   #####                             
//   #  #          #                   #                                 
//   #  #   ###   ####    ###          #      # ##    ###   ## #    ###  
//   #  #      #   #         #         ####   ##  #      #  # # #  #   # 
//   #  #   ####   #      ####         #      #       ####  # # #  ##### 
//   #  #  #   #   #  #  #   #         #      #      #   #  # # #  #     
//  ####    ####    ##    ####         #      #       ####  #   #   ###  
//
//
// Parse []-array of only {}-objects. Extract a single key/value from 
// each {}-object and return all values as an atomic vector.
//
// Pre-requisite: 
//    'arr' is a JSON []-array
//    []-array only contains {}-objects
//===========================================================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Pre-requisites:
//   - all values within {}-objects accessible by key='key_name' can 
//     be contained in an LGLSXP
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP json_array_of_objects_to_lglsxp(yyjson_val *arr, const char *key_name, parse_options *opt) {
  
  size_t nrow = yyjson_get_len(arr);
  SEXP vec_ = PROTECT(allocVector(LGLSXP, (R_xlen_t)nrow)); 
  int *vecp = INTEGER(vec_);
  
  yyjson_arr_iter iter = yyjson_arr_iter_with(arr);
  yyjson_val *obj;
  
  while ((obj = yyjson_arr_iter_next(&iter))) {
    yyjson_val *val = yyjson_obj_get(obj, key_name);
    *vecp++ = json_val_to_logical(val, opt);
  }
  
  UNPROTECT(1);
  return vec_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Pre-requisites:
//   - all values within {}-objects accessible by key='key_name' can 
//     be contained in an INTSXP
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP json_array_of_objects_to_intsxp(yyjson_val *arr, const char *key_name, parse_options *opt) {
  
  size_t nrow = yyjson_get_len(arr);
  SEXP vec_ = PROTECT(allocVector(INTSXP, (R_xlen_t)nrow)); 
  int *vecp = INTEGER(vec_);
  
  yyjson_arr_iter iter = yyjson_arr_iter_with(arr);
  yyjson_val *obj;
  
  while ((obj = yyjson_arr_iter_next(&iter))) {
    yyjson_val *val = yyjson_obj_get(obj, key_name);
    *vecp++ = json_val_to_integer(val, opt);
  }
  
  UNPROTECT(1);
  return vec_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Pre-requisites:
//   - all values within {}-objects accessible by key='key_name' can 
//     be contained in an REALSXP
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP json_array_of_objects_to_realsxp(yyjson_val *arr, const char *key_name, parse_options *opt) {
  
  size_t nrow = yyjson_get_len(arr);
  SEXP vec_ = PROTECT(allocVector(REALSXP, (R_xlen_t)nrow));
  double *vecp = REAL(vec_);
  
  yyjson_arr_iter iter = yyjson_arr_iter_with(arr);
  yyjson_val *obj;
  
  while ((obj = yyjson_arr_iter_next(&iter))) {
    yyjson_val *val = yyjson_obj_get(obj, key_name);
    *vecp++ = json_val_to_double(val, opt);
  }
  
  UNPROTECT(1);
  return vec_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Pre-requisites:
//   - all values within {}-objects accessible by key='key_name' can 
//     be contained in an STRSXP
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP json_array_of_objects_to_strsxp(yyjson_val *arr, const char *key_name, parse_options *opt) {
  
  size_t nrow = yyjson_get_len(arr);
  SEXP vec_ = PROTECT(allocVector(STRSXP, (R_xlen_t)nrow)); 
  
  unsigned int idx = 0;
  yyjson_arr_iter iter = yyjson_arr_iter_with(arr);
  yyjson_val *obj;
  
  while ((obj = yyjson_arr_iter_next(&iter))) {
    yyjson_val *val = yyjson_obj_get(obj, key_name);
    SET_STRING_ELT(vec_, idx, json_val_to_charsxp(val, opt));
    idx++;
  }
  
  UNPROTECT(1);
  return vec_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// All values within {}-objects accessible by key='key_name' are 
// stored in a VECSXP (i.e. list)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP json_array_of_objects_to_vecsxp(yyjson_val *arr, const char *key_name, parse_options *opt) {
  
  size_t nrow = yyjson_get_len(arr);
  SEXP vec_ = PROTECT(allocVector(VECSXP, (R_xlen_t)nrow));
  
  unsigned int idx = 0;
  yyjson_arr_iter iter = yyjson_arr_iter_with(arr);
  yyjson_val *obj;
  
  while ((obj = yyjson_arr_iter_next(&iter))) {
    yyjson_val *val = yyjson_obj_get(obj, key_name);

    if (val == NULL) {
        SET_VECTOR_ELT(vec_, idx, opt->df_missing_list_elem); // NA_logical_
    } else {
      SET_VECTOR_ELT(vec_, idx, json_as_robj(val, opt));
    }
    
    idx++;
  }
  
  UNPROTECT(1);
  return vec_;
}





//===========================================================================
// Parse a JSON []-array of {}-objects into a data.frame
//
//  Pre-requisite:
//     - 'arr' is a JSON []-array
//     - all members of []-array are {}-objects
//===========================================================================
SEXP json_array_of_objects_to_data_frame(yyjson_val *arr, parse_options *opt) {
  
  int nprotect = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Accumulation of unique key-names in the objects
  // These will become the column names of the data.frame.
  // Each column also has a 'type_bitset' to keep track of the type of each
  // value across the different {}-objects
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *colname[MAX_DF_COLS];
  unsigned int type_bitset[MAX_DF_COLS] = {0};
  unsigned int ncols = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Prepare to iterate over all {}-objects within the []-array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int nrows = (unsigned int)yyjson_get_len(arr);
  yyjson_arr_iter iter = yyjson_arr_iter_with(arr);
  yyjson_val *obj;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // A pass over all {}-objects within the []-array
  // Accumulate
  //   - all unique names (in the order they are first encountered)
  //   - a 'type_bitset' for the values represented by each name
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while ((obj = yyjson_arr_iter_next(&iter))) {
    
    yyjson_val *key, *val;
    yyjson_obj_iter obj_iter = yyjson_obj_iter_with(obj);
    
    while ((key = yyjson_obj_iter_next(&obj_iter))) {
      val = yyjson_obj_iter_get_val(key);
      
      int name_idx = -1;
      for (int i = 0; i < ncols; i++) {
        if (yyjson_equals_str(key, colname[i])) {
          name_idx = i;
          break;
        }
      }
      if (name_idx < 0) {
        // Name has not been seen yet. so add it.
        name_idx = (int)ncols;
        colname[ncols] = (char *)yyjson_get_str(key);
        ncols++;
        if (ncols == MAX_DF_COLS) {
          error("Maximum columns for data.frame exceeded: %i", MAX_DF_COLS);
        }
      }
      
      type_bitset[name_idx] = update_type_bitset(type_bitset[name_idx], val, opt);
    }
  }
  
  
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
    
    unsigned int sexp_type = get_best_sexp_to_represent_type_bitset(type_bitset[col], opt);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Debugging types
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // dump_type_bitset(type_bitset[col]);
    // warning("[dfcol %i] %s - sexp_type: %i -> %s\n",
    //         col, colname[col],
    //         sexp_type, type2char(sexp_type));
    
    switch (sexp_type) {
    case LGLSXP:
      SET_VECTOR_ELT(df_, col, json_array_of_objects_to_lglsxp(arr, colname[col], opt));
      break;
    case INTSXP:
      SET_VECTOR_ELT(df_, col, json_array_of_objects_to_intsxp(arr, colname[col], opt));
      break;
    case REALSXP:
      SET_VECTOR_ELT(df_, col, json_array_of_objects_to_realsxp(arr, colname[col], opt));
      break;
    case STRSXP:
      SET_VECTOR_ELT(df_, col, json_array_of_objects_to_strsxp(arr, colname[col], opt));
      break;
    case VECSXP:
      SET_VECTOR_ELT(df_, col, json_array_of_objects_to_vecsxp(arr, colname[col], opt));
      break;
    default:
      warning("Unhandled 'df' coltype: %i -> %s\n", sexp_type, type2char(sexp_type));
      SET_VECTOR_ELT(df_, col, allocVector(LGLSXP, nrows));
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set colnames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(allocVector(STRSXP, ncols)); nprotect++;
  for (unsigned int i = 0; i < ncols; i++) {
    SET_STRING_ELT(nms_, i, mkChar(colname[i]));
  }
  Rf_setAttrib(df_, R_NamesSymbol, nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set rownames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rownames = PROTECT(allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -(int)nrows);
  setAttrib(df_, R_RowNamesSymbol, rownames);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 'data.frame' class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SET_CLASS(df_, mkString("data.frame"));
  
  
  UNPROTECT(nprotect);
  return df_;
}




//===========================================================================
//  #        #            #    
//  #                     #    
//  #       ##     ###   ####  
//  #        #    #       #    
//  #        #     ###    #    
//  #        #        #   #  # 
//  #####   ###   ####     ##  
//
// JSON {}-object to R List
//===========================================================================
SEXP json_object_as_list(yyjson_val *obj, parse_options *opt) {
  int nprotect = 0;
  
  if (!yyjson_is_obj(obj)) {
    error("json_object(): Must be object. Not %i -> %s\n", yyjson_get_type(obj), 
          yyjson_get_type_desc(obj));
  }
  R_xlen_t n = (R_xlen_t)yyjson_get_len(obj);
  
  SEXP res_ = PROTECT(allocVector(VECSXP, n)); nprotect++;
  SEXP nms_ = PROTECT(allocVector(STRSXP, n)); nprotect++;
  
  yyjson_val *key, *val;
  yyjson_obj_iter iter = yyjson_obj_iter_with(obj);
  unsigned int idx = 0;
  while ((key = yyjson_obj_iter_next(&iter))) {
    val = yyjson_obj_iter_get_val(key);
    SET_VECTOR_ELT(res_, idx, json_as_robj(val, opt));
    SET_STRING_ELT(nms_, idx, mkChar(yyjson_get_str(key)));
    ++idx;
  }
  
  Rf_setAttrib(res_, R_NamesSymbol, nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Test if list is promote-able to a data.frame
  //
  // * Opt to promote {}-object of []-arrays to data.frame
  // * All elements are atomic arrays or vecsxp
  // * All these elements are the same length
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (opt->obj_of_arrs_to_df) {
    R_xlen_t nrow = 0;
    bool possible_data_frame = true;
    for (unsigned int col = 0; col < idx; col++) {
      
      SEXP elem_ = VECTOR_ELT(res_, col);
      if (col == 0) {
        nrow = xlength(elem_);
      } else {
        R_xlen_t this_len = xlength(elem_);
        if (this_len != nrow) {
          possible_data_frame = false;
          break;
        }
      }
    }
    
    // all lengths are the same, and there is more
    // than 1 row, and there is more than 1 column
    if (possible_data_frame && nrow > 1 && idx > 1) {
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Set rownames on data.frame
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      SEXP rownames = PROTECT(allocVector(INTSXP, 2)); nprotect++;
      SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
      SET_INTEGER_ELT(rownames, 1, -(int)nrow);
      setAttrib(res_, R_RowNamesSymbol, rownames);
      
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Set 'data.frame' class
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      SET_CLASS(res_, mkString("data.frame"));
    }
  }
  
  UNPROTECT(nprotect);
  return res_;
}




//===========================================================================
//   ###                       
//  #   #                      
//  #       ###   # ##    ###  
//  #      #   #  ##  #  #   # 
//  #      #   #  #      ##### 
//  #   #  #   #  #      #     
//   ###    ###   #       ### 
//===========================================================================
SEXP json_as_robj(yyjson_val *val, parse_options *opt) {
  
  int nprotect = 0;
  static char buf[128];
  SEXP res_ = R_NilValue;
  
  switch (yyjson_get_type(val)) {
  case YYJSON_TYPE_OBJ:
    res_ = PROTECT(json_object_as_list(val, opt)); nprotect++;
    break;
  case YYJSON_TYPE_ARR:
    res_ = PROTECT(json_array_as_robj(val, opt)); nprotect++;
    break;
  case YYJSON_TYPE_BOOL:
    res_ = PROTECT(ScalarLogical(json_val_to_logical(val, opt))); nprotect++;
    break;
  case YYJSON_TYPE_NUM:
    switch (yyjson_get_subtype(val)) {
    case YYJSON_SUBTYPE_UINT:
    {
      uint64_t tmp = yyjson_get_uint(val);
      if (tmp > INT32_MAX) {
        if (opt->int64 == INT64_AS_STR) {
#if defined(__APPLE__) || defined(_WIN32)
          snprintf(buf, 128, "%llu", yyjson_get_uint(val));
#else
          snprintf(buf, 128, "%lu", yyjson_get_uint(val));
#endif
          res_ = PROTECT(mkString(buf)); nprotect++;  
        } else if (opt->int64 == INT64_AS_DBL) {
          double x = json_val_to_double(val, opt);
          res_ = PROTECT(ScalarReal(x)); nprotect++;
        } else if (opt->int64 == INT64_AS_BIT64) {
          if (tmp > INT64_MAX) {
            warning("64bit unsigned integer values exceed bit64::integer64. Expect overflow");
          }
          long long x = json_val_to_integer64(val, opt);
          res_ = PROTECT(ScalarReal(0)); nprotect++;
          ((long long *)(REAL(res_)))[0] = x;
          setAttrib(res_, R_ClassSymbol, mkString("integer64"));
        } else {
          error("Unhandled opt.bit64 option for YYJSON_SUBTYPE_UINT");
        }
      } else {
        res_ = PROTECT(ScalarInteger((int32_t)tmp)); nprotect++;
      }
    }
      break;
    case YYJSON_SUBTYPE_SINT:
    {
      int64_t tmp = yyjson_get_sint(val);
      if (tmp < INT32_MIN || tmp > INT32_MAX) {
        if (opt->int64 == INT64_AS_STR) {
#if defined(__APPLE__) || defined(_WIN32)
        snprintf(buf, 128, "%lld", yyjson_get_sint(val));
#else
        snprintf(buf, 128, "%ld", yyjson_get_sint(val));
#endif
        res_ = PROTECT(mkString(buf)); nprotect++;
        } else if (opt->int64 == INT64_AS_DBL) {
          double x = json_val_to_double(val, opt);
          res_ = PROTECT(ScalarReal(x)); nprotect++;
        } else if (opt->int64 == INT64_AS_BIT64) {
          if (tmp > INT64_MAX || tmp < INT64_MIN) {
            warning("64bit signed integer values exceed bit64::integer64. Expect overflow");
          }
          long long x = json_val_to_integer64(val, opt);
          res_ = PROTECT(ScalarReal(0)); nprotect++;
          ((long long *)(REAL(res_)))[0] = x;
          setAttrib(res_, R_ClassSymbol, mkString("integer64"));
        } else {
          error("Unhandled opt.bit64 option for YYJSON_SUBTYPE_SINT");
        }
      } else {
        res_ = PROTECT(ScalarInteger((int32_t)tmp)); nprotect++;
      }
    }
      break;
    case YYJSON_SUBTYPE_REAL:
      res_ = PROTECT(ScalarReal(yyjson_get_real(val))); nprotect++;
      break;
    default:
      warning("json_as_robj(). Unhandled numeric type: %i\n", yyjson_get_subtype(val));
    }
    break;
  case YYJSON_TYPE_STR:
    res_ = PROTECT(mkString(yyjson_get_str(val))); nprotect++;
    break;
  case YYJSON_TYPE_NULL:
    res_ = R_NilValue;
    break;
  default:
    warning("json_as_robj(): unhandled: %s\n", yyjson_get_type_desc(val));
  }
  
  UNPROTECT(nprotect);
  return res_;
}


#define ERR_CONTEXT 20
void output_verbose_error(const char *str, yyjson_read_err err) {
  // Slice off a bit of the string within +/- ERR_CONTEXT of the error pos
  size_t min_idx = err.pos - ERR_CONTEXT;
  min_idx = min_idx < 0 ? 0 : min_idx;
  size_t max_idx = err.pos + ERR_CONTEXT;
  max_idx = max_idx > strlen(str) ? strlen(str) : max_idx;
  
  // copy this context into a temp string. ensure it ends in '\0'
  char err_string[3 * ERR_CONTEXT];
  strncpy((char *)&err_string, str + min_idx, max_idx - min_idx);
  err_string[max_idx - min_idx] = '\0';
  Rprintf("%s\n", err_string);
  
  // Print a "^" to point to the error
  size_t pos = ERR_CONTEXT;
  pos = err.pos < ERR_CONTEXT ? err.pos - 1 : ERR_CONTEXT;
  for (unsigned int i = 0; i < pos; i++) {
    Rprintf(" ");
  }
  Rprintf("^\n");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_json_from_str(const char *str, parse_options *opt) {
  
  yyjson_read_err err;
  yyjson_doc *doc = yyjson_read_opts((char *)str, strlen(str), opt->yyjson_read_flag, NULL, &err);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If doc is NULL, then an error occurred during parsing.
  // Try and print something sensible like {jsonlite} does.  
  // I.e.
  //   - print some context around where the error occurred
  //   - print the index in the character string where the error occurred
  //   - add a visual pointer to the output so the user knows where this was
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (doc == NULL) {
    output_verbose_error(str, err);
#if defined(_WIN32)
    error("Error parsing JSON: %s code: %u at position: %llu\n", err.msg, err.code, err.pos);
#else
    error("Error parsing JSON: %s code: %u at position: %lu\n", err.msg, err.code, err.pos);
#endif
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the document from the root node
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(json_as_robj(yyjson_doc_get_root(doc), opt));
  
  yyjson_doc_free(doc);
  
  UNPROTECT(1);
  return res_;
} 


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_json_from_file(const char *filename, parse_options *opt) {
  
  yyjson_read_err err;
  
  // yyjson_doc *yyjson_read_file(const char *path,
  //                              yyjson_read_flag flg,
  //                              const yyjson_alc *alc,
  //                              yyjson_read_err *err);
  
  yyjson_doc *doc = yyjson_read_file((char *)filename, opt->yyjson_read_flag, NULL, &err);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If doc is NULL, then an error occurred during parsing.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (doc == NULL) {
#if defined(_WIN32)
    error("Error parsing JSON file '%s': %s code: %u at position: %llu\n", filename, err.msg, err.code, err.pos);
#else
    error("Error parsing JSON file '%s': %s code: %u at position: %lu\n", filename, err.msg, err.code, err.pos);
#endif
    
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the document from the root node
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(json_as_robj(yyjson_doc_get_root(doc), opt));
  
  yyjson_doc_free(doc);
  
  UNPROTECT(1);
  return res_;
} 



//===========================================================================
//  ####                              
//  #   #                             
//  #   #   ###   # ##    ###    ###  
//  ####       #  ##  #  #      #   # 
//  #       ####  #       ###   ##### 
//  #      #   #  #          #  #     
//  #       ####  #      ####    ###  
//
// Parse string containing JSON to an R object
//===========================================================================
SEXP parse_from_str_(SEXP str_, SEXP parse_opts_) {
  
  const char *str = (const char *)CHAR( STRING_ELT(str_, 0) );
  parse_options opt = create_parse_options(parse_opts_);
  
  return parse_json_from_str(str, &opt);
}

//===========================================================================
// From RAW vector
//===========================================================================
SEXP parse_from_raw_(SEXP raw_, SEXP parse_opts_) {
  
  const char *str = (const char *)RAW(raw_);
  parse_options opt = create_parse_options(parse_opts_);
  
  // Raw string may or may not have a NULL byte terminator.
  // So ask 'yyjson' to stop parsing when the json parsing naturally ends
  // rather than running over into dead space after the raw string ends
  opt.yyjson_read_flag |= YYJSON_READ_STOP_WHEN_DONE;
  
  return parse_json_from_str(str, &opt);
}

//===========================================================================
// Parse from file given as a filename
//===========================================================================
SEXP parse_from_file_(SEXP filename_, SEXP parse_opts_) {
  
  const char *filename = (const char *)CHAR( STRING_ELT(filename_, 0) );
  filename = R_ExpandFileName(filename);
  parse_options opt = create_parse_options(parse_opts_);
  
  return parse_json_from_file(filename, &opt);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Validate
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP validate_json_file_(SEXP filename_, SEXP verbose_, SEXP parse_opts_) {
  
  const char *filename = (const char *)CHAR( STRING_ELT(filename_, 0) );
  parse_options opt = create_parse_options(parse_opts_);
  
  yyjson_read_err err;
  yyjson_doc *doc = yyjson_read_file((char *)filename, opt.yyjson_read_flag, NULL, &err);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If doc is NULL, then an error occurred during parsing.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (doc == NULL) {
    if (asLogical(verbose_)) {
#if defined(_WIN32)
      warning("Error parsing JSON file '%s': %s code: %u at position: %llu\n", filename, err.msg, err.code, err.pos);
#else
      warning("Error parsing JSON file '%s': %s code: %u at position: %lu\n", filename, err.msg, err.code, err.pos);
#endif
    }
    return ScalarLogical(0);
    
  }
  
  yyjson_doc_free(doc);
  
  return ScalarLogical(1);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Validate
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP validate_json_str_(SEXP str_, SEXP verbose_, SEXP parse_opts_) {
  const char *str = (const char *)CHAR( STRING_ELT(str_, 0) );
  parse_options opt = create_parse_options(parse_opts_);
  
  yyjson_read_err err;
  yyjson_doc *doc = yyjson_read_opts((char *)str, strlen(str), opt.yyjson_read_flag, NULL, &err);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If doc is NULL, then an error occurred during parsing.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (doc == NULL) {
    if (asLogical(verbose_)) {
      output_verbose_error(str, err);
#if defined(_WIN32)
      warning("Error parsing JSON: %s code: %u at position: %llu\n", err.msg, err.code, err.pos);
#else
      warning("Error parsing JSON: %s code: %u at position: %lu\n", err.msg, err.code, err.pos);
#endif
    }
    return ScalarLogical(0);
  }
  
  yyjson_doc_free(doc);
  return ScalarLogical(1);
}


