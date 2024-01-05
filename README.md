yyjsonr
================

<!-- README.md is generated from README.Rmd. Please edit that file -->

# yyjsonr <img src="man/figures/logo.png" align="right" width = "20%"/>

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/yyjsonr/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/yyjsonr/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`{yyjsonr}` is a fast JSON parser/serializer, which converts R data
to/from JSON.

In most cases it is around 2x to 10x faster than `{jsonlite}` at both
reading and writing JSON.

It is a wrapper for the [`yyjson`](https://github.com/ibireme/yyjson) C
library.

### The `yyjson` C library

This package includes the [`yyjson`](https://github.com/ibireme/yyjson)
C library (version `YYJSON_VERSION_HEX = 0x000700`).

`yysjon` is MIT licensed - see `LICENSE-yyjson.txt` in this package for
more details.

### What’s in the box

|      |          | string              | file                 | raw             | conn             | options           |
|------|----------|---------------------|----------------------|-----------------|------------------|-------------------|
| json | read     | read_json_str()     | read_json_file()     | read_json_raw() | read_json_conn() | opts_read_json()  |
|      | write    | write_json_str()    | write_json_file()    |                 |                  | opts_write_json() |
|      | validate | validate_json_str() | validate_json_file() |                 |                  |                   |

### Comparison to other JSON packages

|              | Write JSON | Read JSON |
|--------------|------------|-----------|
| yyjsonr      | Fast!      | Fast!     |
| jsonlite     | Yes        | Yes       |
| RcppSimdJson |            | Fast!     |
| jsonify      | Yes        | Yes       |

<img src="man/figures/benchmark-summary.png">

Note: Benchmarks were run on Apple M2 Mac. See file
“man/benchmark/benchmark.Rmd” for details.

## Installation

You can install from [GitHub](https://github.com/coolbutuseless/yyjsonr)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/yyjsonr')
```

# Simple usage example

``` r
library(yyjsonr)

str <- write_json_str(head(iris, 3), pretty = TRUE)
cat(str)
#> [
#>   {
#>     "Sepal.Length": 5.1,
#>     "Sepal.Width": 3.5,
#>     "Petal.Length": 1.4,
#>     "Petal.Width": 0.2,
#>     "Species": "setosa"
#>   },
#>   {
#>     "Sepal.Length": 4.9,
#>     "Sepal.Width": 3.0,
#>     "Petal.Length": 1.4,
#>     "Petal.Width": 0.2,
#>     "Species": "setosa"
#>   },
#>   {
#>     "Sepal.Length": 4.7,
#>     "Sepal.Width": 3.2,
#>     "Petal.Length": 1.3,
#>     "Petal.Width": 0.2,
#>     "Species": "setosa"
#>   }
#> ]

read_json_str(str)
#>   Sepal.Length Sepal.Width Petal.Length Petal.Width Species
#> 1          5.1         3.5          1.4         0.2  setosa
#> 2          4.9         3.0          1.4         0.2  setosa
#> 3          4.7         3.2          1.3         0.2  setosa
```

# Parsing differences compared to `{jsonlite}`

## 3-d arrays are parsed as multiple 2-d matrices and combined

In `{yyjsonr}` the order in which elements in an array are serialized to
JSON correspond to an array of row-major matrices in human-readable
order.

`{jsonlite}` does things differently.

The array formats are internally consistent within each package, but not
cross-compatible between them i.e. you cannot serialize an array in
`{yyjsonr}` and re-create it exactly using `{jsonlite}`.

``` r
# A simple 3D array 
mat <- array(1:12, dim = c(2,3,2))
mat
#> , , 1
#> 
#>      [,1] [,2] [,3]
#> [1,]    1    3    5
#> [2,]    2    4    6
#> 
#> , , 2
#> 
#>      [,1] [,2] [,3]
#> [1,]    7    9   11
#> [2,]    8   10   12
```

``` r
# jsonlite's serialization of matrices is internally consistent and re-parses
# to the initial matrix.
str <- jsonlite::toJSON(mat, pretty = TRUE)
cat(str)
#> [
#>   [
#>     [1, 7],
#>     [3, 9],
#>     [5, 11]
#>   ],
#>   [
#>     [2, 8],
#>     [4, 10],
#>     [6, 12]
#>   ]
#> ]
jsonlite::fromJSON(str)
#> , , 1
#> 
#>      [,1] [,2] [,3]
#> [1,]    1    3    5
#> [2,]    2    4    6
#> 
#> , , 2
#> 
#>      [,1] [,2] [,3]
#> [1,]    7    9   11
#> [2,]    8   10   12
```

``` r
# yyjsonr's serialization of matrices is internally consistent and re-parses
# to the initial matrix.
# But note that it is *different* to what jsonlite does.
str <- yyjsonr::write_json_str(mat, pretty = TRUE)
cat(str)
#> [
#>   [
#>     [
#>       1,
#>       3,
#>       5
#>     ],
#>     [
#>       2,
#>       4,
#>       6
#>     ]
#>   ],
#>   [
#>     [
#>       7,
#>       9,
#>       11
#>     ],
#>     [
#>       8,
#>       10,
#>       12
#>     ]
#>   ]
#> ]
yyjsonr::read_json_str(str)
#> , , 1
#> 
#>      [,1] [,2] [,3]
#> [1,]    1    3    5
#> [2,]    2    4    6
#> 
#> , , 2
#> 
#>      [,1] [,2] [,3]
#> [1,]    7    9   11
#> [2,]    8   10   12
```

## Future

- Re-introduce NDJSON support
  - NDJSON support was removed for the initial CRAN release for the sake
    of my sanity.
  - See the `ndjson` branch of this repository
- Re-introduce GeoJSON support
  - GeoJSON support was removed for the initial CRAN release for the
    sake of my sanity.
  - See the `geojson` branch of this repository

## Limitiations

- Some datatypes not currently supported. Please file an issue on github
  if these types are critical for yoy:
  - Complex numbers
  - POSIXlt
  - Matrices of POSIXct / Date

## Acknowledgements

- R Core for developing and maintaining the language.
- CRAN maintainers, for patiently shepherding packages onto CRAN and
  maintaining the repository
