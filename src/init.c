
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

extern SEXP parse_from_str_ (SEXP str_     , SEXP parse_opts_);
extern SEXP parse_from_file_(SEXP filename_, SEXP parse_opts_);
extern SEXP parse_from_raw_ (SEXP filename_, SEXP parse_opts_);

extern SEXP serialize_to_str_ (SEXP x_,                 SEXP serialize_opts_);
extern SEXP serialize_to_file_(SEXP x_, SEXP filename_, SEXP serialize_opts_);

extern SEXP validate_json_file_(SEXP filename_, SEXP verbose_, SEXP parse_opts_);
extern SEXP validate_json_str_ (SEXP str_     , SEXP verbose_, SEXP parse_opts_);

static const R_CallMethodDef CEntries[] = {
  {"serialize_to_str_" , (DL_FUNC) &serialize_to_str_ , 2},
  {"serialize_to_file_", (DL_FUNC) &serialize_to_file_, 3},
  
  {"parse_from_str_"  , (DL_FUNC) &parse_from_str_ , 2},
  {"parse_from_file_" , (DL_FUNC) &parse_from_file_, 2},
  {"parse_from_raw_"  , (DL_FUNC) &parse_from_raw_ , 2},
  
  {"validate_json_file_", (DL_FUNC) &validate_json_file_, 3},
  {"validate_json_str_" , (DL_FUNC) &validate_json_str_ , 3},
  
  {NULL , NULL, 0}
};


void R_init_yyjsonr(DllInfo *info) {
  R_registerRoutines(
    info,      // DllInfo
    NULL,      // .C
    CEntries,  // .Call
    NULL,      // Fortran
    NULL       // External
  );
  R_useDynamicSymbols(info, FALSE);
}



