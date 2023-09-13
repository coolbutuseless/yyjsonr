


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
// GeoJSON Serialize options
//  - this also contains state information i.e.
//      - current bounding box accumulations
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  unsigned int yyjson_write_flag;
  serialize_options *serialize_opt;
} geo_serialize_options;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise geo_parse_opts from an R named list from the user
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
geo_serialize_options create_geo_serialize_options(SEXP to_geo_opts_) {
  geo_serialize_options opt = {
    .yyjson_write_flag = YYJSON_WRITE_PRETTY,
  };
  
  if (isNull(to_geo_opts_) || length(to_geo_opts_) == 0) {
    return opt;
  }
  
  if (!isNewList(to_geo_opts_)) {
    error("'to_geo_opts_' must be a list");
  }
  
  SEXP nms_ = getAttrib(to_geo_opts_, R_NamesSymbol);
  if (isNull(nms_)) {
    error("'to_geo_opts_' must be a named list");
  }
  
  for (unsigned int i = 0; i < length(to_geo_opts_); i++) {
    // const char *opt_name = CHAR(STRING_ELT(nms_, i));
    // SEXP val_ = VECTOR_ELT(to_geo_opts_, i);
    
    // if (strcmp(opt_name, "property_promotion") == 0) {
    //   const char *val = CHAR(STRING_ELT(val_, 0));
    //   opt.property_promotion = strcmp(val, "string") == 0 ? PROP_TYPE_STRING : PROP_TYPE_LIST;
    // } else if (strcmp(opt_name, "property_promotion_lgl_as_int") == 0) {
    //   const char *val = CHAR(STRING_ELT(val_, 0));
    //   opt.property_promotion_lgl_as_int = strcmp(val, "string") == 0 ? PROP_LGL_AS_STR : PROP_LGL_AS_INT;
    // } else if (strcmp(opt_name, "type") == 0) {
    //   const char *val = CHAR(STRING_ELT(val_, 0));
    //   opt.type = strcmp(val, "sf") == 0 ? SF_TYPE : SFC_TYPE;
    // } else {
    //   warning("geo_opt: Unknown option ignored: '%s'\n", opt_name);
    // }
  }
  return opt;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize geometry
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *serialize_geom(SEXP sf_, yyjson_mut_doc *doc, geo_serialize_options *opt) {
  
  yyjson_mut_val *obj = yyjson_mut_obj(doc);
  
  bool geom_collection = false;
  
  if (inherits(sf_, "POINT")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "Point");
  } else if (inherits(sf_, "MULTIPOINT")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "MultiPoint");
  } else if (inherits(sf_, "LINESTRING")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "LineString");
  } else if (inherits(sf_, "MULTILINESTRING")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "MultiLineString");
  } else if (inherits(sf_, "POLYGON")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "Polygon");
  } else if (inherits(sf_, "MULTIPOLYGON")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "MultiPolygon");
  } else if (inherits(sf_, "GEOMETRYCOLLECTION")) {
    geom_collection = true;
    yyjson_mut_obj_add_str(doc, obj, "type", "GeometryCollection");
  } else {
    error("@@@@@@@ serialize_geom Issue. Unhandled geometry type\n");
  }
  
  
  if (!geom_collection) {
    yyjson_mut_val *key = yyjson_mut_str(doc, "coordinates");
    yyjson_mut_obj_add(obj, key, serialize_core(sf_, doc, opt->serialize_opt));
  } else{
    
    if (!isNewList(sf_)) {
      error("Expecting geomcollection to be a VECSXP not: %s", type2char(TYPEOF(sf_)));
    }
    
    // An array of geoms
    yyjson_mut_val *geoms = yyjson_mut_arr(doc);
    
    // Unpack each element of the 'sf_' list as a geom and add to the array
    for (unsigned int i = 0; i < length(sf_); i++) {
      yyjson_mut_val *geom = serialize_geom(VECTOR_ELT(sf_, i), doc, opt);
      yyjson_mut_arr_add_val(geoms, geom);
    }
    
    yyjson_mut_val *key = yyjson_mut_str(doc, "geometries");
    yyjson_mut_obj_add(obj, key, geoms);
  }
  
  
  return obj;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP sfc_to_str(SEXP sfc_, geo_serialize_options *opt) {
  
  unsigned int nprotect = 0;
  if (!isNewList(sfc_)) {
    error("serialize_sfc(): Expeting list\n");
  }
  
  
  unsigned int N = length(sfc_);
  SEXP geojson_ = PROTECT(allocVector(STRSXP, N)); nprotect++;
  
  for (unsigned int idx = 0; idx < N; idx++) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    SEXP elem_ = VECTOR_ELT(sfc_, idx);
    yyjson_mut_val *val = serialize_geom(elem_, doc, opt);
    yyjson_mut_doc_set_root(doc, val);
    
    yyjson_write_err err;
    char *json = yyjson_mut_write_opts(doc, opt->yyjson_write_flag, NULL, NULL, &err);
    if (json == NULL) {
      yyjson_mut_doc_free(doc);
      error("Write to string error: %s code: %u\n", err.msg, err.code);
    }
    
    SET_STRING_ELT(geojson_, idx, mkChar(json));
    yyjson_mut_doc_free(doc);
  }
  
  
  UNPROTECT(nprotect++);
  return geojson_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sf data.frame to feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_doc *sf_to_json(SEXP sf_, geo_serialize_options *opt) {

  // unsigned int nprotect = 0;
  if (!isNewList(sf_) || !inherits(sf_, "data.frame")) {
    error("serialize_sf(): Expecting data.frame\n");
  }

  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  // int ncol = length(sf_);
  int nrow = length(VECTOR_ELT(sf_, 0));
  // Rprintf("[%i, %i] data.frame\n", nrow, ncol);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Determine index of geometry collection
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int geom_col_idx = -1;
  SEXP geom_col_name_ = getAttrib(sf_, mkString("sf_column"));
  if (isNull(geom_col_name_)) {
    error("sf_to_str(): Couldn't determine 'sf_column' name");
  }
  const char *geom_col_name = CHAR(STRING_ELT(geom_col_name_, 0));
  SEXP colnames_ = getAttrib(sf_, R_NamesSymbol);
  for (unsigned int i = 0; i < length(colnames_); i++) {
    if (strcmp(CHAR(STRING_ELT(colnames_, i)), geom_col_name) == 0) {
      geom_col_idx = i;
      break;
    }
  }
  if (geom_col_idx == -1) {
    error("sf_to_str(): Couldn't 'sf_column' name '%s' in column names of sf object", geom_col_name);
  }
  
  SEXP geom_col_ = VECTOR_ELT(sf_, geom_col_idx);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Iterate over each row to 
  //    - create a 'Feature' object 
  //    - add it to the features JSON []-array
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_val *features = yyjson_mut_arr(doc);
  unsigned int *col_type = detect_data_frame_types(sf_, opt->serialize_opt);
  
  for (unsigned int row = 0; row < nrow; row++) {
    yyjson_mut_val *properties = data_frame_row_to_json_object(sf_, col_type, row, geom_col_idx, doc, opt->serialize_opt);
    yyjson_mut_val *geometry = serialize_geom(VECTOR_ELT(geom_col_, row), doc, opt);
    yyjson_mut_val *feature = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_str(doc, feature, "type", "Feature");
    yyjson_mut_obj_add(feature, yyjson_mut_str(doc, "properties"), properties);
    yyjson_mut_obj_add(feature, yyjson_mut_str(doc, "geometry"  ), geometry);
    
    // Add the feature to the array of features
    yyjson_mut_arr_add_val(features, feature);
  }

  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Feature collection
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_val *fc = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_str(doc, fc, "type", "FeatureCollection");
  yyjson_mut_obj_add(fc, yyjson_mut_str(doc, "features"), features);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Add the feature collection as the root element of the JSON 'doc'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_mut_doc_set_root(doc, fc);
  
  return doc;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sf data.frame to feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP sf_to_str(SEXP sf_, geo_serialize_options *opt) {
  
  unsigned int nprotect = 0;
  yyjson_mut_doc *doc = sf_to_json(sf_, opt);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write the doc to a string
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_write_err err;
  char *json = yyjson_mut_write_opts(doc, opt->yyjson_write_flag, NULL, NULL, &err);
  if (json == NULL) {
    yyjson_mut_doc_free(doc);
    error("serialize_sf() Write to string error: %s code: %u\n", err.msg, err.code);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Convert string to R character, tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geojson_ = PROTECT(mkString(json)); nprotect++;
  yyjson_mut_doc_free(doc);
  UNPROTECT(nprotect++);
  return geojson_;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sf data.frame to feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP sf_to_file(SEXP sf_, SEXP filename_, geo_serialize_options *opt) {
  
  yyjson_mut_doc *doc = sf_to_json(sf_, opt);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write to JSON file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *filename = CHAR(STRING_ELT(filename_, 0));
  yyjson_write_err err;
  bool success = yyjson_mut_write_file(filename, doc, opt->yyjson_write_flag, NULL, &err);
  if (!success) {
    yyjson_mut_doc_free(doc);
    error("Write to file error '%s': %s code: %u\n", filename, err.msg, err.code);
  }
  yyjson_mut_doc_free(doc);
  return R_NilValue;
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
SEXP serialize_sf_to_str_(SEXP sf_, SEXP to_geo_opts_, SEXP serialize_opts_) {

  unsigned int nprotect = 0;
  if (!inherits(sf_, "sf") && !inherits(sf_, "sfc")) {
    error("Not an sf object");
  }
  geo_serialize_options opt = create_geo_serialize_options(to_geo_opts_);
  
  serialize_options serialize_opt = parse_serialize_options(serialize_opts_);
  opt.serialize_opt = &serialize_opt;

  SEXP res_ = R_NilValue;
  if (inherits(sf_, "sfc")) {
    res_ = PROTECT(sfc_to_str(sf_, &opt)); nprotect++;
  } else if (inherits(sf_, "sf")) {
    res_ = PROTECT(sf_to_str(sf_, &opt)); nprotect++;
  } else {
    error("serialize_sf_to_str_: class not handled yet");
  }

  UNPROTECT(nprotect);
  return res_;
}

SEXP serialize_sf_to_file_(SEXP sf_, SEXP filename_, SEXP to_geo_opts_, SEXP serialize_opts_) {
  
  unsigned int nprotect = 0;
  if (!inherits(sf_, "sf") && !inherits(sf_, "sfc")) {
    error("Not an sf object");
  }
  geo_serialize_options opt = create_geo_serialize_options(to_geo_opts_);
  
  serialize_options serialize_opt = parse_serialize_options(serialize_opts_);
  opt.serialize_opt = &serialize_opt;
  
  // SEXP res_ = R_NilValue;
  if (inherits(sf_, "sfc")) {
    // res_ = PROTECT(sfc_to_str(sf_, &opt)); nprotect++;
    error("Serializing 'sfc' objects to file not done yet");
  } else if (inherits(sf_, "sf")) {
    PROTECT(sf_to_file(sf_, filename_, &opt)); nprotect++;
  } else {
    error("serialize_sf_to_file_: class not handled yet");
  }
  
  UNPROTECT(nprotect);
  return R_NilValue;
}

