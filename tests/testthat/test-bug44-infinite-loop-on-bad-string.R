

test_that("Invalid string input should cause error", {
  
  testthat::capture_output({
  expect_error({
    read_json_str("hello")
  })
  })
  
})
