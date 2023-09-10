

test_that("data.frame to-from json works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with lots of types
  #   - coltypes: int, real, integer64, string, date, time, raw
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  df1 <- data.frame(
    a = 1:3,
    b = as.numeric(1:3),
    c = bit64::as.integer64(1:3),
    d = letters[1:3],
    e = as.Date(1:3, origin = "1970-01-01"),
    f = as.POSIXct("2020-01-01 01:02:03", tz = 'UTC'),
    g = as.raw(1:3)
  )
  df1$h <- list(a = 1:2, b = 2:3, c = 3:4)
  
  expect_equal(
    write_json_str(df1),
    r"([{"a":1,"b":1.0,"c":1,"d":"a","e":"1970-01-02","f":"2020-01-01 01:02:03","g":1,"h":[1,2]},{"a":2,"b":2.0,"c":2,"d":"b","e":"1970-01-03","f":"2020-01-01 01:02:03","g":2,"h":[2,3]},{"a":3,"b":3.0,"c":3,"d":"c","e":"1970-01-04","f":"2020-01-01 01:02:03","g":3,"h":[3,4]}])"
  )
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Parse it back.  Lots of types don't make the round trip.
  #'  - raw         -> integer
  #'  - small bit64 -> integer
  #'  - Date        -> str
  #'  - POSIXct     -> str
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  df1_json <- r"([{"a":1,"b":1.0,"c":1,"d":"a","e":"1970-01-02","f":"2020-01-01 01:02:03","g":1,"h":[1,2]},{"a":2,"b":2.0,"c":2,"d":"b","e":"1970-01-03","f":"2020-01-01 01:02:03","g":2,"h":[2,3]},{"a":3,"b":3.0,"c":3,"d":"c","e":"1970-01-04","f":"2020-01-01 01:02:03","g":3,"h":[3,4]}])"
  df2_check <- read_json_str(df1_json)
  
  df2_reference <- data.frame(
    a = 1:3,
    b = as.numeric(1:3),
    c = 1:3,
    d = letters[1:3],
    e = as.character(as.Date(1:3, origin = "1970-01-01")),
    f = as.character(as.POSIXct("2020-01-01 01:02:03")),
    g = 1:3
  )
  df2_reference$h <- list(1:2, 2:3, 3:4)

  expect_equal(df2_check, df2_reference)
  
})


test_that("array of nested json objects to data.frame", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with only atomic vectors.  No missing values.
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str('[{"a":1,"b":10.1,"c":"fred"},{"a":2,"b":10.2,"c":"greg"}]')
  df <- data.frame(
    a = c(1L, 2L),
    b = c(10.1, 10.2),
    c = c("fred", "greg")
  )
  expect_equal(test, df)
    
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with only atomic vectors - and missing values
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str('[{"b":10.1},{"a":2,"c":"greg"}]')
  df <- data.frame(
    b = c(10.1, NA),
    a = c(NA, 2L),
    c = c(NA, "greg")
  )
  expect_equal(test, df)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' List column with NULL for missing values (default)
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str('[{"a":10.1},{"a":"greg","b":4},{"b":4.1}]')
  df <- data.frame(
    a = NA,
    b = c(NA, 4.0, 4.1)
  )
  df$a <- list(10.1, "greg", NULL)
  expect_equal(test, df)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' List column with "NA" for missing values
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str('[{"a":10.1},{"a":"greg","b":4},{"b":4.1}]', missing_list_elem = 'na')
  test
  df <- data.frame(
    a = NA,
    b = c(NA, 4.0, 4.1)
  )
  df$a <- list(10.1, "greg", NA)
  expect_equal(test, df)

  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with explicit NULL
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str('[{"a":1,"b":2},{"a":2,"b":null}]')
  test  
  df <- data.frame(
    a = 1:2,
    b = c(2L, NA_integer_)
  )
  expect_equal(test, df)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with sub-data.frame
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str('[{"a":1,"b":2},{"a":2,"b":[{"c":100},{"c":101}]}]')
  test
  
  df <- data.frame(
    a = 1:2
  )
  df$b <- list(2L, data.frame(c = 100:101))
  expect_equal(test, df)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with sub list
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str('[{"a":1,"b":2},{"a":2,"b":{"c":100}}]')
  test
  
  df <- data.frame(
    a = 1:2
  )
  df$b <- list(2L, list(c = 100L))
  expect_equal(test, df)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with missing values
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str(r"(
[
  {"a":1, "b":2.2, "c":"greg", "d":null},
  {}
])")
  test
  df <- data.frame(
    a = c(1L, NA_integer_),
    b = c(2.2, NA_real_),
    c = c('greg', NA_character_)
  )
  df$d <- list(NULL, NULL)
  expect_equal(test, df)
  
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Data.frame with missing values
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  test <- read_json_str(r"(
[
  {"a":1, "b":2.2, "c":"greg", "d":null},
  {}
])", missing_list_elem = 'na')
  test
  df <- data.frame(
    a = c(1L, NA_integer_),
    b = c(2.2, NA_real_),
    c = c('greg', NA_character_)
  )
  df$d <- list(NULL, NA)
  expect_equal(test, df)
  
})


test_that("data.frame by column", {
  # Should look the same as a named list in JSON
  df <- data.frame(a = 1:2, b = c('greg', 'jim'))
  js <- write_json_str(df, dataframe = 'cols')  
  expect_equal(js, '{\"a\":[1,2],\"b\":[\"greg\",\"jim\"]}')
})


test_that("iris round trip works", {
  df <- head(iris)
  df$Species <- as.character(df$Species)  
  
  json <- write_json_str(df)
  df_out <- read_json_str(json)
  
  expect_identical(df, df_out)
})






