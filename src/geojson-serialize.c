
#define R_NO_REMAP

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
  serialize_options *serialize_opt;
} geo_serialize_options;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise geo_parse_opts from an R named list from the user
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
geo_serialize_options create_geo_serialize_options(SEXP to_geo_opts_) {
  geo_serialize_options opt = {
    .serialize_opt = NULL
  };
  
  if (Rf_isNull(to_geo_opts_) || Rf_length(to_geo_opts_) == 0) {
    return opt;
  }
  
  if (!Rf_isNewList(to_geo_opts_)) {
    Rf_error("'to_geo_opts_' must be a list");
  }
  
  SEXP nms_ = Rf_getAttrib(to_geo_opts_, R_NamesSymbol);
  if (Rf_isNull(nms_)) {
    Rf_error("'to_geo_opts_' must be a named list");
  }
  
  for (unsigned int i = 0; i < Rf_length(to_geo_opts_); i++) {
    const char *opt_name = CHAR(STRING_ELT(nms_, i));
    // SEXP val_ = VECTOR_ELT(to_geo_opts_, i);
    Rf_warning("opt_geojson_write(): Unknown option ignored: '%s'\n", opt_name);
  }
  return opt;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize geometry
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_val *serialize_geom(SEXP sf_, yyjson_mut_doc *doc, geo_serialize_options *opt) {
  
  yyjson_mut_val *obj = yyjson_mut_obj(doc);
  
  bool geom_collection = false;
  
  if (Rf_inherits(sf_, "POINT")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "Point");
  } else if (Rf_inherits(sf_, "MULTIPOINT")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "MultiPoint");
  } else if (Rf_inherits(sf_, "LINESTRING")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "LineString");
  } else if (Rf_inherits(sf_, "MULTILINESTRING")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "MultiLineString");
  } else if (Rf_inherits(sf_, "POLYGON")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "Polygon");
  } else if (Rf_inherits(sf_, "MULTIPOLYGON")) {
    yyjson_mut_obj_add_str(doc, obj, "type", "MultiPolygon");
  } else if (Rf_inherits(sf_, "GEOMETRYCOLLECTION")) {
    geom_collection = true;
    yyjson_mut_obj_add_str(doc, obj, "type", "GeometryCollection");
  } else {
    Rf_error("@@@@@@@ serialize_geom Issue. Unhandled geometry type\n");
  }
  
  
  if (!geom_collection) {
    yyjson_mut_val *key = yyjson_mut_str(doc, "coordinates");
    yyjson_mut_obj_add(obj, key, serialize_core(sf_, doc, opt->serialize_opt));
  } else{
    
    if (!Rf_isNewList(sf_)) {
      Rf_error("Expecting geomcollection to be a VECSXP not: %s", Rf_type2char((SEXPTYPE)TYPEOF(sf_)));
    }
    
    // An array of geoms
    yyjson_mut_val *geoms = yyjson_mut_arr(doc);
    
    // Unpack each element of the 'sf_' list as a geom and add to the array
    for (unsigned int i = 0; i < Rf_length(sf_); i++) {
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
  
  int nprotect = 0;
  if (!Rf_isNewList(sfc_)) {
    Rf_error("serialize_sfc(): Expeting list\n");
  }
  
  
  R_xlen_t N = Rf_xlength(sfc_);
  SEXP geojson_ = PROTECT(Rf_allocVector(STRSXP, N)); nprotect++;
  
  for (unsigned int idx = 0; idx < N; idx++) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    SEXP elem_ = VECTOR_ELT(sfc_, idx);
    yyjson_mut_val *val = serialize_geom(elem_, doc, opt);
    yyjson_mut_doc_set_root(doc, val);
    
    yyjson_write_err err;
    char *json = yyjson_mut_write_opts(doc, opt->serialize_opt->yyjson_write_flag, NULL, NULL, &err);
    if (json == NULL) {
      yyjson_mut_doc_free(doc);
      Rf_error("Write to string error: %s", err.msg);
    }
    
    SET_STRING_ELT(geojson_, idx, Rf_mkChar(json));
    yyjson_mut_doc_free(doc);
  }
  
  
  UNPROTECT(nprotect);
  return geojson_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sf data.frame to feature collection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_mut_doc *sf_to_json(SEXP sf_, geo_serialize_options *opt) {

  // int nprotect = 0;
  if (!Rf_isNewList(sf_) || !Rf_inherits(sf_, "data.frame")) {
    Rf_error("serialize_sf(): Expecting data.frame\n");
  }

  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  // int ncol = Rf_length(sf_);
  int nrow = Rf_length(VECTOR_ELT(sf_, 0));
  // Rprintf("[%i, %i] data.frame\n", nrow, ncol);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Determine index of geometry collection
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int geom_col_idx = -1;
  // SEXP geom_col_name_ = Rf_getAttrib(sf_, Rf_mkString("sf_column"));
  SEXP geom_col_name_ = Rf_getAttrib(sf_, Rf_install("sf_column"));
  if (Rf_isNull(geom_col_name_)) {
    Rf_error("sf_to_str(): Couldn't determine 'sf_column' name");
  }
  const char *geom_col_name = CHAR(STRING_ELT(geom_col_name_, 0));
  SEXP colnames_ = Rf_getAttrib(sf_, R_NamesSymbol);
  for (int i = 0; i < Rf_length(colnames_); i++) {
    if (strcmp(CHAR(STRING_ELT(colnames_, i)), geom_col_name) == 0) {
      geom_col_idx = i;
      break;
    }
  }
  if (geom_col_idx == -1) {
    Rf_error("sf_to_str(): Couldn't 'sf_column' name '%s' in column names of sf object", geom_col_name);
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
  free(col_type);
  
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
  
  int nprotect = 0;
  yyjson_mut_doc *doc = sf_to_json(sf_, opt);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write the doc to a string
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  yyjson_write_err err;
  char *json = yyjson_mut_write_opts(doc, opt->serialize_opt->yyjson_write_flag, NULL, NULL, &err);
  if (json == NULL) {
    yyjson_mut_doc_free(doc);
    Rf_error("serialize_sf() Write to string error: %s", err.msg);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Convert string to R character, tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP geojson_ = PROTECT(Rf_mkString(json)); nprotect++;
  free(json);
  yyjson_mut_doc_free(doc);
  UNPROTECT(nprotect);
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
  bool success = yyjson_mut_write_file(filename, doc, opt->serialize_opt->yyjson_write_flag, NULL, &err);
  if (!success) {
    yyjson_mut_doc_free(doc);
    Rf_error("Write to file error '%s': %s", filename, err.msg);
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

  int nprotect = 0;
  if (!Rf_inherits(sf_, "sf") && !Rf_inherits(sf_, "sfc")) {
    Rf_error("Not an sf object");
  }
  geo_serialize_options opt = create_geo_serialize_options(to_geo_opts_);
  
  serialize_options serialize_opt = parse_serialize_options(serialize_opts_);
  opt.serialize_opt = &serialize_opt;

  SEXP res_ = R_NilValue;
  if (Rf_inherits(sf_, "sfc")) {
    res_ = PROTECT(sfc_to_str(sf_, &opt)); nprotect++;
  } else if (Rf_inherits(sf_, "sf")) {
    res_ = PROTECT(sf_to_str(sf_, &opt)); nprotect++;
  } else {
    Rf_error("serialize_sf_to_str_: class not handled yet");
  }

  UNPROTECT(nprotect);
  return res_;
}

SEXP serialize_sf_to_file_(SEXP sf_, SEXP filename_, SEXP to_geo_opts_, SEXP serialize_opts_) {
  
  int nprotect = 0;
  if (!Rf_inherits(sf_, "sf") && !Rf_inherits(sf_, "sfc")) {
    Rf_error("Not an sf object");
  }
  geo_serialize_options opt = create_geo_serialize_options(to_geo_opts_);
  
  serialize_options serialize_opt = parse_serialize_options(serialize_opts_);
  opt.serialize_opt = &serialize_opt;
  
  // SEXP res_ = R_NilValue;
  if (Rf_inherits(sf_, "sfc")) {
    // res_ = PROTECT(sfc_to_str(sf_, &opt)); nprotect++;
    Rf_error("Serializing 'sfc' objects to file not done yet");
  } else if (Rf_inherits(sf_, "sf")) {
    PROTECT(sf_to_file(sf_, filename_, &opt)); nprotect++;
  } else {
    Rf_error("serialize_sf_to_file_: class not handled yet");
  }
  
  UNPROTECT(nprotect);
  return R_NilValue;
}

