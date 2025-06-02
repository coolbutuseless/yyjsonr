

test_that("geojson raw IO works", {
  
  file <- testthat::test_path("geojson/standard-example.json")
  gj <- read_geojson_file(file)
  
  js <- write_geojson_raw(gj)
  expect_true(is.raw(js))
  res <- read_geojson_raw(js)
  expect_identical(res, gj)
  
    
})
