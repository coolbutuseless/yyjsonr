test_that("write_json_str works", {

  # Scalar values  
  expect_equal(write_json_str(1), '[1.0]')
  expect_equal(write_json_str(1L), '[1]')
  expect_equal(write_json_str('a'), '["a"]')
  expect_equal(write_json_str(as.raw(1)), '[1]')
  expect_equal(write_json_str(as.Date("2020-01-01")), '["2020-01-01"]')
  expect_equal(write_json_str(as.Date(1 , origin = "1970-01-01")), '["1970-01-02"]')
  expect_equal(write_json_str(as.Date(1L, origin = "1970-01-01")), '["1970-01-02"]')
  expect_equal(write_json_str(bit64::as.integer64(2^32 + 1)), "[4294967297]")
  expect_equal(write_json_str(bit64::as.integer64(-(2^32 + 1))), "[-4294967297]")
  expect_equal(write_json_str(as.POSIXct("2020-01-01 01:02:03", tz = 'UTC')), '["2020-01-01 01:02:03"]')
    
  # Auto-unboxed scalar values
  expect_equal(write_json_str(1, auto_unbox = TRUE), '1.0')
  expect_equal(write_json_str(1L, auto_unbox = TRUE), '1')
  expect_equal(write_json_str('a', auto_unbox = TRUE), '"a"')
  expect_equal(write_json_str(as.raw(1), auto_unbox = TRUE), '1')
  expect_equal(write_json_str(as.Date("2020-01-01"), auto_unbox = TRUE), '"2020-01-01"')
  expect_equal(write_json_str(as.Date(1 , origin = "1970-01-01"), auto_unbox = TRUE), '"1970-01-02"')
  expect_equal(write_json_str(as.Date(1L, origin = "1970-01-01"), auto_unbox = TRUE), '"1970-01-02"')
  expect_equal(write_json_str(bit64::as.integer64(2^32 + 1), auto_unbox = TRUE), "4294967297")
  expect_equal(write_json_str(bit64::as.integer64(-(2^32 + 1)), auto_unbox = TRUE), "-4294967297")
  expect_equal(write_json_str(as.POSIXct("2020-01-01 01:02:03", tz = 'UTC'), auto_unbox = TRUE), '"2020-01-01 01:02:03"')
  
  # Scalar specials
  expect_equal(write_json_str(NA_real_), '[null]')
  expect_equal(write_json_str(NA_integer_), '[null]')
  expect_equal(write_json_str(NA_character_), '[null]')
  expect_equal(write_json_str(as.Date(NA, origin = "1970-01-01")), '[null]')
  expect_equal(write_json_str(as.Date(NA_real_, origin = "1970-01-01")), '[null]')
  expect_equal(write_json_str(as.Date(NA_integer_, origin = "1970-01-01")), '[null]')
  expect_equal(write_json_str(bit64::as.integer64(NA)), '[null]')
  expect_equal(write_json_str(as.POSIXct(NA, tz = 'UTC')), '[null]')
  expect_equal(write_json_str(as.POSIXct(NA_real_, tz = 'UTC')), '[null]')
  expect_equal(write_json_str(as.POSIXct(NA_integer_, tz = 'UTC')), '[null]')
  
  
  # Vectors values  
  expect_equal(write_json_str(c(1, 2, 3)), '[1.0,2.0,3.0]')
  expect_equal(write_json_str(c(1L, 2L, 3L)), '[1,2,3]')
  expect_equal(write_json_str(c('a', 'b', 'c')), '["a","b","c"]')
  expect_equal(write_json_str(as.raw(1:3)), '[1,2,3]')
  expect_equal(write_json_str(as.Date(c("2020-01-01", "2020-01-02"))), '["2020-01-01","2020-01-02"]')
  expect_equal(write_json_str(bit64::as.integer64(2^32 + 1:2)), "[4294967297,4294967298]")
  expect_equal(write_json_str(bit64::as.integer64(-(2^32 + 1:2))), "[-4294967297,-4294967298]")
  expect_equal(write_json_str(as.POSIXct(c("2020-01-01 01:02:03","2020-01-01 01:02:04"), tz = 'UTC')), '["2020-01-01 01:02:03","2020-01-01 01:02:04"]')
  
  # Lists
  expect_equal(write_json_str(list(a=1, b=2)), '{"a":[1.0],"b":[2.0]}')
  expect_equal(write_json_str(list(  1, b=2)), '{"":[1.0],"b":[2.0]}')
  expect_equal(write_json_str(list(  1, b=2), name_repair = 'minimal'), '{"1":[1.0],"b":[2.0]}')
  expect_equal(write_json_str(list(  1,   2)), '[[1.0],[2.0]]')
  expect_equal(write_json_str(list(a=1, b=list(c=3))), '{"a":[1.0],"b":{"c":[3.0]}}')
  expect_equal(write_json_str(list(a=1, b=list(c=3)), auto_unbox = TRUE), 
               '{"a":1.0,"b":{"c":3.0}}')
  
  # Vectors with specials
  
})


