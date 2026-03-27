

json <- r"(
{ 
// Application Settings 
app: { 
   name: 'My Application', 
   version: '2.1.0', 
   env: 'development', 
}, 
// Server Configuration 
server: { 
  host: 'localhost', 
  port: 3000, 
  https: false, 
}, 
// Database Connection 
database: { 
  driver: 'postgres', 
  host: 'localhost', 
  port: 5432, 
  name: 'myapp_dev', 
  pool: { min: 2, max: 10, }, 
}, 
// Logging 
logging: { 
  level: 'debug', 
  format: 'pretty', 
}, 
}
)"



test_that("json5 isn't the same as json", {
  expect_error(read_json_str(json))
})

test_that("json5 parsing works with options set", {
  res <- 
    read_json_str(json, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_JSON5)
  expect_true(is.list(res))
})
