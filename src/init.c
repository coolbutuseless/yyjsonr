
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>


SEXP yyjson_version_(void);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Regular JSON
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern SEXP parse_from_str_ (SEXP str_     , SEXP parse_opts_);
extern SEXP parse_from_file_(SEXP filename_, SEXP parse_opts_);
extern SEXP parse_from_raw_ (SEXP filename_, SEXP parse_opts_);

extern SEXP serialize_to_str_ (SEXP x_,                 SEXP serialize_opts_, SEXP as_raw_);
extern SEXP serialize_to_file_(SEXP x_, SEXP filename_, SEXP serialize_opts_);

extern SEXP validate_json_file_(SEXP filename_, SEXP verbose_, SEXP parse_opts_);
extern SEXP validate_json_str_ (SEXP str_     , SEXP verbose_, SEXP parse_opts_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NDJSON
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern SEXP parse_ndjson_file_as_df_  (SEXP filename_, SEXP nread_, SEXP nskip_, SEXP nprobe_, SEXP parse_opts_);
extern SEXP parse_ndjson_file_as_list_(SEXP filename_, SEXP nread_, SEXP nskip_,               SEXP parse_opts_);

extern SEXP parse_ndjson_str_as_df_  (SEXP str_, SEXP nread_, SEXP nskip_, SEXP nprobe_, SEXP parse_opts_);
extern SEXP parse_ndjson_str_as_list_(SEXP str_, SEXP nread_, SEXP nskip_,               SEXP parse_opts_);

extern SEXP serialize_df_to_ndjson_str_ (SEXP robj_,                 SEXP serialize_opts_, SEXP as_raw_);
extern SEXP serialize_df_to_ndjson_file_(SEXP robj_, SEXP filename_, SEXP serialize_opts_);

extern SEXP serialize_list_to_ndjson_str_ (SEXP robj_,                 SEXP serialize_opts_, SEXP as_raw_);
extern SEXP serialize_list_to_ndjson_file_(SEXP robj_, SEXP filename_, SEXP serialize_opts_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GeoJSON
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern SEXP parse_geojson_str_ (SEXP str_     , SEXP geo_opts_, SEXP parse_opts_);
extern SEXP parse_geojson_file_(SEXP filename_, SEXP geo_opts_, SEXP parse_opts_);

extern SEXP serialize_sf_to_str_ (SEXP sf_                , SEXP geo_opts_, SEXP serialize_opts_);
extern SEXP serialize_sf_to_file_(SEXP sf_, SEXP filename_, SEXP geo_opts_, SEXP serialize_opts_);


static const R_CallMethodDef CEntries[] = {
  
  {"yyjson_version_", (DL_FUNC) &yyjson_version_, 0},
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Regular JSON
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  {"serialize_to_str_" , (DL_FUNC) &serialize_to_str_ , 3},
  {"serialize_to_file_", (DL_FUNC) &serialize_to_file_, 3},
  
  {"parse_from_str_"  , (DL_FUNC) &parse_from_str_ , 2},
  {"parse_from_file_" , (DL_FUNC) &parse_from_file_, 2},
  {"parse_from_raw_"  , (DL_FUNC) &parse_from_raw_ , 2},
  
  {"validate_json_file_", (DL_FUNC) &validate_json_file_, 3},
  {"validate_json_str_" , (DL_FUNC) &validate_json_str_ , 3},
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // NDJSON
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  {"parse_ndjson_file_as_df_"  , (DL_FUNC) &parse_ndjson_file_as_df_  , 5},
  {"parse_ndjson_file_as_list_", (DL_FUNC) &parse_ndjson_file_as_list_, 4},
  
  {"parse_ndjson_str_as_df_"  , (DL_FUNC) &parse_ndjson_str_as_df_  , 5},
  {"parse_ndjson_str_as_list_", (DL_FUNC) &parse_ndjson_str_as_list_, 4},
  
  {"serialize_df_to_ndjson_str_" , (DL_FUNC) &serialize_df_to_ndjson_str_ , 3},
  {"serialize_df_to_ndjson_file_", (DL_FUNC) &serialize_df_to_ndjson_file_, 3},
  
  {"serialize_list_to_ndjson_str_" , (DL_FUNC) &serialize_list_to_ndjson_str_ , 3},
  {"serialize_list_to_ndjson_file_", (DL_FUNC) &serialize_list_to_ndjson_file_, 3},
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // GeoJSON
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  {"parse_geojson_str_" , (DL_FUNC) &parse_geojson_str_ , 3},
  {"parse_geojson_file_", (DL_FUNC) &parse_geojson_file_, 3},

  {"serialize_sf_to_str_" , (DL_FUNC) &serialize_sf_to_str_ , 3},
  {"serialize_sf_to_file_", (DL_FUNC) &serialize_sf_to_file_, 4},
  
  
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



