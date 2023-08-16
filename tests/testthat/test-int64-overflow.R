
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' bit64::int64 is a signed integer.  but json has an unsigned 64bit
#' integer type.  We should warn when something happens here.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("int64 overflow protection works", {
  
  expect_warning(from_json_str('[9223372036854775809]', int64 = 'bit64'), "overflow")
  
  expect_identical(from_json_str('[9223372036854775809]'), "9223372036854775809")  
  expect_identical(
    from_json_str('[9223372036854775807]', int64 = 'bit64'), 
    bit64::as.integer64("9223372036854775807")
  )
  
  
  
  expect_warning(from_json_str('9223372036854775809', int64 = 'bit64'), "overflow")
  
})
