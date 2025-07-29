
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
#include "R-yyjson-parse.h"

#define SF_POINT               1 << 1
#define SF_MULTIPOINT          1 << 2
#define SF_LINESTRING          1 << 3
#define SF_MULTILINESTRING     1 << 4
#define SF_POLYGON             1 << 5
#define SF_MULTIPOLYGON        1 << 6
#define SF_FEATURE_COLLECTION  1 << 7
#define SF_GEOMETRY_COLLECTION 1 << 8

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Make a CRS string to match what is done by: geojsonsf::geojson_sf()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP make_crs(void) {
  SEXP crs_  = PROTECT(Rf_allocVector(VECSXP, 2));
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, 2));
  SET_STRING_ELT(nms_, 0, Rf_mkChar("input"));
  SET_STRING_ELT(nms_, 1, Rf_mkChar("wkt"));
  Rf_setAttrib(crs_, R_NamesSymbol, nms_);
  Rf_setAttrib(crs_, R_ClassSymbol, Rf_mkString("crs"));
  SET_VECTOR_ELT(crs_, 0, Rf_mkString("4326"));
  SET_VECTOR_ELT(crs_, 1, Rf_mkString("GEOGCS[\"WGS 84\",\n      DATUM[\"WGS_1984\",\n        SPHEROID[\"WGS 84\",6378137,298.257223563,\n          AUTHORITY[\"EPSG\",\"7030\"]],\n        AUTHORITY[\"EPSG\",\"6326\"]],\n      PRIMEM[\"Greenwich\",0,\n        AUTHORITY[\"EPSG\",\"8901\"]],\n      UNIT[\"degree\",0.0174532925199433,\n        AUTHORITY[\"EPSG\",\"9122\"]],\n      AXIS[\"Latitude\",NORTH],\n      AXIS[\"Longitude\",EAST],\n    AUTHORITY[\"EPSG\",\"4326\"]]"));
  
  UNPROTECT(2);
  return crs_;
}

#define SF_TYPE  1  // data.frame
#define SFC_TYPE 2  // list

#define PROP_TYPE_LIST   0
#define PROP_TYPE_STRING 1

