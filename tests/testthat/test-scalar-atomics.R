


test_that("as_scalar() works", {

  
  s <- write_json_str(list(
    my_scalar = as_scalar("foo"),               # will be output as scalar
    still_vec = as_scalar(c("foo", "bar")),     # no effect
    my_vector = "bar"                     ,     # will be output as 1-element array
    my_list   = as_scalar(list(a = 1, b = 'b')) # unchanged
  ))
  
  expect_identical(
    s,
    '{"my_scalar":"foo","still_vec":["foo","bar"],"my_vector":["bar"],"my_list":{"a":[1.0],"b":["b"]}}'
  )
    
  
  s <- write_json_str(list(
    my_scalar = as_scalar("foo"),           # will be output as scalar
    still_vec = as_scalar(c("foo", "bar")), # no effect
    my_vector = "bar"                     , # will be output as json []-array element
    my_list   = as_scalar(list(a = 1, b = 'b'))        #unchanged
  ), auto_unbox = TRUE)
    
})



if (FALSE) {
  
  s <- write_json_str(list(
    my_scalar = as_scalar("foo"),           # will be output as scalar
    still_vec = as_scalar(c("foo", "bar")), # no effect
    my_vector = "bar"                       # will be output as 1-element array
  ), pretty = TRUE)
  
  cat(s)
  
}