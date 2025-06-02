

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Options for reading in GeoJSON
#' 
#' @param type 'sf' or 'sfc'
#' @param property_promotion What is the most general container type to use when 
#'        properties differ across a FEATURECOLLECTION?  E.g. if the property
#'        exists both as a numeric and a string, should all values be promoted
#'        to a 'string', or contained as different types in a 'list'.  
#'        Default: 'string' will behave like \code{geojsonsf} package.
#' @param property_promotion_lgl when \code{property_promotion = "string"}
#'        should logical values become words (i.e. \code{"TRUE"}/\code{"FALSE"}) 
#'        or integers (i.e. \code{"1"}/\code{"0"}).  
#'        Default: "integer" in order to match \code{geojsonsf} package
#' 
#' @return Named list of options specific to reading GeoJSON
#' @export
#' 
#' @examples
#' # Create a set of options to use when reading geojson
#' opts_read_geojson()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
opts_read_geojson <- function(type                   = c('sf', 'sfc'), 
                              property_promotion     = c('string', 'list'),
                              property_promotion_lgl = c('integer', 'string')) {
  structure(
    list(
      type                   = match.arg(type),
      property_promotion     = match.arg(property_promotion),
      property_promotion_lgl = match.arg(property_promotion_lgl)
    ),
    class = "opts_read_geojson"
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Options for writing from \code{sf} object to \code{GeoJSON}
#' 
#' Currently no options available.
#' 
#' @return Named list of options specific to writing GeoJSON
#' @export
#' 
#' @examples
#' # Create a set of options to use when writing geojson
#' opts_write_geojson()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
opts_write_geojson <- function() {
  structure(
    list(
      
    ),
    class = "opts_write_geojson"
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Load GeoJSON as \code{sf} object
#' 
#' Coordinate reference system is always WGS84 in accordance with GeoJSON RFC.
#' 
#' @param filename Filename
#' @param str Single string containing GeoJSON.
#' @param opts Named list of GeoJSON-specific options. Usually created 
#'        with \code{opts_read_geojson()}.
#'        Default: empty \code{list()} to use the default options.
#' @param ... Any extra named options override those in GeoJSON-specific options 
#'        - \code{opts}
#' @param json_opts Named list of vanilla JSON options as used by \code{read_json_str()}.
#'        This is usually created with \code{opts_read_json()}.  Default value is
#'        an empty \code{list()} which means to use all the default JSON parsing 
#'        options which is usually the correct thing to do when reading GeoJSON.
#'
#' @return \code{sf} object
#' @export
#' 
#' @examples
#' geojson_file <- system.file("geojson-example.json", package = 'yyjsonr')
#' read_geojson_file(geojson_file)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_geojson_str <- function(str, opts = list(), ..., json_opts = list()) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    parse_geojson_str_,
    str, 
    opts,  # geojson parse opts
    json_opts
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname read_geojson_str
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_geojson_file <- function(filename, opts = list(), ..., json_opts = list()) {
  opts <- modify_list(opts, list(...))

  .Call(
    parse_geojson_file_,
    normalizePath(filename), 
    opts,  # geojson parse opts
    json_opts
  )
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Write SF to GeoJSON string
#' 
#' Coordinate reference system is always WGS84 in accordance with GeoJSON RFC.
#' 
#' @param x \code{sf} object. Supports \code{sf} or \code{sfc}
#' @param filename filename
#' @param opts named list of options. Usually created with \code{opts_write_geojson()}.
#'        Default: empty \code{list()} to use the default options.
#' @param ... any extra named options override those in \code{opts}
#' @param json_opts Named list of vanilla JSON options as used by \code{write_json_str()}.
#'        This is usually created with \code{opts_write_json()}.  Default value is
#'        an empty \code{list()} which means to use all the default JSON writing 
#'        options which is usually the correct thing to do when writing GeoJSON.
#'
#' @return Character string containing GeoJSON, or \code{NULL} if GeoJSON
#'         written to file.
#' @export
#' 
#' @examples
#' geojson_file <- system.file("geojson-example.json", package = 'yyjsonr')
#' sf <- read_geojson_file(geojson_file)
#' cat(write_geojson_str(sf, json_opts = opts_write_json(pretty = TRUE)))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_geojson_str <- function(x, opts = list(), ..., json_opts = list()) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    serialize_sf_to_str_,
    x,
    opts,      # geojson serialize opts
    json_opts  # general serialize opts
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname write_geojson_str
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_geojson_file <- function(x, filename, opts = list(), ..., json_opts = list()) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    serialize_sf_to_file_,
    x,
    normalizePath(filename, mustWork = FALSE),
    opts,     # geojson serialize opts
    json_opts # general serialize opts
  )
  
  invisible()
}
