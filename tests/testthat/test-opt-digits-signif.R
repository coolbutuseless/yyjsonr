


test_that("multiplication works", {

  nums <- c(0.001, 12.34567890123456)
  
  s <- write_json_str(nums)
  expect_identical(s, "[0.001,12.34567890123456]")
  
  s <- write_json_str(nums, digits_signif = 6)
  expect_identical(s, "[0.001,12.3457]")
  
  
  s <- write_json_str(nums, digits_signif = 3)
  expect_identical(s, "[0.001,12.3]")
  
  
})