test_that("read_json_str works", {
  
  # Scalar values - in an array
  expect_equal(read_json_str('[1.0]'), 1)
  expect_equal(read_json_str('[1]'), 1L)
  expect_equal(read_json_str('["a"]'), 'a')
  expect_equal(read_json_str('["2020-01-01"]'), '2020-01-01')
  expect_identical(read_json_str("[4294967297]", int64 = 'bit64'), bit64::as.integer64(2^32 + 1))
  expect_equal(read_json_str("[4294967297]", int64 = 'string'), "4294967297")
  expect_equal(read_json_str('["2020-01-01 01:02:03"]'), "2020-01-01 01:02:03")
  
  # Scalar values  - raw
  expect_equal(read_json_str('1.0'), 1)
  expect_equal(read_json_str('1'), 1L)
  expect_equal(read_json_str('"a"'), 'a')
  expect_equal(read_json_str('"2020-01-01"'), '2020-01-01')
  expect_identical(read_json_str("4294967297", int64 = 'bit64'), bit64::as.integer64(2^32 + 1))
  expect_equal(read_json_str("4294967297", int64 = 'string'), "4294967297")
  expect_equal(read_json_str('"2020-01-01 01:02:03"'), "2020-01-01 01:02:03")
  
  
  # Simple vectors  
  expect_equal(read_json_str('[1.0,2.0]'), c(1,2))
  expect_equal(read_json_str('[1,2]'), c(1L, 2L))
  expect_equal(read_json_str('["a","b"]'), c('a', 'b'))
  expect_equal(read_json_str('["2020-01-01","2020-01-02"]'), c('2020-01-01', '2020-01-02'))
  expect_identical(read_json_str("[4294967297,4294967298]", int64 = 'bit64'), bit64::as.integer64(2^32 + 1:2))
  expect_equal(read_json_str("[4294967297,4294967298]", int64 = 'string'), c("4294967297","4294967298"))
  expect_equal(read_json_str('["2020-01-01 01:02:03","2020-01-01 01:02:04"]'), 
               c("2020-01-01 01:02:03", "2020-01-01 01:02:04"))
  
  
  # Simple vectors with JSON-null
  expect_equal(read_json_str('[1.1,null]'), c(1.1,NA_real_))
  expect_equal(read_json_str('[1,null]'), c(1L, NA_integer_))
  expect_equal(read_json_str('["a", null]'), c('a', NA_character_))
  expect_equal(read_json_str('["2020-01-01",null]'), c('2020-01-01', NA_character_))
  expect_identical(read_json_str("[4294967297,null]", int64 = 'bit64'), bit64::as.integer64(2^32 + c(1, NA)))
  expect_equal(read_json_str("[4294967297,null]", int64 = 'string'), c("4294967297",NA_character_))
  expect_equal(read_json_str('["2020-01-01 01:02:03",null]'), 
               c("2020-01-01 01:02:03", NA_character_))
  
  # Simple vectors with special-values-as-strings, with default convert_special_* handling
  # convert_special_dbl = TRUE
  # convert_special_int = TRUE
  # convert_special_str = FALSE
  expect_equal(read_json_str('[1.1,"greg"]'), list(1.1,"greg"))
  expect_equal(read_json_str('[1.1,"NA"]'), c(1.1,NA_real_))
  expect_equal(read_json_str('[1.1,"NaN"]'), c(1.1,NaN))
  expect_equal(read_json_str('[1.1,"Inf"]'), c(1.1,Inf))
  expect_equal(read_json_str('[1.1,"-Inf"]'), c(1.1,-Inf))
  expect_equal(read_json_str('[1,"NA"]'), c(1L, NA_integer_))
  expect_equal(read_json_str('["a", "NA"]'), c('a', "NA"))
  expect_equal(read_json_str('["2020-01-01","NA"]'), c('2020-01-01', "NA"))
  expect_identical(read_json_str('[4294967297,"NA"]', int64 = 'bit64'), bit64::as.integer64(2^32 + c(1, NA)))
  expect_equal(read_json_str('[4294967297,"NA"]', int64 = 'string'), c("4294967297","NA"))
  expect_equal(read_json_str('["2020-01-01 01:02:03","NA"]'), 
               c("2020-01-01 01:02:03", "NA"))
  
  
  # Simple vectors with special-values-as-strings, with default convert_special_* handling
  # convert_special_dbl = TRUE
  # convert_special_int = TRUE
  # convert_special_str = FALSE
  # In these cases, the vectors include a confounding type that promotes the 
  # first values to a different type by the end of the array.
  expect_equal(read_json_str('[1.1,2.2,"greg"]'), list(1.1,2.2,"greg"))
  expect_equal(read_json_str('[1.1,"hello","NA"]'), list(1.1, "hello", "NA"))
  expect_equal(read_json_str('[1.1,"hello", "NaN"]'), list(1.1, "hello", "NaN"))
  expect_equal(read_json_str('[1.1,1,"Inf"]'), c(1.1,1.0,Inf))
  expect_equal(read_json_str('[1.1,1,"-Inf"]'), c(1.1, 1, -Inf))
  expect_equal(read_json_str('[1, 4294967297, "NA"]', int64 = 'bit64'), bit64::as.integer64(c(1, 4294967297, NA)))
  expect_equal(read_json_str('[1, 4294967297, "NA"]'), list(1L, "4294967297", "NA"))
  expect_equal(read_json_str('["a", 1.3, "NA"]'), list('a', 1.3, "NA"))
  
  
  # {}-Object with scalar values  
  expect_equal(read_json_str('{"a":1.0}'), list(a=1))
  expect_equal(read_json_str('{"a":1}'), list(a=1L))
  expect_equal(read_json_str('{"a":"a"}'), list(a='a'))
  expect_equal(read_json_str('{"a":"2020-01-01"}'), list(a='2020-01-01'))
  expect_identical(read_json_str('{"a":4294967297}', int64 = 'bit64'), list(a=bit64::as.integer64(2^32 + 1)))
  expect_equal(read_json_str('{"a":4294967297}', int64 = 'string'), list(a="4294967297"))
  expect_equal(read_json_str('{"a":"2020-01-01 01:02:03"}'), list(a="2020-01-01 01:02:03"))
  
  
  # {}-object with imple vectors  
  expect_equal(read_json_str('{"a":[1.0,2.0]}'), list(a=c(1,2)))
  expect_equal(read_json_str('{"a":[1,2]}'), list(a=c(1L, 2L)))
  expect_equal(read_json_str('{"a":["a","b"]}'), list(a=c('a', 'b')))
  expect_equal(read_json_str('{"a":["2020-01-01","2020-01-02"]}'), list(a=c('2020-01-01', '2020-01-02')))
  expect_identical(read_json_str('{"a":[4294967297,4294967298]}', int64 = 'bit64'), list(a=bit64::as.integer64(2^32 + 1:2)))
  expect_equal(read_json_str('{"a":[4294967297,4294967298]}', int64 = 'string'), list(a=c("4294967297","4294967298")))
  expect_equal(read_json_str('{"a":["2020-01-01 01:02:03","2020-01-01 01:02:04"]}'), 
               list(a=c("2020-01-01 01:02:03", "2020-01-01 01:02:04")))
})