#define PROP_LGL_AS_STR 0
#define PROP_LGL_AS_INT 1

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GeoJSON Parse options
//  - this also contains state information i.e.
//      - current bounding box accumulations
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  bool property_promotion;
  bool property_promotion_lgl; // When promoting properties to string, should lgl be '1' or 'TRUE"?
  unsigned int type;
  unsigned int yyjson_read_flag;
  parse_options *parse_opt;
  double xmin;
  double ymin;
  double xmax;
  double ymax;
  double zmin;
  double zmax;
  double mmin;
  double mmax;
} geo_parse_options;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise geo_parse_opts from an R named list from the user
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
geo_parse_options create_geo_parse_options(SEXP geo_opts_) {
  geo_parse_options opt = {
    .property_promotion = PROP_TYPE_STRING, // emulate 'geojsonsf' behaviour
  .property_promotion_lgl = PROP_LGL_AS_INT, // emulate 'geojsonsf' behaviour
    .type = SF_TYPE,
    .yyjson_read_flag = 0,
    .xmin =  INFINITY,
    .ymin =  INFINITY,
    .xmax = -INFINITY,
    .ymax = -INFINITY,
    .zmin =  INFINITY,
    .zmax = -INFINITY,
    .mmin =  INFINITY,
    .mmax = -INFINITY
  };
  
  if (Rf_isNull(geo_opts_) || Rf_length(geo_opts_) == 0) {
    return opt;
  }
  
  if (!Rf_isNewList(geo_opts_)) {
    Rf_error("'geo_opts_' must be a list");
  }
  
  SEXP nms_ = Rf_getAttrib(geo_opts_, R_NamesSymbol);
  if (Rf_isNull(nms_)) {
    Rf_error("'geo_opts_' must be a named list");
  }
  
  for (int i = 0; i < Rf_length(geo_opts_); i++) {
    const char *opt_name = CHAR(STRING_ELT(nms_, i));
    SEXP val_ = VECTOR_ELT(geo_opts_, i);
    
    if (strcmp(opt_name, "property_promotion") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      opt.property_promotion = strcmp(val, "string") == 0 ? PROP_TYPE_STRING : PROP_TYPE_LIST;
    } else if (strcmp(opt_name, "property_promotion_lgl") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      opt.property_promotion_lgl = strcmp(val, "string") == 0 ? PROP_LGL_AS_STR : PROP_LGL_AS_INT;
    } else if (strcmp(opt_name, "type") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      opt.type = strcmp(val, "sf") == 0 ? SF_TYPE : SFC_TYPE;
    } else {
      Rf_warning("opt_geojson_read(): Unknown option ignored: '%s'\n", opt_name);
    }
  }
  
  
  return opt;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// reset the bounding box in 'opt'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void reset_bbox(geo_parse_options *opt) {
  opt->xmin =  INFINITY;
  opt->ymin =  INFINITY;
  opt->xmax = -INFINITY;
  opt->ymax = -INFINITY;
  opt->zmin =  INFINITY;
  opt->zmax = -INFINITY;
  opt->mmin =  INFINITY;
  opt->mmax = -INFINITY;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Make an R bbox object based upon the current bbox in 'opt'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP make_bbox(geo_parse_options *opt) {
  
  int nprotect = 0;
  
  SEXP bbox_ = PROTECT(Rf_allocVector(REALSXP, 4)); nprotect++;
  REAL(bbox_)[0] = R_FINITE(opt->xmin) ? opt->xmin : NA_REAL;
  REAL(bbox_)[1] = R_FINITE(opt->ymin) ? opt->ymin : NA_REAL;
  REAL(bbox_)[2] = R_FINITE(opt->xmax) ? opt->xmax : NA_REAL;
  REAL(bbox_)[3] = R_FINITE(opt->ymax) ? opt->ymax : NA_REAL;
  SEXP bbox_nms_ = PROTECT(Rf_allocVector(STRSXP, 4)); nprotect++;
  SET_STRING_ELT(bbox_nms_, 0, Rf_mkChar("xmin"));
  SET_STRING_ELT(bbox_nms_, 1, Rf_mkChar("ymin"));
  SET_STRING_ELT(bbox_nms_, 2, Rf_mkChar("xmax"));
  SET_STRING_ELT(bbox_nms_, 3, Rf_mkChar("ymax"));
  Rf_setAttrib(bbox_, R_NamesSymbol, bbox_nms_);
  Rf_setAttrib(bbox_, R_ClassSymbol, Rf_mkString("bbox"));
  
  UNPROTECT(nprotect);
  return bbox_;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Make a z-range object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP make_z_range(geo_parse_options *opt) {
  
  int nprotect = 0;
  
  SEXP z_range_ = PROTECT(Rf_allocVector(REALSXP, 2)); nprotect++;
  REAL(z_range_)[0] = R_FINITE(opt->zmin) ? opt->zmin : NA_REAL;
  REAL(z_range_)[1] = R_FINITE(opt->zmax) ? opt->zmax : NA_REAL;
  SEXP z_range_nms_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  SET_STRING_ELT(z_range_nms_, 0, Rf_mkChar("zmin"));
  SET_STRING_ELT(z_range_nms_, 1, Rf_mkChar("zmax"));
  Rf_setAttrib(z_range_, R_NamesSymbol, z_range_nms_);
  Rf_setAttrib(z_range_, R_ClassSymbol, Rf_mkString("z_range"));
  
  UNPROTECT(nprotect);
  return z_range_;
}


bool needs_z_range(geo_parse_options *opt) {
  return R_FINITE(opt->zmin) && 
    R_FINITE(opt->zmax) &&
    opt->zmin != NA_REAL &&
    opt->zmax != NA_REAL
    ;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Make a z-range object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP make_m_range(geo_parse_options *opt) {
  
  int nprotect = 0;
  
  SEXP m_range_ = PROTECT(Rf_allocVector(REALSXP, 2)); nprotect++;
  REAL(m_range_)[0] = R_FINITE(opt->mmin) ? opt->mmin : NA_REAL;
  REAL(m_range_)[1] = R_FINITE(opt->mmax) ? opt->mmax : NA_REAL;
  SEXP m_range_nms_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  SET_STRING_ELT(m_range_nms_, 0, Rf_mkChar("mmin"));
  SET_STRING_ELT(m_range_nms_, 1, Rf_mkChar("mmax"));
  Rf_setAttrib(m_range_, R_NamesSymbol, m_range_nms_);
  Rf_setAttrib(m_range_, R_ClassSymbol, Rf_mkString("m_range"));
  
  UNPROTECT(nprotect);
  return m_range_;
}


bool needs_m_range(geo_parse_options *opt) {
  return R_FINITE(opt->mmin);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Forward Declarations
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP geojson_as_sf(yyjson_val *val, geo_parse_options *opt, unsigned int depth);

#define COORD_XY   2
#define COORD_XYZ  3
#define COORD_XYZM 4

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Determine if the coords in this array of coordinate arrays is
//   XY, XYZ or XYZM
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned int calc_matrix_coord_type(yyjson_val *arr, geo_parse_options *opt) {
  unsigned int coord_bitset = 0;
  unsigned int coord_type = COORD_XY;
  yyjson_arr_iter row_iter = yyjson_arr_iter_with(arr);
  yyjson_val *row;
  while ((row = yyjson_arr_iter_next(&row_iter))) {
    size_t ncoords = yyjson_get_len(row);
    coord_bitset |= (1 << ncoords);
  }
  
  if (coord_bitset & (1 << 4)) {
    coord_type = COORD_XYZM;
  } else if (coord_bitset & (1 << 3)) {
    coord_type = COORD_XYZ;
  } else {
    coord_type = COORD_XY;
  }
  
  return coord_type;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    ###                            #        
//   #   #                           #        
//   #       ###    ###   # ##    ## #   ###  
//   #      #   #  #   #  ##  #  #  ##  #     
//   #      #   #  #   #  #      #   #   ###  
//   #   #  #   #  #   #  #      #  ##      # 
//    ###    ###    ###   #       ## #  ####  
//
// Parse a JSON []-array of coordinates to a matrix.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_coords_as_matrix(yyjson_val *arr, unsigned int coord_type, geo_parse_options *opt) {
  
  size_t nrows = yyjson_get_len(arr);
  size_t ncols = coord_type;
  size_t N = nrows * ncols;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate memory for the R-matrix
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP mat_ = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t)N)); 
  double *ptr = REAL(mat_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over nested JSON []-arrays to populate the R-matrix
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter row_iter = yyjson_arr_iter_with(arr);
  yyjson_val *row;
  unsigned int row_idx = 0;
  while ((row = yyjson_arr_iter_next(&row_iter))) {
    
    yyjson_arr_iter col_iter = yyjson_arr_iter_with(row);
    yyjson_val *val;
    unsigned int col_idx = 0;
    while((val = yyjson_arr_iter_next(&col_iter))) {
      double tmp = yyjson_get_num(val);
      ptr[row_idx + col_idx * nrows] = tmp;
      
      if (col_idx == 0) {
        if (tmp > opt->xmax) opt->xmax = tmp;
        if (tmp < opt->xmin) opt->xmin = tmp;
      } else if (col_idx == 1) {
        if (tmp > opt->ymax) opt->ymax = tmp;
        if (tmp < opt->ymin) opt->ymin = tmp;
      } else if (col_idx == 2) {
        if (tmp > opt->zmax) opt->zmax = tmp;
        if (tmp < opt->zmin) opt->zmin = tmp;
        if (tmp == NA_REAL) {
          opt->zmax = NA_REAL;
          opt->zmin = NA_REAL;
        }
      } else if (col_idx == 3) {
        if (tmp > opt->mmax) opt->mmax = tmp;
        if (tmp < opt->mmin) opt->mmin = tmp;
      }
      
      col_idx++;
    }
    
    for (; col_idx < coord_type; col_idx++) {
      ptr[row_idx + col_idx * nrows] = NA_REAL;
    }
    
    row_idx++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dims
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dims_ = PROTECT(Rf_allocVector(INTSXP, 2)); 
  INTEGER(dims_)[0] = (int)nrows;
  INTEGER(dims_)[1] = (int)ncols;
  Rf_setAttrib(mat_, R_DimSymbol, dims_);
  
  UNPROTECT(2);
  return mat_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse multiple matrices into a list 
// Used for polygon and multipolygon
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_coords_as_matrix_list(yyjson_val *arr, 
                                 unsigned int *accumulated_coord_type, 
                                 geo_parse_options *opt) {
  size_t nrings = yyjson_get_len(arr);
  
  SEXP ll_ = PROTECT(Rf_allocVector(VECSXP, (R_xlen_t)nrings)); 
  
  yyjson_arr_iter ring_iter = yyjson_arr_iter_with(arr);
  yyjson_val *coords;
  unsigned int ring_idx = 0;
  unsigned int coord_type = COORD_XY;
  while ((coords = yyjson_arr_iter_next(&ring_iter))) {
    
    coord_type = calc_matrix_coord_type(coords, opt);
    SEXP mat_ = PROTECT(parse_coords_as_matrix(coords, coord_type, opt));
    SET_VECTOR_ELT(ll_, ring_idx, mat_);
    
    UNPROTECT(1);
    ring_idx++;
  }
  
  *accumulated_coord_type = coord_type;
  
  UNPROTECT(1);
  return ll_;
}

static char *COORD_SYSTEM[5] = {"NA", "NA", "XY", "XYZ", "XYZM"};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    ###                                #                  
//   #   #                               #                  
//   #       ###    ###   ## #    ###   ####   # ##   #   # 
//   #      #   #  #   #  # # #  #   #   #     ##  #  #   # 
//   #  ##  #####  #   #  # # #  #####   #     #      #  ## 
//   #   #  #      #   #  # # #  #       #  #  #       ## # 
//    ###    ###    ###   #   #   ###     ##   #          # 
//                                                    #   # 
//                                                     ###  
//
//  Point
//  MultiPoint
//  LineString
//  MultiLineString
//  Polygon
//  MultiPolygon
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_point(yyjson_val *obj, geo_parse_options *opt) {
  // Rprintf(">> Point\n");
  
  yyjson_val *coords = yyjson_obj_get(obj, "coordinates");
  size_t N = yyjson_get_len(coords);
  
  SEXP vec_ = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t)N));
  double *ptr = REAL(vec_);
  
  yyjson_arr_iter iter = yyjson_arr_iter_with(coords);
  yyjson_val *val;
  unsigned int idx = 0;
  while ((val = yyjson_arr_iter_next(&iter))) {
    ptr[idx] = yyjson_get_num(val);
    
    // XY bounding box
    if (idx == 0) {
      if (ptr[idx] > opt->xmax) opt->xmax = ptr[idx];
      if (ptr[idx] < opt->xmin) opt->xmin = ptr[idx];
    } else if (idx == 1) {
      if (ptr[idx] > opt->ymax) opt->ymax = ptr[idx];
      if (ptr[idx] < opt->ymin) opt->ymin = ptr[idx];
    } else if (idx == 2) {
      if (ptr[idx] > opt->zmax) opt->zmax = ptr[idx];
      if (ptr[idx] < opt->zmin) opt->zmin = ptr[idx];
      if (ptr[idx] == NA_REAL) {
        opt->zmax = NA_REAL;
        opt->zmin = NA_REAL;
      }
    } else if (idx == 3) {
      if (ptr[idx] > opt->mmax) opt->mmax = ptr[idx];
      if (ptr[idx] < opt->mmin) opt->mmin = ptr[idx];
    }
    
    idx++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, 3));
  SET_STRING_ELT(nms_, 0, Rf_mkChar(COORD_SYSTEM[N]));
  SET_STRING_ELT(nms_, 1, Rf_mkChar("POINT"));
  SET_STRING_ELT(nms_, 2, Rf_mkChar("sfg"));
  Rf_setAttrib(vec_, R_ClassSymbol, nms_);
  
  UNPROTECT(2);
  return vec_;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Multipoint
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_multipoint(yyjson_val *obj, geo_parse_options *opt) {
  // Rprintf(">> Point\n");
  
  yyjson_val *coords = yyjson_obj_get(obj, "coordinates");
  
  unsigned int coord_type = calc_matrix_coord_type(coords, opt);
  // Rprintf("Multipoint %i\n", coord_type);
  SEXP mat_ = PROTECT(parse_coords_as_matrix(coords, coord_type, opt));
  
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, 3));
  SET_STRING_ELT(nms_, 0, Rf_mkChar(COORD_SYSTEM[coord_type]));
  SET_STRING_ELT(nms_, 1, Rf_mkChar("MULTIPOINT"));
  SET_STRING_ELT(nms_, 2, Rf_mkChar("sfg"));
  Rf_setAttrib(mat_, R_ClassSymbol, nms_);
  
  UNPROTECT(2);
  return mat_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  LINESTRING
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_linestring(yyjson_val *obj, geo_parse_options *opt) {
  
  yyjson_val *coords = yyjson_obj_get(obj, "coordinates");
  
  unsigned int coord_type = calc_matrix_coord_type(coords, opt);
  SEXP mat_ = PROTECT(parse_coords_as_matrix(coords, coord_type, opt));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, 3));
  SET_STRING_ELT(nms_, 0, Rf_mkChar(COORD_SYSTEM[coord_type]));
  SET_STRING_ELT(nms_, 1, Rf_mkChar("LINESTRING"));
  SET_STRING_ELT(nms_, 2, Rf_mkChar("sfg"));
  Rf_setAttrib(mat_, R_ClassSymbol, nms_);
  
  UNPROTECT(2);
  return mat_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  MULTILINESTRING
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_multilinestring(yyjson_val *obj, geo_parse_options *opt) {
  
  yyjson_val *linestrings = yyjson_obj_get(obj, "coordinates");
  
  size_t nlinestrings = yyjson_get_len(linestrings);
  
  SEXP ll_ = PROTECT(Rf_allocVector(VECSXP, (R_xlen_t)nlinestrings)); 
  
  yyjson_arr_iter ring_iter = yyjson_arr_iter_with(linestrings);
  yyjson_val *coords;
  unsigned int ring_idx = 0;
  unsigned int coord_type = COORD_XY;
  while ((coords = yyjson_arr_iter_next(&ring_iter))) {
    
    coord_type = calc_matrix_coord_type(coords, opt);
    SEXP mat_ = PROTECT(parse_coords_as_matrix(coords, coord_type, opt));
    SET_VECTOR_ELT(ll_, ring_idx, mat_);
    
    UNPROTECT(1);
    ring_idx++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, 3)); 
  SET_STRING_ELT(nms_, 0, Rf_mkChar(COORD_SYSTEM[coord_type]));
  SET_STRING_ELT(nms_, 1, Rf_mkChar("MULTILINESTRING"));
  SET_STRING_ELT(nms_, 2, Rf_mkChar("sfg"));
  Rf_setAttrib(ll_, R_ClassSymbol, nms_);
  
  UNPROTECT(2);
  return ll_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  Polygon
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_polygon(yyjson_val *obj, geo_parse_options *opt) {
  
  yyjson_val *coords = yyjson_obj_get(obj, "coordinates");
  
  unsigned int coord_type = COORD_XY;
  SEXP ll_ = PROTECT(parse_coords_as_matrix_list(coords, &coord_type, 
                                                 opt));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, 3)); 
  SET_STRING_ELT(nms_, 0, Rf_mkChar(COORD_SYSTEM[coord_type]));
  SET_STRING_ELT(nms_, 1, Rf_mkChar("POLYGON"));
  SET_STRING_ELT(nms_, 2, Rf_mkChar("sfg"));
  Rf_setAttrib(ll_, R_ClassSymbol, nms_);
  
  UNPROTECT(2);
  return ll_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  MultiPolygon
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_multipolygon(yyjson_val *obj, geo_parse_options *opt) {
  
  yyjson_val *polygons = yyjson_obj_get(obj, "coordinates");
  
  size_t npolygons = yyjson_get_len(polygons);
  
  SEXP ll_ = PROTECT(Rf_allocVector(VECSXP, (R_xlen_t)npolygons)); 
  
  yyjson_arr_iter ring_iter = yyjson_arr_iter_with(polygons);
  yyjson_val *coords;
  unsigned int polygon_idx = 0;
  unsigned int coord_type = COORD_XY;
  while ((coords = yyjson_arr_iter_next(&ring_iter))) {
    
    SEXP inner_ = PROTECT(parse_coords_as_matrix_list(coords, &coord_type, 
                                                      opt));
    SET_VECTOR_ELT(ll_, polygon_idx, inner_);
    
    UNPROTECT(1);
    polygon_idx++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, 3)); 
  SET_STRING_ELT(nms_, 0, Rf_mkChar(COORD_SYSTEM[coord_type]));
  SET_STRING_ELT(nms_, 1, Rf_mkChar("MULTIPOLYGON"));
  SET_STRING_ELT(nms_, 2, Rf_mkChar("sfg"));
  Rf_setAttrib(ll_, R_ClassSymbol, nms_);
  
  UNPROTECT(2);
  return ll_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ####                                       #       #                 
