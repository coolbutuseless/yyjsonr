
test_that("write_json_raw() works", {

  dat <- head(iris, 3)
  dat$Species <- as.character(dat$Species)
  
  js <- write_json_raw(dat)
  expect_true(is.raw(js))
  res <- read_json_raw(js)
  expect_identical(res, dat)
  
  
})
