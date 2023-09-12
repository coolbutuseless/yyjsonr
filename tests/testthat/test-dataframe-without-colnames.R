


test_that("data.frames without column names work like jsonlite", {
  
  # output an array of arrays.
  # the inner array represents a row of the data.frame
  
  aa <- data.frame(x = 1:2, y = c('y', 'n'))
  colnames(aa) <- NULL
  
  s <- write_json_str(aa)
  expect_equal(s, '[[1,"y"],[2,"n"]]')
  
})
