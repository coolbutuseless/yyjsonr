
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


test_that("geojson digits works", {
  
  x <- read_geojson_str(js)
  
  js2 <- write_geojson_str(x, digits = 0)
  
  x2 <- read_geojson_str(js2)
  
  expect_equal(unclass(x2$geometry[[1]]), c(-81, 35))
  expect_equal(unclass(x2$geometry[[2]]), c(-81, 35))
  
    
})
