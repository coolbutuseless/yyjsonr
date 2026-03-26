
test_that("empty_object option works", {
  
  res <- read_json_str("{}")
  expect_identical(res, setNames(list(), character(0)))
  
  res <- read_json_str("{}", empty_object = 'named_list')
  expect_identical(res, setNames(list(), character(0)))
  
  
  res <- read_json_str("{}", empty_object = 'NULL')
  expect_identical(res, NULL)
  
  
  res <- read_json_str("[{}, {}, 3]")
  expect_identical(res, list(
    setNames(list(), character(0)),
    setNames(list(), character(0)),
    3L
  ))
  
  res <- read_json_str("[{}, {}, 3]", empty_object = 'NULL')
  expect_identical(res, list(NULL, NULL, 3L))
})
