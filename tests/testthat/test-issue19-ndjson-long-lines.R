


test_that("multiplication works", {
  
  filename <- "examples/ndjson-long-lines-10k-issue19.ndjson"
  
  expect_no_error(
    read_ndjson_file(filename)
  )  
})
