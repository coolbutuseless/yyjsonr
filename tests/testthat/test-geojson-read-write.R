

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Set load path depending on whether debugging or testing within package
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
examples_dir <- testthat::test_path("geojson")

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Determine list of JSON files to test
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
json_files <- list.files(examples_dir, pattern = "json$", full.names = TRUE)

json <- lapply(json_files, function(x) {
  paste(readLines(x), collapse = "\n")
})
names(json) <- basename(tools::file_path_sans_ext(json_files))


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate reference objects using {geojsonsf}
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


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Read reference objects created from GeoJSON with {geojsonsf} package
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ref <- readRDS(file.path(examples_dir, "ref.rds"))


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Trivial column sorting for data.frames
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
df_sort <- function(df) {
  df[, sort(colnames(df)), drop = FALSE]
}

expect_equal_when_colnames_sorted <- function(tst, ref, label = NULL) {
  tst <- df_sort(tst)
  ref <- df_sort(ref)
  expect_equal(tst, ref, label = label)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Check that {yyjsonr} parses GeoJSON in the same was as {geojsonsf}
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("Compare parsing of GeoJSON to {geojsonsf}", {
  
  for (json_file in json_files) {
    
    # Reference objects from {geojsonsf} are indexed by the
    # basename() of the JSON file
    nm <- basename(json_file)

    # Read as 'sf' data.frame object
    sf <- read_geojson_file(json_file, type = 'sf')
    if (nm == 'standard-example.json') {
      # When geojsonsf promotos double 0.0 to string, it makes it '0'
      # whereas yyjsonr makes it "0.000000"
      sf$prop1 <- gsub("0.000000", "0", sf$prop1)
    }
    expect_equal_when_colnames_sorted(sf, ref$sf[[nm]], label = nm)
    
    # Read as 'sfc' list object  of just geometry
    sfc <- read_geojson_file(json_file, type = 'sfc')
    expect_equal(sfc, ref$sfc[[nm]], label = nm)
  }
})



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Using 'yyjsonr', round-trip some JSON 
#' - from geojson string to R 'sf' object 
#' - from R 'sf' object to geojson string
#' - from geojson string to R 'sf' object
#' 
#' Then assert that the two 'sf' objects are equal 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("Basic writing works for lots of different geojson", {
  
  tmp <- tempfile()
  
  for (i in seq_along(json)) {
    
    # Take the JSON
    js1 <- json[[i]]
    
    # Load it into R as 'sf'
    geojson1 <- read_geojson_str(js1)
    
    # Write it out to JSON str
    js2 <- write_geojson_str(geojson1)
    
    # Re-load it into R as 'sf'
    geojson2 <- read_geojson_str(js2)
    
    # Test 'sf' representations are equal
    expect_identical(geojson1, geojson2, label = names(json)[i])
    
    # write the json to file
    write_geojson_file(geojson1, tmp)
    
    geojson3 <- read_geojson_file(tmp)
    
    expect_identical(geojson1, geojson3)
  }
})



test_that("opts",  {
  
  opts <- opts_read_geojson()
  expect_true(is.list(opts))
  expect_true(inherits(opts, "opts_read_geojson"))
  
  opts <- opts_write_geojson()
  expect_true(is.list(opts))
  expect_true(inherits(opts, "opts_write_geojson"))
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # geojson read options
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  geo <- read_geojson_file(
    testthat::test_path("geojson/standard-example.json"), 
    opts = opts_read_geojson(property_promotion = 'string')
  )
  expect_true(is.character(geo$prop1))
    
  geo <- read_geojson_file(
    testthat::test_path("geojson/standard-example.json"), 
    opts = opts_read_geojson(property_promotion = 'list')
  )
  expect_true(is.list(geo$prop1))
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # unknown options spark warning
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_warning(
    write_geojson_str(geo, bad = TRUE),
    "Unknown option ignored"
  )
  
  expect_warning(
    write_geojson_str(geo, pretty = TRUE),
    "Unknown option ignored"
  )
  
  if (FALSE) {
    write_geojson_str(geo, json_opts = opts_write_json(pretty = TRUE)) |> cat()
  }
  
  
})




