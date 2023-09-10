

ref <- "[{\"Sepal.Length\":5.1,\"Sepal.Width\":3.5,\"Petal.Length\":1.4,\"Petal.Width\":0.2,\"Species\":\"setosa\"},{\"Sepal.Length\":4.9,\"Sepal.Width\":3.0,\"Petal.Length\":1.4,\"Petal.Width\":0.2,\"Species\":\"setosa\"},{\"Sepal.Length\":4.7,\"Sepal.Width\":3.2,\"Petal.Length\":1.3,\"Petal.Width\":0.2,\"Species\":\"setosa\"},{\"Sepal.Length\":4.6,\"Sepal.Width\":3.1,\"Petal.Length\":1.5,\"Petal.Width\":0.2,\"Species\":\"setosa\"}]"
ref <- '[
  {
    "Sepal.Length": 5.1,
    "Sepal.Width": 3.5,
    "Petal.Length": 1.4,
    "Petal.Width": 0.2,
    "Species": "setosa"
  },
  {
    "Sepal.Length": 4.9,
    "Sepal.Width": 3.0,
    "Petal.Length": 1.4,
    "Petal.Width": 0.2,
    "Species": "setosa"
  },
  {
    "Sepal.Length": 4.7,
    "Sepal.Width": 3.2,
    "Petal.Length": 1.3,
    "Petal.Width": 0.2,
    "Species": "setosa"
  },
  {
    "Sepal.Length": 4.6,
    "Sepal.Width": 3.1,
    "Petal.Length": 1.5,
    "Petal.Width": 0.2,
    "Species": "setosa"
  }
]'


test_that("serialization works", {
  expect_equal(write_json_str(head(iris, 4), pretty = TRUE), ref)
})
