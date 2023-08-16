
test_that("parsing json from connection works", {
  
  js1 <- from_json_file("examples/test.json")
  
  con <- gzfile("examples/test.json.gz")
  js2 <- from_json_conn(gzfile("examples/test.json.gz"))
  close.connection(con)
  
  expect_identical(js1, js2)
})
