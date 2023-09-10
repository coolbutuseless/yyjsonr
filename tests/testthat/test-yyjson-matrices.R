

test_that("Basic matrices work", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Logical
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  mat  <- matrix(c(TRUE, FALSE), 2, 3)
  json <- write_json_str(mat)
  expect_identical(
    json,
    "[[true,true,true],[false,false,false]]"
  )
  mat2 <- read_json_str(json)
  expect_identical(mat2, mat)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Integer
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  mat  <- matrix(1:6, 2, 3)
  json <- write_json_str(mat)
  expect_identical(
    json,
    "[[1,3,5],[2,4,6]]"
  )
  mat2 <- read_json_str(json)
  expect_identical(mat2, mat)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Numeric
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  mat  <- matrix(as.numeric(1:6), 2, 3)
  json <- write_json_str(mat)
  expect_identical(
    json,
    "[[1.0,3.0,5.0],[2.0,4.0,6.0]]"
  )
  mat2 <- read_json_str(json)
  expect_identical(mat2, mat)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Character
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  mat  <- matrix(as.character(1:6), 2, 3)
  json <- write_json_str(mat)
  expect_identical(
    json,
    '[["1","3","5"],["2","4","6"]]'
  )
  mat2 <- read_json_str(json)
  expect_identical(mat2, mat)
  
})


test_that("Basic Arrays work", {
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Logical
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  arr <- array(c(TRUE,FALSE,FALSE), c(2, 3, 2))
  arr
  json <- write_json_str(arr)
  json  
  
  expect_identical(
    json,
    "[[[true,false,false],[false,true,false]],[[true,false,false],[false,true,false]]]"
  )
  
   arr2 <- read_json_str(json)
  expect_identical(arr2, arr)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Integer
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  arr <- array(1:12, c(2, 3, 2))
  arr
  json <- write_json_str(arr)
  json  
  
  expect_identical(
    json,
    "[[[1,3,5],[2,4,6]],[[7,9,11],[8,10,12]]]"
  )
  
  (arr2 <- read_json_str(json))
  expect_identical(arr2, arr)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Real
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  arr <- array(as.numeric(1:12), c(2, 3, 2))
  json <- write_json_str(arr)
  
  expect_identical(
    json,
    "[[[1.0,3.0,5.0],[2.0,4.0,6.0]],[[7.0,9.0,11.0],[8.0,10.0,12.0]]]"
  )
  
  arr2 <- read_json_str(json)
  expect_identical(arr2, arr)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Character
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  arr <- array(as.character(1:12), c(2, 3, 2))
  json <- write_json_str(arr)
  
  expect_identical(
    json,
    '[[["1","3","5"],["2","4","6"]],[["7","9","11"],["8","10","12"]]]'
  )
  
  arr2 <- read_json_str(json)
  expect_identical(arr2, arr)
})
















