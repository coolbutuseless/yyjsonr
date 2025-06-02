

test_that("ndjson raw io works", {

  dat <- head(mtcars)
  rownames(dat) <- NULL
  
  js <- write_ndjson_raw(dat)
  expect_true(is.raw(js))
  res <- read_ndjson_raw(js, 'df')
  expect_identical(res, dat)
  
  
  res <- read_ndjson_raw(js, 'list')
  expect_true(is.list(res))
  
})
