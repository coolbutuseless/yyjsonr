

js <- r"(
{
    "type": "FeatureCollection",
    "features": [
        {
            "type": "Feature",
            "geometry": {
                "type": "Point",
                "coordinates": [
                    -80.870885,
                    35.215151
                ]
            },
            "properties": {
                "value": 1.0
            }
        },
        {
            "type": "Feature",
            "geometry": {
                "type": "Point",
                "coordinates": [
                    -80.837753,
                    35.249801
                ]
            },
            "properties": {
                "value": "a"
            }
        }
    ]
} 
)"


  
test_that("multiplication works", {
  
  tst <- from_geojson_str(js) # geojson compat
  expect_identical(tst$value, c("1.000000", "a"))
  
  tst <- from_geojson_str(js, property_promotion = 'string') # geojson compat
  expect_identical(tst$value, c("1.000000", "a"))
  
  tst <- from_geojson_str(js, property_promotion = 'list')
  expect_identical(tst$value, list(1.0, "a"))
  
})
