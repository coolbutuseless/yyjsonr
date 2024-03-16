


test_that("multiplication works", {
  
  filename <- testthat::test_path("ndjson/ndjson-long-lines-10k-issue19.ndjson")
  
  expect_no_error(
    read_ndjson_file(filename)
  )  
})
