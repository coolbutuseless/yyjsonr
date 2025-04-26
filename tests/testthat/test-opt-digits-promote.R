

test_that("option 'digits_promote' works", {

  j <- '[1.234567890123456,"hello"]'
  x <- read_json_str(j)
  expect_true(is.list(x))
  expect_equal(x[[1]], 1.234567890123456)
  
  
  x <- read_json_str(j, opts = opts_read_json(promote_num_to_string = TRUE))
  x
  expect_true(is.atomic(x))
  expect_equal(x[[1]], "1.234568")
  
  
  x <- read_json_str(j, opts = opts_read_json(promote_num_to_string = TRUE,
                                              digits_promote = 15))
  x
  expect_true(is.atomic(x))
  expect_equal(x[[1]], "1.234567890123456")

  
})
