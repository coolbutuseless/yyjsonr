
# library(sf)
# library(geojsonsf)
# 
# 
# gid <- "tests/testthat/geojson/standard-example.json"
# gid <- "tests/testthat/geojson-ids/standard-example-with-int-ids.json"
# gid <- "tests/testthat/geojson-ids/standard-example-with-chr-ids.json"
# gid <- "tests/testthat/geojson-ids/standard-example-with-chr-ids-missings.json"
# gid <- "tests/testthat/geojson-ids/standard-example-with-mixed-ids.json"
# 
# read_geojson_file(gid)


test_that("parse geojson ids", {
  
  # no IDs
  ref_noid <- read_geojson_file(testthat::test_path("geojson/standard-example.json"))
  expect_identical(names(ref_noid)[[1]], "prop0")
  
  # integer ids
  res <- read_geojson_file(testthat::test_path("geojson-ids/standard-example-with-int-ids.json"))
  expect_identical(names(res)[[1]], "id")
  expect_true(is.integer(res$id))
  expect_identical(res$id, c(1L, 2L, 3L))
  
  
  # string ids
  res <- read_geojson_file(testthat::test_path("geojson-ids/standard-example-with-chr-ids.json"))
  expect_identical(names(res)[[1]], "id")
  expect_true(is.character(res$id))
  expect_identical(res$id, as.character(c(0L, 1L, 2L)))
  
  # string ids with missing values
  res <- read_geojson_file(testthat::test_path("geojson-ids/standard-example-with-chr-ids-missings.json"))
  expect_identical(names(res)[[1]], "id")
  expect_true(is.character(res$id))
  expect_identical(res$id, as.character(c(0L, 1L, NA)))
  
  # mixture of string/integer ids is promoted to string
  res <- read_geojson_file(testthat::test_path("geojson-ids/standard-example-with-mixed-ids.json"))
  expect_identical(names(res)[[1]], "id")
  expect_true(is.character(res$id))
  expect_identical(res$id, as.character(c(1L, 2L, 3L)))
  
})