//   #   #                                      #                         
//   #   #  # ##    ###   # ##    ###   # ##   ####    ##     ###    ###  
//   ####   ##  #  #   #  ##  #  #   #  ##  #   #       #    #   #  #     
//   #      #      #   #  ##  #  #####  #       #       #    #####   ###  
//   #      #      #   #  # ##   #      #       #  #    #    #          # 
//   #      #       ###   #       ###   #        ##    ###    ###   ####  
//                        #                                               
//                        #                                               
//
// Parse a scalar feature that is not part of a collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP prop_to_rchar(yyjson_val *prop_val, geo_parse_options *opt) {
  if (prop_val == NULL) {
    return NA_STRING;
  } 
  
  char buf[128] = "";
  static char *bool_str[2] = {"FALSE", "TRUE"};
  static char *bool_int[2] = {"0", "1"};
  
  switch (yyjson_get_type(prop_val)) {
  case YYJSON_TYPE_NULL:
    return NA_STRING;
    break;
  case YYJSON_TYPE_STR:
    return Rf_mkChar(yyjson_get_str(prop_val));
    break;
  case YYJSON_TYPE_BOOL:
  {
    if (opt->property_promotion_lgl == PROP_LGL_AS_INT) {
    int tmp = yyjson_get_bool(prop_val);
    return Rf_mkChar(bool_int[tmp]);
  } else {
    int tmp = yyjson_get_bool(prop_val);
    return Rf_mkChar(bool_str[tmp]);
    }
  }
    break;
  case YYJSON_TYPE_ARR:
  case YYJSON_TYPE_OBJ:
  {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *val = yyjson_val_mut_copy(doc, prop_val);
    yyjson_mut_doc_set_root(doc, val);
    char *json = yyjson_mut_write(doc, 0, NULL);
    if (json == NULL) {
      Rf_error("Error converting json to string in prop_to_strsxp");
    }
    SEXP res_ = PROTECT(Rf_mkChar(json));
    free(json);
    yyjson_mut_doc_free(doc);
    UNPROTECT(1);
    return res_;
  }
    break;
  case YYJSON_TYPE_NUM:
    switch(yyjson_get_subtype(prop_val)) {
    case YYJSON_SUBTYPE_UINT:
#if defined(__APPLE__) || defined(_WIN32)
      snprintf(buf, 128, "%llu", yyjson_get_uint(prop_val));
#else
      snprintf(buf, 128, "%lu", yyjson_get_uint(prop_val));
#endif
      return Rf_mkChar(buf);
      break;
    case YYJSON_SUBTYPE_SINT:
#if defined(__APPLE__) || defined(_WIN32)
      snprintf(buf, 128, "%lld", yyjson_get_sint(prop_val));
#else
      snprintf(buf, 128, "%ld", yyjson_get_sint(prop_val));
#endif
      return Rf_mkChar(buf);
      break;
    case YYJSON_SUBTYPE_REAL:
      snprintf(buf, 128, "%f", yyjson_get_real(prop_val));
      return Rf_mkChar(buf);
      break;
    default:
      Rf_warning("prop_to_strsxp unhandled numeric type %s\n", yyjson_get_type_desc(prop_val));
    }
    break;
  default:
    Rf_warning("prop_to_strsxp unhandled type: %s\n", yyjson_get_type_desc(prop_val));
  return NA_STRING;
  }
  

  Rprintf("Ugh\n");
  return NA_STRING;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse a property as a character string from a feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP prop_to_strsxp(yyjson_val *features, char *prop_name, geo_parse_options *opt) {
  
  size_t N = yyjson_get_len(features);
  SEXP vec_ = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t)N)); 
  
  yyjson_arr_iter feature_iter = yyjson_arr_iter_with(features);
  yyjson_val *feature_obj;
  unsigned int idx = 0;
  while ((feature_obj = yyjson_arr_iter_next(&feature_iter))) {
    yyjson_val *props_obj = yyjson_obj_get(feature_obj, "properties");
    yyjson_val *prop_val = yyjson_obj_get(props_obj, prop_name);

    SET_STRING_ELT(vec_, idx, prop_to_rchar(prop_val, opt));
    idx++;
  }
  
  UNPROTECT(1);
  return vec_;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse a property as a character string from a feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP prop_to_vecsxp(yyjson_val *features, char *prop_name, geo_parse_options *opt) {
  size_t N = yyjson_get_len(features);
  SEXP vec_ = PROTECT(Rf_allocVector(VECSXP, (R_xlen_t)N)); 
  
  yyjson_arr_iter feature_iter = yyjson_arr_iter_with(features);
  yyjson_val *feature_obj;
  unsigned int idx = 0;
  while ((feature_obj = yyjson_arr_iter_next(&feature_iter))) {
    yyjson_val *props_obj = yyjson_obj_get(feature_obj, "properties");
    yyjson_val *prop_val = yyjson_obj_get(props_obj, prop_name);
    if (prop_val == NULL) {
      SET_VECTOR_ELT(vec_, idx, Rf_ScalarLogical(NA_LOGICAL));
    } else {
      SET_VECTOR_ELT(vec_, idx, json_as_robj(prop_val, opt->parse_opt));
    }
    idx++;
  }
  
  UNPROTECT(1);
  return vec_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse a property as a INTSXP from a feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP prop_to_lglsxp(yyjson_val *features, char *prop_name, geo_parse_options *opt) {
  size_t N = yyjson_get_len(features);
  SEXP vec_ = PROTECT(Rf_allocVector(LGLSXP, (R_xlen_t)N)); 
  int *ptr = INTEGER(vec_);
  
  yyjson_arr_iter feature_iter = yyjson_arr_iter_with(features);
  yyjson_val *feature_obj;
  while ((feature_obj = yyjson_arr_iter_next(&feature_iter))) {
    yyjson_val *props_obj = yyjson_obj_get(feature_obj, "properties");
    yyjson_val *prop_val  = yyjson_obj_get(props_obj, prop_name);
    if (prop_val == NULL) {
      *ptr++ = NA_INTEGER;
    } else {
      *ptr++ = yyjson_get_bool(prop_val);
    }
  }
  
  UNPROTECT(1);
  return vec_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse a property as a INTSXP from a feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP prop_to_intsxp(yyjson_val *features, char *prop_name, geo_parse_options *opt) {
  size_t N = yyjson_get_len(features);
  SEXP vec_ = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t)N)); 
  int *ptr = INTEGER(vec_);
  
  yyjson_arr_iter feature_iter = yyjson_arr_iter_with(features);
  yyjson_val *feature_obj;
  while ((feature_obj = yyjson_arr_iter_next(&feature_iter))) {
    yyjson_val *props_obj = yyjson_obj_get(feature_obj, "properties");
    yyjson_val *prop_val  = yyjson_obj_get(props_obj, prop_name);
    if (prop_val == NULL) {
      *ptr++ = NA_INTEGER;
    } else {
      *ptr++ = yyjson_get_int(prop_val);
    }
  }
  
  UNPROTECT(1);
  return vec_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse a property as a REALSXP from a feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP prop_to_realsxp(yyjson_val *features, char *prop_name, geo_parse_options *opt) {
  size_t N = yyjson_get_len(features);
  SEXP vec_ = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t)N)); 
  double *ptr = REAL(vec_);
  
  yyjson_arr_iter feature_iter = yyjson_arr_iter_with(features);
  yyjson_val *feature_obj;
  while ((feature_obj = yyjson_arr_iter_next(&feature_iter))) {
    yyjson_val *props_obj = yyjson_obj_get(feature_obj, "properties");
    yyjson_val *prop_val  = yyjson_obj_get(props_obj, prop_name);
    if (prop_val == NULL) {
      *ptr++ = NA_REAL;
    } else {
      *ptr++ = yyjson_get_num(prop_val);
    }
  }
  
  UNPROTECT(1);
  return vec_;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Forward declaration
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_geometry_type(yyjson_val *val, geo_parse_options *opt);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse a feature
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_feature(yyjson_val *obj, geo_parse_options *opt) {
  // Rprintf(">>> Feature\n");
  
  reset_bbox(opt);
  int nprotect = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse GEOMETRY
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_val *geom  = yyjson_obj_get(obj, "geometry");
  SEXP geom_col_ = PROTECT(Rf_allocVector(VECSXP, 1)); nprotect++;
  SEXP geom_ = PROTECT(parse_geometry_type(geom, opt)); nprotect++;
  SET_VECTOR_ELT(geom_col_, 0, geom_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set attributes on geometry 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Rf_setAttrib(geom_col_, Rf_mkString("n_empty")  , Rf_ScalarInteger(0));
  Rf_setAttrib(geom_col_, Rf_mkString("crs")      , make_crs());
  
  SEXP geom_class_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  
  
  yyjson_val *geom_type = yyjson_obj_get(geom, "type");
  // Rprintf("parse_geometry_type(): %s\n", yyjson_get_str(geom_type));
  
  if (yyjson_equals_str(geom_type, "Point")) {
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_POINT"));
  } else if (yyjson_equals_str(geom_type, "MultiPoint")) {
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_MULTIPOINT"));
  } else if (yyjson_equals_str(geom_type, "LineString")) {
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_LINESTRING"));
  } else if (yyjson_equals_str(geom_type, "MultiLineString")) {
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_MULTILINESTRING"));
  } else if (yyjson_equals_str(geom_type, "Polygon")) {
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_POLYGON"));
  } else if (yyjson_equals_str(geom_type, "MultiPolygon")) {
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_MULTIPOLYGON"));
  } else {
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_UNKNOWN"));
  }
  
  SET_STRING_ELT(geom_class_, 1, Rf_mkChar("sfc"));
  Rf_setAttrib(geom_col_, R_ClassSymbol, geom_class_);
  
  Rf_setAttrib(geom_col_, Rf_mkString("precision"), Rf_ScalarReal(0));
  Rf_setAttrib(geom_col_, Rf_mkString("bbox"), make_bbox(opt));
  
  if (opt->type == SFC_TYPE) {
    // only care about geom, not properties.
    UNPROTECT(nprotect);
    return(geom_col_);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Properties
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_val *props = yyjson_obj_get(obj, "properties");
  size_t ncols = yyjson_get_len(props) + 1;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate space for data.frame columns and column names
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(Rf_allocVector(VECSXP, (R_xlen_t)ncols)); nprotect++;
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t)ncols)); nprotect++;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over the keys/values "properties"
  //   keys become column names
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_obj_iter iter = yyjson_obj_iter_with(props);
  yyjson_val *val, *key;
  unsigned int idx = 0;
  while ((key = yyjson_obj_iter_next(&iter))) {
    val = yyjson_obj_iter_get_val(key);
    SEXP robj_ = PROTECT(json_as_robj(val, opt->parse_opt)); 
    
    if (Rf_isNull(robj_)) {
      // compatibilty with geojson: promotes NULL values to NA_character_
      UNPROTECT(1);
      robj_ = PROTECT(Rf_allocVector(STRSXP, 1));
      SET_STRING_ELT(robj_, 0, NA_STRING);
    } else if (Rf_isNewList(robj_) && Rf_length(robj_) == 0) {
      // compatibilty with geojson. promote empty list to "{}"
      UNPROTECT(1);
      robj_ = PROTECT(Rf_allocVector(STRSXP, 1));
      SET_STRING_ELT(robj_, 0, Rf_mkChar("{}"));
    } else if (Rf_isNewList(robj_) && opt->property_promotion == PROP_TYPE_STRING) {
      // this is either a list or a multi-element {}-objet or []-array
      // so turn it back into a string
      UNPROTECT(1);
      robj_ = PROTECT(Rf_allocVector(STRSXP, 1));
      SET_STRING_ELT(robj_, 0, prop_to_rchar(val, opt));
    } else if (Rf_length(robj_) > 1 && opt->property_promotion == PROP_TYPE_STRING) {
      // this is either a list or a multi-element {}-objet or []-array
      // so turn it back into a string
      UNPROTECT(1);
      robj_ = PROTECT(Rf_allocVector(STRSXP, 1));
      SET_STRING_ELT(robj_, 0, prop_to_rchar(val, opt));
    }
    
    SET_VECTOR_ELT(res_, idx, robj_);
    UNPROTECT(1);
    SET_STRING_ELT(nms_, idx, Rf_mkChar(yyjson_get_str(key)));
    idx++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // The geometry column should be a list (i.e. VECSXP)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SET_VECTOR_ELT(res_, (R_xlen_t)ncols - 1, geom_col_);
  SET_STRING_ELT(nms_, (R_xlen_t)ncols - 1, Rf_mkChar("geometry"));
  
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set column names
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Rf_setAttrib(res_, R_NamesSymbol, nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Row.names
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rownames = PROTECT(Rf_allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -1); // only a single rows
  Rf_setAttrib(res_, R_RowNamesSymbol, rownames);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Nominate the geometry column name. 
  // Store this as the 'sf_geometry' attribute
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP sf_name_ = PROTECT(Rf_mkString("geometry")); nprotect++;
  Rf_setAttrib(res_, Rf_mkString("sf_column"), sf_name_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 'data.frame' class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP df_names_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  SET_STRING_ELT(df_names_, 0, Rf_mkChar("sf"));
  SET_STRING_ELT(df_names_, 1, Rf_mkChar("data.frame"));
  Rf_setAttrib(res_, R_ClassSymbol, df_names_);
  
  
  
  UNPROTECT(nprotect);
  return res_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Accumulating types and names for properies should really be dynamic, 
// but for first draft, just set a static upper limit on number of properiies.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define MAX_PROPS 256

//===========================================================================
//  #####                 #                            ###           ##     ##   
//  #                     #                           #   #           #      #   
//  #       ###    ###   ####   #   #  # ##    ###    #       ###     #      #   
//  ####   #   #      #   #     #   #  ##  #  #   #   #      #   #    #      #   
//  #      #####   ####   #     #   #  #      #####   #      #   #    #      #   
//  #      #      #   #   #  #  #  ##  #      #       #   #  #   #    #      #   
//  #       ###    ####    ##    ## #  #       ###     ###    ###    ###    ### 
//===========================================================================

SEXP parse_feature_collection_geometry(yyjson_val *features, geo_parse_options *opt) {
  
  int nprotect = 0;
  reset_bbox(opt);
  
  if (!yyjson_is_arr(features)) {
    Rf_error("Expecting FeatureCollection::features to be an array");
  }
  size_t nrows = yyjson_get_len(features);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // List-column will be used for geometry
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geom_col_     = PROTECT(Rf_allocVector(VECSXP, (R_xlen_t)nrows)); nprotect++;
  SEXP geom_classes_ = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t)nrows)); nprotect++;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over array to gather geometry
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter feature_iter = yyjson_arr_iter_with(features);
  yyjson_val *feature;
  unsigned int feature_idx = 0;
  unsigned int sf_type_bitset = 0;
  while ((feature = yyjson_arr_iter_next(&feature_iter))) {
    
    yyjson_val *geom = yyjson_obj_get(feature, "geometry");
    SET_VECTOR_ELT(geom_col_, feature_idx, geojson_as_sf(geom, opt, 1));
    
    yyjson_val *geom_type = yyjson_obj_get(geom, "type");
    
    if (yyjson_equals_str(geom_type, "Point")) {
      sf_type_bitset |= SF_POINT;
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("POINT"));
    } else if (yyjson_equals_str(geom_type, "MultiPoint")) {
      sf_type_bitset |= SF_MULTIPOINT;
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("MULTIPOINT"));
    } else if (yyjson_equals_str(geom_type, "LineString")) {
      sf_type_bitset |= SF_LINESTRING;
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("LINESTRING"));
    } else if (yyjson_equals_str(geom_type, "MultiLineString")) {
      sf_type_bitset |= SF_MULTILINESTRING;
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("MULTILINESTRING"));
    } else if (yyjson_equals_str(geom_type, "Polygon")) {
      sf_type_bitset |= SF_POLYGON;
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("POLYGON"));
    } else if (yyjson_equals_str(geom_type, "MultiPolygon")) {
      sf_type_bitset |= SF_MULTIPOLYGON;
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("MULTIPOLYGON"));
    } else if (yyjson_equals_str(geom_type, "GeometryCollection")) {
      sf_type_bitset |= SF_GEOMETRY_COLLECTION;
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("GEOMETRYCOLLECTION"));
    } else {
      SET_STRING_ELT(geom_classes_, feature_idx, Rf_mkChar("UNKNOWN"));
    }
    
    feature_idx++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set attributes on geometry
  //
  // According to: https://r-spatial.github.io/sf/reference/sfc.html
  // if all classes are the same, then:
  //    1. Omit the 'classes' attribute
  //    2. set the class to sfc_TYPE
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geom_class_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  
  switch(sf_type_bitset) {
  case SF_POINT:
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_POINT"));
    break;
  case SF_MULTIPOINT:
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_MULTIPOINT"));
    break;
  case SF_LINESTRING:
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_LINESTRING"));
    break;
  case SF_MULTILINESTRING:
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_MULTILINESTRING"));
    break;
  case SF_POLYGON:
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_POLYGON"));
    break;
  case SF_MULTIPOLYGON:
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_MULTIPOLYGON"));
    break;
  default:
    if (nrows > 0) {
      Rf_setAttrib(geom_col_, Rf_mkString("classes")  , geom_classes_);
    }
    SET_STRING_ELT(geom_class_, 0, Rf_mkChar("sfc_GEOMETRY"));
  }
  
  
  Rf_setAttrib(geom_col_, Rf_mkString("n_empty")  , Rf_ScalarInteger(0));
  Rf_setAttrib(geom_col_, Rf_mkString("crs")      , make_crs());
  
  SET_STRING_ELT(geom_class_, 1, Rf_mkChar("sfc"));
  Rf_setAttrib(geom_col_, R_ClassSymbol, geom_class_);
  
  Rf_setAttrib(geom_col_, Rf_mkString("precision"), Rf_ScalarReal(0));
  Rf_setAttrib(geom_col_, Rf_mkString("bbox"), make_bbox(opt));
  
  if (needs_z_range(opt)) {
    Rf_setAttrib(geom_col_, Rf_mkString("z_range"), make_z_range(opt));
  }
  
  if (needs_m_range(opt)) {
    Rf_setAttrib(geom_col_, Rf_mkString("m_range"), make_m_range(opt));
  }
  
  UNPROTECT(nprotect);
  return geom_col_;
}





