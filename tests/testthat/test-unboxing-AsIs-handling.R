

test_that("AsIs handling works", {
  library(jsonlite)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' I() forces scalar values to be written as JSON-array
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  l1 <- list(a = 1, b = I(2))
  expect_equal(
    write_json_str(l1, auto_unbox = TRUE),
    '{"a":1.0,"b":[2.0]}'
  )
  expect_equal(
    write_json_str(l1, auto_unbox = FALSE),
    '{"a":[1.0],"b":[2.0]}'
  )
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Unboxing doesn't affect vectors with length > 1
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  l2 <- list(a = 1, b = I(c(2, 3)))
  expect_equal(
    write_json_str  (l2, auto_unbox = TRUE),
    '{"a":1.0,"b":[2.0,3.0]}'
  )
  expect_equal(
    write_json_str  (l2, auto_unbox = FALSE),
    '{"a":[1.0],"b":[2.0,3.0]}'
  )
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' 
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
  s <- '{"a":1,"b":[2]}'
  
  x <- read_json_str(s)
  expect_identical(x, list(a = 1L, b = 2L))
  
  
  x <- read_json_str(s, length1_array_asis = TRUE)
  expect_identical(x, list(a = 1L, b = structure(2L, class = 'AsIs')))
  
  s2 <- write_json_str(x, auto_unbox = TRUE)
  expect_identical(s, s2)

})


test_that("asis = 'strip' ignores AsIs with auto_unbox", {
  l <- list(a = 1, b = I(2))

  # Default: keep (existing behavior)
  expect_equal(
    write_json_str(l, auto_unbox = TRUE),
    '{"a":1.0,"b":[2.0]}'
  )

  # Strip: ignore AsIs, unbox normally
  expect_equal(
    write_json_str(l, auto_unbox = TRUE, asis = "strip"),
    '{"a":1.0,"b":2.0}'
  )

  # Without auto_unbox, asis has no effect
  expect_equal(
    write_json_str(l, auto_unbox = FALSE, asis = "strip"),
    '{"a":[1.0],"b":[2.0]}'
  )
})


test_that("asis = 'keep' is default and backward compatible", {
  l <- list(a = I(1))
  expect_equal(
    write_json_str(l, auto_unbox = TRUE, asis = "keep"),
    '{"a":[1.0]}'
  )
  # Same as default
  expect_equal(
    write_json_str(l, auto_unbox = TRUE),
    '{"a":[1.0]}'
  )
})


test_that("asis = 'strip' works with string AsIs from format()", {
  l <- list(val = I("hello"))
  expect_equal(
    write_json_str(l, auto_unbox = TRUE, asis = "strip"),
    '{"val":"hello"}'
  )
  expect_equal(
    write_json_str(l, auto_unbox = TRUE),
    '{"val":["hello"]}'
  )
})
