
test_that("multiplication works", {

  # issue #35
  color <- c("red", "blue", NA, "red", "white")
  color <- factor(color, levels=c("red", "blue"))
  color
  
  expect_no_error({
    color_str <- yyjsonr::write_json_str(color, factor = 'integer')
  })
  expect_identical('[1,2,null,1,null]', color_str)
  
  expect_no_error({
    color_str <- yyjsonr::write_json_str(color, factor = 'string')
  })
  expect_identical('["red","blue",null,"red",null]', color_str)

})