//===========================================================================
// 
//===========================================================================
SEXP parse_feature_collection(yyjson_val *obj, geo_parse_options *opt) {
  
  int nprotect = 0;
  
  yyjson_val *features = NULL;
  
  if (yyjson_is_obj(obj)) {
    // This is an official geojson FeatureCollection object
    features = yyjson_obj_get(obj, "features");
  } else if (yyjson_is_arr(obj)) {
    // This is just a JSON []-array with multiple features in it.
    features  = obj;
  } else {    
    Rf_error("parse_feature_collection() obj not array or object, but %s", yyjson_get_type_desc(obj));
  }
  
  if (!yyjson_is_arr(features)) {
    Rf_error("Expecting FeatureCollection::features to be an array. Got %s", yyjson_get_type_desc(features));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the geometry
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geom_col_ = PROTECT(parse_feature_collection_geometry(features, opt)); nprotect++;
  if (opt->type == SFC_TYPE) {
    // we onlyl care about geometry! Don't worry about parsing properties
    UNPROTECT(nprotect);
    return geom_col_;
  }
  
  size_t nrows = yyjson_get_len(features);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over all features to determine full set of properties
  // and their types
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *prop_names[MAX_PROPS];
  unsigned int type_bitset[MAX_PROPS] = {0};
   int nprops = 0;
  
  yyjson_arr_iter feature_iter = yyjson_arr_iter_with(features);
  yyjson_val *feature;
  while ((feature = yyjson_arr_iter_next(&feature_iter))) {
    
    yyjson_val *props = yyjson_obj_get(feature, "properties");
    
    yyjson_obj_iter prop_iter = yyjson_obj_iter_with(props);
    yyjson_val *prop_name, *prop_val;
    while ((prop_name = yyjson_obj_iter_next(&prop_iter))) {
      prop_val = yyjson_obj_iter_get_val(prop_name);
      
      int name_idx = -1;
      for (int i = 0; i < nprops; i++) {
        if (yyjson_equals_str(prop_name, prop_names[i])) {
          name_idx = i;
          break;
        }
      }
      if (name_idx < 0) {
        // Name has not been seen yet. so add it.
        name_idx = nprops;
        prop_names[nprops] = (char *)yyjson_get_str(prop_name);
        // Rprintf("Key: %s\n", yyjson_get_str(prop_name));
        nprops++;
        if (nprops == MAX_PROPS) {
          Rf_error("Maximum properies exceeded parsing feature collection: %i", MAX_PROPS);
        }
      }
      
      type_bitset[name_idx] = update_type_bitset(type_bitset[name_idx], prop_val, opt->parse_opt);
    }
  }
  
  // for (unsigned int i=0; i<nprops; i++) {
  //   unsigned int sexp_type = get_best_sexp_to_represent_type_bitset(type_bitset[i], opt->parse_opt);
  //   dump_type_bitset(type_bitset[i]);
  //   Rprintf("[prop %i] %s - sexp_type: %i -> %s\n",
  //           i, prop_names[i],
  //           sexp_type, Rf_type2char(sexp_type));
  // }
  
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a data.frame with:
  //   prop1, prop2, prop3.... propN, geometry
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP df_ = PROTECT(Rf_allocVector(VECSXP, nprops + 1)); nprotect++;
  SET_VECTOR_ELT(df_, nprops, geom_col_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For each property 
  //   - determine the best SEXP to represent the 'type_bitset'
  //   - Call a parse function which will
  //        - loop through the entire FeatureCollection[]-array, 
  //            plucking the property value from each {}-object
  //        - return an atomic vector or a list
  //   - place this vector as a column in the data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (unsigned int idx = 0; idx < nprops; idx++) {
    
    unsigned int sexp_type = get_best_sexp_to_represent_type_bitset(type_bitset[idx], opt->parse_opt);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check if user requested the "maximum" container type for properties
    // to be a string, rather than a list()
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (sexp_type == VECSXP && opt->property_promotion == PROP_TYPE_STRING) {
      sexp_type = STRSXP;
    }
    
    switch (sexp_type) {
    case LGLSXP:
      SET_VECTOR_ELT(df_, idx, prop_to_lglsxp(features, prop_names[idx], opt));
      break;
    case INTSXP:
      SET_VECTOR_ELT(df_, idx, prop_to_intsxp(features, prop_names[idx], opt));
      break;
    case REALSXP:
      SET_VECTOR_ELT(df_, idx, prop_to_realsxp(features, prop_names[idx], opt));
      break;
    case STRSXP:
      SET_VECTOR_ELT(df_, idx, prop_to_strsxp(features, prop_names[idx], opt));
      break;
    case VECSXP:
      SET_VECTOR_ELT(df_, idx, prop_to_vecsxp(features, prop_names[idx], opt));
      break;
    default:
      Rf_warning("Unhandled 'prop' coltype: %i -> %s\n", sexp_type, Rf_type2char(sexp_type));
    SET_VECTOR_ELT(df_, idx, Rf_allocVector(LGLSXP, (R_xlen_t)nrows));
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Nominate the geometry column name. 
  // Store this as the 'sf_geometry' attribute
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP sf_name_ = PROTECT(Rf_mkString("geometry")); nprotect++;
  Rf_setAttrib(df_, Rf_mkString("sf_column"), sf_name_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set colnames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, nprops + 1)); nprotect++;
  for (unsigned int i = 0; i < nprops; i++) {
    SET_STRING_ELT(nms_, i, Rf_mkChar(prop_names[i]));
  }
  SET_STRING_ELT(nms_, nprops, Rf_mkChar("geometry"));
  Rf_setAttrib(df_, R_NamesSymbol, nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set rownames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rownames = PROTECT(Rf_allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -(int)nrows);
  Rf_setAttrib(df_, R_RowNamesSymbol, rownames);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 'data.frame' class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP df_class_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  SET_STRING_ELT(df_class_, 0, Rf_mkChar("sf"));
  SET_STRING_ELT(df_class_, 1, Rf_mkChar("data.frame"));
  Rf_setAttrib(df_, R_ClassSymbol, df_class_);
  
  UNPROTECT(nprotect);
  return df_;
}


//===========================================================================
// 
//===========================================================================
SEXP promote_bare_geometry_to_list(SEXP geom_, yyjson_val *val, geo_parse_options *opt) {
  int nprotect = 0;
  
  SEXP geom_col_ = PROTECT(Rf_allocVector(VECSXP, 1)); nprotect++;
  SET_VECTOR_ELT(geom_col_, 0, geom_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Figure out type of geometry
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geom_col_class_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  
  if (!yyjson_is_obj(val)) {
    Rf_error("promote_bare_geometry_to_list(): Expecting object. Got %s", yyjson_get_type_desc(val));
  }
  
  yyjson_val *type = yyjson_obj_get(val, "type");
  if (type == NULL) {
    Rf_error("parse_geometry(): type == NULL");
  }
  
  if (yyjson_equals_str(type, "Point")) {
    SET_STRING_ELT(geom_col_class_, 0, Rf_mkChar("sfc_POINT"));
  } else if (yyjson_equals_str(type, "MultiPoint")) {
    SET_STRING_ELT(geom_col_class_, 0, Rf_mkChar("sfc_MULTIPOINT"));
  } else if (yyjson_equals_str(type, "LineString")) {
    SET_STRING_ELT(geom_col_class_, 0, Rf_mkChar("sfc_LINESTRING"));
  } else if (yyjson_equals_str(type, "MultiLineString")) {
    SET_STRING_ELT(geom_col_class_, 0, Rf_mkChar("sfc_MULTILINESTRING"));
  } else if (yyjson_equals_str(type, "Polygon")) {
    SET_STRING_ELT(geom_col_class_, 0, Rf_mkChar("sfc_POLYGON"));
  } else if (yyjson_equals_str(type, "MultiPolygon")) {
    SET_STRING_ELT(geom_col_class_, 0, Rf_mkChar("sfc_MULTIPOLYGON"));
  } else if (yyjson_equals_str(type, "GeometryCollection")) {
    SET_STRING_ELT(geom_col_class_, 0, Rf_mkChar("sfc_GEOMETRY"));
    Rf_setAttrib(geom_col_, Rf_mkString("classes")  , Rf_mkString("GEOMETRYCOLLECTION"));
  } else {
    Rf_error("promote_bare_geometry_to_list(): Unknown geojson type: %s", yyjson_get_str(type));
  }
  
  SET_STRING_ELT(geom_col_class_, 1, Rf_mkChar("sfc"));
  Rf_setAttrib(geom_col_, R_ClassSymbol, geom_col_class_);
  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set attributes on geometry 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Rf_setAttrib(geom_col_, Rf_mkString("n_empty")  , Rf_ScalarInteger(0));
  Rf_setAttrib(geom_col_, Rf_mkString("crs")      , make_crs());
  
  Rf_setAttrib(geom_col_, Rf_mkString("precision"), Rf_ScalarReal(0));
  Rf_setAttrib(geom_col_, Rf_mkString("bbox"), make_bbox(opt));
  
  if (needs_z_range(opt)) {
    Rf_setAttrib(geom_col_, Rf_mkString("z_range"), make_z_range(opt));
  }
  
  if (needs_m_range(opt)) {
    Rf_setAttrib(geom_col_, Rf_mkString("m_range"), make_m_range(opt));
  }\
  
  UNPROTECT(nprotect);
  return geom_col_;
}


//===========================================================================
// 
//===========================================================================
SEXP promote_bare_geometry_to_df(SEXP geom_, yyjson_val *val, geo_parse_options *opt) {
  
  int nprotect = 0;
  
  // Setup data.frame with single column called 'geometry'
  // 'geometry' is a list column
  SEXP df_ = PROTECT(Rf_allocVector(VECSXP, 1)); nprotect++;
  SET_VECTOR_ELT(df_, 0, promote_bare_geometry_to_list(geom_, val, opt));
  Rf_setAttrib(df_, R_NamesSymbol, Rf_mkString("geometry"));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set NULL rownames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rownames = PROTECT(Rf_allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -1);
  Rf_setAttrib(df_, R_RowNamesSymbol, rownames);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set class: 'sf' + 'data.frame'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP df_class_ = PROTECT(Rf_allocVector(STRSXP, 2)); nprotect++;
  SET_STRING_ELT(df_class_, 0, Rf_mkChar("sf"));
  SET_STRING_ELT(df_class_, 1, Rf_mkChar("data.frame"));
  Rf_setAttrib(df_, R_ClassSymbol, df_class_);
  
  // Attributes for {sf}
  Rf_setAttrib(df_, Rf_mkString("sf_column"), Rf_mkString("geometry"));
  
  UNPROTECT(nprotect);
  return df_;
}



//===========================================================================
//    ###                                ###           ##     ##   
//   #   #                              #   #           #      #   
//   #       ###    ###   ## #          #       ###     #      #   
//   #      #   #  #   #  # # #         #      #   #    #      #   
//   #  ##  #####  #   #  # # #         #      #   #    #      #   
//   #   #  #      #   #  # # #         #   #  #   #    #      #   
//    ###    ###    ###   #   #          ###    ###    ###    ###  
//
// Geometry Collection 
//===========================================================================
SEXP parse_geometry_collection(yyjson_val *obj, geo_parse_options *opt) {
  
  int nprotect = 0;
  reset_bbox(opt);
  
  yyjson_val *geoms = yyjson_obj_get(obj, "geometries");
  if (!yyjson_is_arr(geoms)) {
    Rf_error("Expecting GeomCollection::geometries to be an array. not %s", 
          yyjson_get_type_desc(geoms));
  }
  size_t ngeoms = yyjson_get_len(geoms);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // List-column will be used for geometry
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geoms_ = PROTECT(Rf_allocVector(VECSXP, (R_xlen_t)ngeoms)); nprotect++;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over array to gather geometry
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_arr_iter geom_iter = yyjson_arr_iter_with(geoms);
  yyjson_val *geom;
  unsigned int geom_idx = 0;
  while ((geom = yyjson_arr_iter_next(&geom_iter))) {
    
    SEXP geom_ = PROTECT(geojson_as_sf(geom, opt, 1)); 
    SET_VECTOR_ELT(geoms_, geom_idx, geom_);
    
    UNPROTECT(1);
    geom_idx++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set attributes on geometry 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geom_class_ = PROTECT(Rf_allocVector(STRSXP, 3)); nprotect++;
  SET_STRING_ELT(geom_class_, 0, Rf_mkChar("XY"));
  SET_STRING_ELT(geom_class_, 1, Rf_mkChar("GEOMETRYCOLLECTION"));
  SET_STRING_ELT(geom_class_, 2, Rf_mkChar("sfg"));
  Rf_setAttrib(geoms_, R_ClassSymbol, geom_class_);
  
  UNPROTECT(nprotect);
  return geoms_;
}

//===========================================================================
// Parse geometry
//===========================================================================
SEXP parse_geometry_type(yyjson_val *val, geo_parse_options *opt) {
  
  if (!yyjson_is_obj(val)) {
    Rf_error("parse_geometry(): Expecting object. Got %s", yyjson_get_type_desc(val));
  }
  
  yyjson_val *type = yyjson_obj_get(val, "type");
  if (type == NULL) {
    Rf_error("parse_geometry(): type == NULL");
  }
  
  if (yyjson_equals_str(type, "Point")) {
    return parse_point(val, opt); 
  } else if (yyjson_equals_str(type, "MultiPoint")) {
    return parse_multipoint(val, opt);
  } else if (yyjson_equals_str(type, "LineString")) {
    return parse_linestring(val, opt);
  } else if (yyjson_equals_str(type, "MultiLineString")) {
    return parse_multilinestring(val, opt);
  } else if (yyjson_equals_str(type, "Polygon")) {
    return parse_polygon(val, opt);
  } else if (yyjson_equals_str(type, "MultiPolygon")) {
    return parse_multipolygon(val, opt);
  } else if (yyjson_equals_str(type, "GeometryCollection")) {
    return parse_geometry_collection(val, opt);
  } else {
    Rf_error("parse_geometry(): Unknown geojson type: %s", yyjson_get_str(type));
  }
}


//===========================================================================
// 
//===========================================================================
SEXP geojson_as_sf(yyjson_val *val, geo_parse_options *opt, unsigned int depth) {
  
  if (yyjson_is_arr(val)) {
    // Assume this is a JSON []-array of features  
    return parse_feature_collection(val, opt);
  }
  
  if (!yyjson_is_obj(val)) {
    Rf_error("geojson_as_sf(): Expecting object. Got %s", yyjson_get_type_desc(val));
  }
  
  yyjson_val *type = yyjson_obj_get(val, "type");
  if (type == NULL) {
    Rf_error("geojson_as_sf(): type == NULL");
  }
  
  if (yyjson_equals_str(type, "Feature")) {
    return parse_feature(val, opt);
  } else if (yyjson_equals_str(type, "FeatureCollection")) {
    return parse_feature_collection(val, opt);
  } else {
    int nprotect = 0;
    SEXP res_= PROTECT(parse_geometry_type(val, opt)); nprotect++;
    
    // If this is not a top-level object, then just return it.
    // the calling function will wrap it in a data.frame or whatever
    if (depth > 0) {
      UNPROTECT(nprotect);
      return res_;
    }
    
    // This is top-level bare geometry! 
    // This must be a single POINT/MULTIPOINT/LINESTRING/MULTILINSTRING
    //  POLYGON/MULTIPOLYGON/GEOMETRYCOLLECTION
    if (opt->type == SF_TYPE) {
      // Return as a df
      res_ = PROTECT(promote_bare_geometry_to_df(res_, val, opt)); nprotect++;
    } else {
      // opt->type == SFC_TYPE
      // return as a list
      res_ = PROTECT(promote_bare_geometry_to_list(res_, val, opt)); nprotect++;
    }
    
    UNPROTECT(nprotect);
    return res_;
  }
  
  return R_NilValue;
}






//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse GeoJSON from a string
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_geojson_str_(SEXP str_, SEXP geo_opts_, SEXP parse_opts_) {
  
  geo_parse_options opt = create_geo_parse_options(geo_opts_);
  
  parse_options parse_opt = create_parse_options(parse_opts_);
  
  opt.parse_opt = &parse_opt;
  opt.yyjson_read_flag |= YYJSON_READ_STOP_WHEN_DONE;
  
  char *str  = (char *)CHAR(STRING_ELT(str_, 0));
  
  yyjson_read_err err;
  state_t state = { 0 };
  state.doc = yyjson_read_opts((char *)str, strlen(str), opt.yyjson_read_flag, NULL, &err);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If doc is NULL, then an error occurred during parsing.
  // Try and print something sensible like {jsonlite} does.  
  // I.e.
  //   - print some context around where the error occurred
  //   - print the index in the character string where the error occurred
  //   - add a visual pointer to the output so the user knows where this was
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (state.doc == NULL) {
    output_verbose_error(str, err);
    Rf_error("Error parsing JSON [Loc: %ld]: %s", (long)err.pos, err.msg);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the document from the root node
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(geojson_as_sf(yyjson_doc_get_root(state.doc), &opt, 0));
  
  free_state(&state);
  UNPROTECT(1);
  return res_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse GeoJSON from a string
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP parse_geojson_file_(SEXP filename_, SEXP geo_opts_, SEXP parse_opts_) {
  
  geo_parse_options opt = create_geo_parse_options(geo_opts_);
  
  parse_options parse_opt = create_parse_options(parse_opts_);
  opt.parse_opt = &parse_opt;
  
  const char *filename = (const char *)CHAR( STRING_ELT(filename_, 0) );
  filename = R_ExpandFileName(filename);
  yyjson_read_err err;
  state_t state = { 0 };
  state.doc = yyjson_read_file((char *)filename, opt.yyjson_read_flag, NULL, &err);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If doc is NULL, then an error occurred during parsing.
  // Try and print something sensible like {jsonlite} does.  
  // I.e.
  //   - print some context around where the error occurred
  //   - print the index in the character string where the error occurred
  //   - add a visual pointer to the output so the user knows where this was
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (state.doc == NULL) {
    Rf_error("Error parsing JSON file '%s' [Loc %ld]: %s", 
          filename, (long)err.pos, err.msg);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse the document from the root node
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(geojson_as_sf(yyjson_doc_get_root(state.doc), &opt, 0));
  
  free_state(&state);
  UNPROTECT(1);
  return res_;
}















