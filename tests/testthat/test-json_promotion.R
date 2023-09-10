

js <- r"(
[
  {"a":1.234560},
  {"a":2},
  {"a":"apple"}
])"


test_that("numeric promotion to string instead of list works", {

  res <- read_json_str(js)
  expect_identical(res$a, list(1.234560, 2L, "apple"))
  
  res <- read_json_str(js, promote_num_to_string = FALSE)
  expect_identical(res$a, list(1.234560, 2L, "apple"))

  res <- read_json_str(js, promote_num_to_string = TRUE) 
  expect_identical(res$a, c("1.234560", "2", "apple"))
})
