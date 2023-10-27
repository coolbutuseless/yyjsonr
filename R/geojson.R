

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Options for reading in GeoJSON
#' 
#' @param property_promotion What is the most general container type to use when 
#'        properties differ across a FEATURECOLLECTION?  E.g. if the property
#'        exists both as a numeric and a string, should all values be promoted
#'        to a 'string', or contained as different types in a 'list'.  
#'        Default: 'string' will behave like 'geojsonsf'
#' @param type 'sf' or 'sfc'
#' @param property_promotion_lgl_as_int when promoting properties into a string,
#'        should logical values beccome strings e.g. "TRUE" or integers
#'        e.g. "1".  Default: "integer" in order to match `geojsonsf` packages
#' 
#' @return named list
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
opts_read_geojson <- function(type = c('sf', 'sfc'), 
                          property_promotion = c('string', 'list'),
                          property_promotion_lgl_as_int = c('integer', 'string')) {
  structure(
    list(
      type               = match.arg(type),
      property_promotion = match.arg(property_promotion)
    ),
    class = "opts_read_geojson"
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Options for writing from \code{sf} object to \code{GeoJSON}
#' 
#' Currently no options available.
#' 
#' @return named list of options
#' @export
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
#' @param filename filename
#' @param str single character string containing GeoJSON
#' @param opts named list of options. Usually created with \code{opts_read_geojson()}.
#'        Default: empty \code{list()} to use the default options.
#' @param ... any extra named options override those in \code{opts}
#'
#' @return \code{sf} object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_geojson_str <- function(str, opts = list(), ...) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    parse_geojson_str_,
    str, 
    opts,  # geojson parse opts
    list(yyjson_read_flag  = 0L) # general parse opts
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname read_geojson_str
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_geojson_file <- function(filename, opts = list(), ...) {
  opts <- modify_list(opts, list(...))

  .Call(
    parse_geojson_file_,
    filename, 
    opts,  # geojson parse opts
    list(yyjson_read_flag  = 0L) # general parse opts
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Write SF to geojson string
#' 
#' @param x \code{sf} object. Supports \code{sf} or \code{sfc}
#' @param filename filename
#' @param opts named list of options. Usually created with \code{opts_write_geojson()}.
#'        Default: empty \code{list()} to use the default options.
#' @param ... any extra named options override those in \code{opts}
#' @param digits decimal places to keep for floating point numbers. Default: -1.
#'        Positive values specify number of decimal places. Using zero will
#'        write the numeric value as an integer. Values less than zero mean that
#'        the floating point value should be written as-is (the default).
#'
#' @return character string containing json
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_geojson_str <- function(x, opts = list(), ..., digits = -1) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    serialize_sf_to_str_,
    x,
    opts,                        # geojson serialize opts
    list(yyjson_write_flag = 0L, digits = digits) # general serialize opts
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname write_geojson_str
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_geojson_file <- function(x, filename, opts = list(), ..., digits = -1) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    serialize_sf_to_file_,
    x,
    filename,
    opts,                        # geojson serialize opts
    list(yyjson_write_flag = 0L, digits = digits) # general serialize opts
  )
  
  invisible()
}
