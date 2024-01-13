

test_that("unsigned int64 to double works", {
  
  str <- '[1.0, 4294967296]'
  robj <- read_json_str(str, int64 = 'double')
  
  expect_true(is.double(robj))
  expect_equal(robj, c(1, 4294967296))
})



test_that("signed int64 to double works", {
  
  str <- '[1.0, -4294967296]'
  robj <- read_json_str(str, int64 = 'double')
  
  expect_true(is.double(robj))
  expect_equal(robj, c(1, -4294967296))
})
