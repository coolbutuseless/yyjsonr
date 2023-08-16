
test_that("to_json handling specials works", {

  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Special Logical
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    to_json_str(c(TRUE, NA)),
    '[true,null]'
  )
  
  expect_identical(
    to_json_str(c(TRUE, NA), opts = to_opts(num_specials = 'string')),
    '[true,"NA"]'
  )
  
  expect_identical(
    to_json_str(c(TRUE, NA), opts = to_opts(num_specials = 'null')),
    '[true,null]'
  )
  
    
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Special INTs
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    to_json_str(c(0L, NA_integer_)),
    '[0,null]'
  )
  
  expect_identical(
    to_json_str(c(0L, NA_integer_), opts = to_opts(num_specials = 'string')),
    '[0,"NA"]'
  )
  
  expect_identical(
    to_json_str(c(0L, NA_integer_), opts = to_opts(num_specials = 'null')),
    '[0,null]'
  )
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Special STRING
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    to_json_str(c("a", NA_character_)),
    '["a",null]'
  )
  
  expect_identical(
    to_json_str(c("a", NA_character_), opts = to_opts(str_specials = 'string')),
    '["a","NA"]'
  )
  
  expect_identical(
    to_json_str(c("a", NA_character_), opts = to_opts(str_specials = 'null')),
    '["a",null]'
  )
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Special Numeric
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    to_json_str(c(0, NA_real_, Inf, -Inf, NaN)),
    '[0.0,null,null,null,null]'
  )
  
  expect_identical(
    to_json_str(c(0, NA_real_, Inf, -Inf, NaN), opts = to_opts(num_specials = 'string')),
    '[0.0,"NA","Inf","-Inf","NaN"]'
  )
  
  expect_identical(
    to_json_str(c(0, NA_real_, Inf, -Inf, NaN), opts = to_opts(num_specials = 'null')),
    '[0.0,null,null,null,null]'
  )
})


test_that("from_json handling specials works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Logical
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    from_json_str('[true,"NA"]'),
    c(TRUE, NA)
  )  
  
  expect_identical(
    from_json_str('[true,"NA"]', opts = from_opts(num_specials = "special")),
    c(TRUE, NA)
  )  
  
  expect_identical(
    from_json_str('[true,"NA"]', opts = from_opts(num_specials = "string")),
    list(TRUE, "NA")
  )  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Integer
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    from_json_str('[0, "NA"]'),
    c(0L, NA_integer_)
  )  
  
  expect_identical(
    from_json_str('[0, "NA"]', opts = from_opts(num_specials = "special")),
    c(0L, NA_integer_)
  )  
  
  expect_identical(
    from_json_str('[0, "NA"]', opts = from_opts(num_specials = "string")),
    list(0L, "NA")
  )  
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Numeric
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    from_json_str('[0.0, "NA", "Inf", "-Inf", "NaN"]'),
    c(0, NA_real_, Inf, -Inf, NaN)
  )  
  
  expect_identical(
    from_json_str('[0.0, "NA", "Inf", "-Inf", "NaN"]', opts = from_opts(num_specials = "special")),
    c(0, NA_real_, Inf, -Inf, NaN)
  )  
  
  expect_identical(
    from_json_str('[0.0, "NA", "Inf", "-Inf", "NaN"]', opts = from_opts(num_specials = "string")),
    list(0, "NA", "Inf", "-Inf", "NaN")
  )  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # String
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_identical(
    from_json_str('["a", "NA"]'),
    c("a", "NA")
  )
  
  expect_identical(
    from_json_str('["a", "NA"]', opts = from_opts(str_specials = "string")),
    c("a", "NA")
  )
  
  expect_identical(
    from_json_str('["a", "NA"]', opts = from_opts(str_specials = "special")),
    c("a", NA_character_)
  )
  
  
})










