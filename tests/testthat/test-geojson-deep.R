

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' All names are the same.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
expect_same_names <- function(tst, ref, label = "names") {
  expect_equal(
    sort(names(tst)),
    sort(names(ref)),
    label = label
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Attributes check
#'   - same attribute names  (ignore 'names' which may be sorted differently)
#'   - matchine attributes have the same contents
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
expect_same_attributes <- function(tst, ref, ignore = c("names"), 
                                    label = "In 'expect_same_attributes()") {
  expect_equal(
    setdiff(sort(names(attributes(tst))), ignore),
    setdiff(sort(names(attributes(ref))), ignore),
    label = paste(label, "attribute names")
  )
  
  for (nm in setdiff(names(attributes(ref)), ignore)) {
    expect_same_contents(
      attr(tst, nm, exact = TRUE),
      attr(ref, nm, exact = TRUE),
      label = paste0(label, " attr[", nm, "]")
    )
  }
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  List
#    - same length
#    - same names
#    - each matching item in the list has the same contents
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
expect_same_contents.list <- function(tst, ref, label = "list") {
  
  expect_equal(length(tst), length(ref), label = label)
  expect_same_names(tst, ref, label)
  expect_same_attributes(tst, ref, label)
  
  nms <- sort(names(ref))
  if (is.null(nms)) {
    for (i in seq_len(length(ref))) {
      expect_same_contents(tst[[i]], ref[[i]], label = paste0(label, "[[", i, "]]"))
    }
  } else {
    # TODO: also check for duplicate names!
    for (nm in nms) {
      expect_same_contents(tst[[nm]], ref[[nm]], label = paste0(label,"$",nm))
    }
  }
  
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# For vectors:
#   - equal (when we ignore attributes)
#   - attributes are the same
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
expect_same_contents.default <- function(tst, ref, label = "default") {
  expect_same_attributes(tst, ref, label = label)
  
  if (is.character(tst)) {
    tst <- gsub("^0.0+", "0", tst)
  }
  
  expect_equal(tst, ref, ignore_attr = TRUE, label = label)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' DAta.frame
#   - same names (irrespective of order)
#   - attributes are equal
#   - each column has the same content
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
expect_same_contents.data.frame <- function(tst, ref, label = "df") {
  expect_same_names(tst, ref, label = label)
  expect_same_attributes(tst, ref, label = label)
  for (nm in colnames(ref)) {
    expect_same_contents(tst[[nm]], ref[[nm]], label = paste0(label, "$", nm))  
  }
}


expect_same_contents <- function(tst, ref, label) UseMethod("expect_same_contents")


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Set load path depending on whether debugging or testing within package
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if (testthat::is_testing()) {
  examples_dir <- "./geojson"
} else {
  # manually running during development
  examples_dir <- "tests/testthat/geojson"
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Determine list of JSON files to test
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
json_files <- list.files(examples_dir, pattern = "json$", full.names = TRUE)

json <- lapply(json_files, function(x) {
  paste(readLines(x), collapse = "\n")
})
names(json) <- basename(tools::file_path_sans_ext(json_files))

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate reference objects using geojson
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if (FALSE) {
  # json_files <- list.files(examples_dir, pattern = "json$", full.names = TRUE)
  # 
  # ref <- list(sf = list(), sfc = list())
  # 
  # prep <- function(json_file) {
  #   nm <- basename(json_file)
  #   ref$sf [[nm]] <<- geojsonsf::geojson_sf (json_file)
  #   ref$sfc[[nm]] <<- geojsonsf::geojson_sfc(json_file)
  # }
  # 
  # for (json_file in json_files) {
  #   prep(json_file)
  # }
  # 
  # saveRDS(ref, "tests/testthat/geojson/ref.rds", compress = 'xz')
}

ref <- readRDS(file.path(examples_dir, "ref.rds"))


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("ensure helper 'expect_same_content()' works", {
  mtcars2 <- mtcars[, sample(colnames(mtcars))]
  expect_same_contents(mtcars, mtcars2)
})


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("Compare input to geojsonsf", {
  
  json_file <- json_files[5]
  
  for (json_file in json_files) {
    
    nm <- basename(json_file)
    # cat(nm, "\n")
    
    expect_same_contents(
      from_geojson_file(json_file, type = 'sf'), 
      ref$sf[[nm]], 
      label = nm
    )
    
    expect_same_contents(
      from_geojson_file(json_file, type = 'sfc'), 
      ref$sfc[[nm]],
      label = nm
    )
  }
  
})


# if (FALSE) {
#   js <- json$`coord-class-multipolygon-xyzm`
#   cat(js)
#   
#   tst <- from_geojson_str(js, type = 'sfc')
#   ref <- geojsonsf::geojson_sfc(js)
#   expect_same_contents(tst, ref)
# 
#   tst <- from_geojson_str(js)
#   ref <- geojsonsf::geojson_sf(js)
#   expect_same_contents(tst, ref)
# }


# if (FALSE) {
#   bench::mark(
#     geojsonsf::geojson_sf(geojsonsf::geo_melbourne),
#     from_geojson_str     (geojsonsf::geo_melbourne),
#     check = FALSE
#   )
# }
# 
# 
# if (FALSE) {
#   library(sf)
#   library(microbenchmark)
#   
#   nc = st_read(system.file("shape/nc.shp", package="sf"))
#   
#   dst_gjsf = tempfile(fileext = ".geojson")
#   dst_gjyy = tempfile(fileext = ".geojson")
#   
#   bench::mark(
#     sfgj = cat(geojsonsf::sf_geojson(nc), file = dst_gjsf),
#     yygj = yyjsonr::to_json_file(nc, filename = dst_gjyy)
#   )
# }









