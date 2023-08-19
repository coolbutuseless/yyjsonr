yyjsonr
================

<!-- README.md is generated from README.Rmd. Please edit that file -->

# yyjsonr <img src="man/figures/logo.png" align="right" width = "20%"/>

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/yyjsonr/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/yyjsonr/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`{yyjsonr}` is a fast JSON parser/serializer, which converts R data
to/from JSON and NDJSON.

In most cases it is around 2x to 10x faster than `{jsonlite}` at both
reading and writing JSON.

It is based around the [`yyjson`](https://github.com/ibireme/yyjson) C
library.

### Help needed!

If you have an interest in fast JSON reading/writing in R, then **I need
your help**.

The scope of this package and options it supports are still in flux.
What can I add to help your JSON needs? Open an issue on github and let
me know!

You can assist by:

- Finding cases that give unexpected results for your JSON needs
- Suggesting how you want to have a particular JSON object to appear in
  R
- Propose configuration options to control a particular aspect of how
  *you* want JSON to translate to/from R
- Trying it out in your package as an alternative to your current JSON
  package.
  - Is it worth the change?
  - What functionality is lacking?
  - What would you need to make the switch proper?
- Suggesting additional interesting benchmarks/tests.
- Creating/Donating a nice hex logo!

### The `yyjson` C library

This package includes the [`yyjson`](https://github.com/ibireme/yyjson)
C library (version `YYJSON_VERSION_HEX = 0x000700`).

`yysjon` is MIT licensed - see `LICENSE-yyjson.txt` in this package for
more details.

### What’s in the box

- Write R objects to JSON
  - `to_json_str()`, `to_json_file()`
- Read in JSON as R objects
  - `from_json_str()`, `from_json_file()`, `from_json_conn()`,
    `from_json_raw()`
- Read in NDJSON
  - `from_ndjson_file_as_list()`, `from_ndjson_file_as_df()`
- `to_opts()`, `from_opts()` construct configuration options for
  reading/writing JSON

### Comparison to other JSON packages

|              | R to JSON | JSON to R | ndjson read | ndjson write | geojson to `{sf}` |
|--------------|-----------|-----------|-------------|--------------|-------------------|
| yyjsonr      | Fast!     | Fast!     | Fast!       | Not yet      | In progress       |
| jsonlite     | Yes       | Yes       | Yes         | Yes          |                   |
| RcppSimdJson |           | Fast!     |             |              |                   |
| jsonify      | Yes       | Yes       | Yes         | Yes          |                   |
| ndjson       |           |           | Yes         | Yes          |                   |
| geojsonsf    |           |           |             |              | Yes               |

## Installation

You can install from [GitHub](https://github.com/coolbutuseless/yyjsonr)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/yyjsonr')
```

# R to JSON string

``` r
str <- to_json_str(head(iris, 3), pretty = TRUE)
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

from_json_str(str)
#>   Sepal.Length Sepal.Width Petal.Length Petal.Width Species
#> 1          5.1         3.5          1.4         0.2  setosa
#> 2          4.9         3.0          1.4         0.2  setosa
#> 3          4.7         3.2          1.3         0.2  setosa
```

## Benchmark: R to JSON string

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
res <- bench::mark(
  jsonlite::toJSON(iris),
  jsonify::to_json(iris),
  yyjsonr::to_json_str(iris),
  check = FALSE
)
#> Registered S3 method overwritten by 'jsonify':
#>   method     from    
#>   print.json jsonlite

knitr::kable(res[,1:5])
```

| expression                 |     min |  median |   itr/sec | mem_alloc |
|:---------------------------|--------:|--------:|----------:|----------:|
| jsonlite::toJSON(iris)     | 323.7µs | 344.4µs |  2759.951 |    2.17MB |
| jsonify::to_json(iris)     | 235.3µs | 246.5µs |  3971.252 |    4.91MB |
| yyjsonr::to_json_str(iris) |  42.9µs |  45.6µs | 21350.904 |        0B |

</details>

<img src="man/figures/README-unnamed-chunk-4-1.png" width="100%" />

## Benchmark: JSON string to R

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
json_str <- to_json_str(iris)
res <- bench::mark(
  jsonlite::fromJSON(json_str),
  jsonify::from_json(json_str),
  RcppSimdJson::fparse(json_str),
  yyjsonr::from_json_str(json_str),
  check = FALSE
)
knitr::kable(res[,1:5])
```

| expression                       |     min |  median |   itr/sec | mem_alloc |
|:---------------------------------|--------:|--------:|----------:|----------:|
| jsonlite::fromJSON(json_str)     | 470.9µs | 500.4µs |  1981.143 |  427.19KB |
| jsonify::from_json(json_str)     | 711.8µs | 738.9µs |  1331.108 |    36.2KB |
| RcppSimdJson::fparse(json_str)   |  47.6µs |  49.7µs | 19714.396 |  107.09KB |
| yyjsonr::from_json_str(json_str) |  30.2µs |  31.1µs | 31244.556 |    6.09KB |

</details>

<img src="man/figures/README-unnamed-chunk-6-1.png" width="100%" />

# R to JSON file

``` r
to_json_file(iris, tempfile())
```

## Benchmark: R to JSON file

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
json_file <- tempfile()
res <- bench::mark(
  jsonlite::write_json(iris, json_file),
  yyjsonr::to_json_file(iris, json_file),
  check = FALSE
)

knitr::kable(res[, 1:5])
```

| expression                             |     min | median |   itr/sec | mem_alloc |
|:---------------------------------------|--------:|-------:|----------:|----------:|
| jsonlite::write_json(iris, json_file)  | 385.5µs |  407µs |  2413.218 |    48.8KB |
| yyjsonr::to_json_file(iris, json_file) |  67.3µs | 75.5µs | 13023.221 |    7.06KB |

</details>

<img src="man/figures/README-unnamed-chunk-10-1.png" width="100%" />

## Benchmark: JSON file to R

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
json_file <- tempfile()
jsonlite::write_json(iris, json_file)
res <- bench::mark(
  jsonlite::fromJSON(file(json_file)), 
  RcppSimdJson::fload(json_file), 
  yyjsonr::from_json_file(json_file),
  check = TRUE
)

knitr::kable(res[, 1:5])
```

| expression                          |     min |  median |   itr/sec | mem_alloc |
|:------------------------------------|--------:|--------:|----------:|----------:|
| jsonlite::fromJSON(file(json_file)) | 515.5µs | 549.6µs |  1788.843 |   145.9KB |
| RcppSimdJson::fload(json_file)      |  78.4µs |  83.6µs | 11070.682 |   136.4KB |
| yyjsonr::from_json_file(json_file)  |  40.5µs |  41.9µs | 23295.255 |    12.1KB |

</details>

<img src="man/figures/README-unnamed-chunk-12-1.png" width="100%" />

# Parsing ndjson

`ndjson` is “newline delimited json” which is multiple json strings in a
file, with each string separated by a newline. This is a convenient
storage method for lots of similarly structured objects e.g. log output.

There are options to read only a subset of lines from the ndjson file
(`nskip` and `nread`) - which can lead to time savings if only a subset
of lines are needed.

``` r
ndjson_file <- tempfile()

{
  # Setup an 'ndjson' file to read back in
  df <- head( nycflights13::flights[, 1:5], 2)
  jsonlite::stream_out(df, file(ndjson_file), verbose = FALSE)
}

from_ndjson_file_as_df(ndjson_file)
#>   year month day dep_time sched_dep_time
#> 1 2013     1   1      517            515
#> 2 2013     1   1      533            529
from_ndjson_file_as_list(ndjson_file)
#> [[1]]
#> [[1]]$year
#> [1] 2013
#> 
#> [[1]]$month
#> [1] 1
#> 
#> [[1]]$day
#> [1] 1
#> 
#> [[1]]$dep_time
#> [1] 517
#> 
#> [[1]]$sched_dep_time
#> [1] 515
#> 
#> 
#> [[2]]
#> [[2]]$year
#> [1] 2013
#> 
#> [[2]]$month
#> [1] 1
#> 
#> [[2]]$day
#> [1] 1
#> 
#> [[2]]$dep_time
#> [1] 533
#> 
#> [[2]]$sched_dep_time
#> [1] 529
```

## Benchmark: Parsing ndjson

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Set-up benchmark data
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ndjson_filename <- tempfile()
df <- head( nycflights13::flights, 1000)
jsonlite::stream_out(df, file(ndjson_filename), verbose = FALSE)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' benchmark
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
res <- bench::mark(
  ndjson::stream_in(ndjson_filename),
  jsonlite::stream_in(file(ndjson_filename), verbose = FALSE),
  jsonify::from_ndjson(ndjson_filename),
  yyjsonr::from_ndjson_file_as_list(ndjson_filename),
  yyjsonr::from_ndjson_file_as_df  (ndjson_filename),
  check = FALSE
)

knitr::kable(res[, 1:5])
```

| expression                                                  |      min |  median |   itr/sec | mem_alloc |
|:------------------------------------------------------------|---------:|--------:|----------:|----------:|
| ndjson::stream_in(ndjson_filename)                          |  14.48ms |  15.2ms |  66.03189 |    3.62MB |
| jsonlite::stream_in(file(ndjson_filename), verbose = FALSE) |  21.61ms | 22.39ms |  44.47338 |    1.26MB |
| jsonify::from_ndjson(ndjson_filename)                       |  11.83ms | 12.03ms |  82.67475 |  568.56KB |
| yyjsonr::from_ndjson_file_as_list(ndjson_filename)          |   1.39ms |   1.5ms | 660.71060 |  404.22KB |
| yyjsonr::from_ndjson_file_as_df(ndjson_filename)            | 998.06µs |  1.04ms | 905.43215 |   95.03KB |

</details>

<img src="man/figures/README-unnamed-chunk-15-1.png" width="100%" />

# More Benchmarks

## Benchmark from `{RcppSimdJson}`

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
jsonfile <- system.file("jsonexamples", "twitter.json", package="RcppSimdJson")
json <- paste(readLines(jsonfile), collapse = "\n")

res <- bench::mark(
  jsonlite::fromJSON(json),
  RcppSimdJson::fparse(json),
  yyjsonr::from_json_str(json),
  check = FALSE
)

knitr::kable(res[, 1:5])
```

| expression                   |    min |  median |   itr/sec | mem_alloc |
|:-----------------------------|-------:|--------:|----------:|----------:|
| jsonlite::fromJSON(json)     | 22.7ms | 23.27ms |  42.05495 |     832KB |
| RcppSimdJson::fparse(json)   |    2ms |   2.1ms | 465.05409 |     182KB |
| yyjsonr::from_json_str(json) | 1.19ms |  1.22ms | 805.67069 |     214KB |

</details>

<img src="man/figures/README-unnamed-chunk-17-1.png" width="100%" />

## Benchmark from `{jsonify}`

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
n <- 1e5
df <- data.frame(
  id = 1:n
  , value = sample(letters, size = n, replace = T)
  , val2 = rnorm(n = n)
  , log = sample(c(T,F), size = n, replace = T)
  , stringsAsFactors = FALSE
)

res <- bench::mark(
  jsonlite::toJSON( df ),
  jsonify::to_json( df ),
  yyjsonr::to_json_str( df ),
  check = FALSE
)
#> Warning: Some expressions had a GC in every iteration; so filtering is
#> disabled.

knitr::kable(res[,1:5])
```

| expression               |     min |  median |  itr/sec | mem_alloc |
|:-------------------------|--------:|--------:|---------:|----------:|
| jsonlite::toJSON(df)     |  59.6ms |  70.7ms | 14.17483 |   20.81MB |
| jsonify::to_json(df)     | 105.5ms | 111.6ms |  7.88904 |     8.3MB |
| yyjsonr::to_json_str(df) |  18.4ms |  19.3ms | 50.91636 |    6.01MB |

</details>

<img src="man/figures/README-unnamed-chunk-19-1.png" width="100%" />

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
n <- 1e4
x <- list(
  x = rnorm(n = n)
  , y = list(x = rnorm(n = n))
  , z = list( list( x = rnorm(n = n)))
  , xx = rnorm(n = n)
  , yy = data.frame(
      id = 1:n
      , value = sample(letters, size = n, replace = T)
      , val2 = rnorm(n = n)
      , log = sample(c(T,F), size = n, replace = T)
    )
)

res <- bench::mark(
 jsonlite::toJSON( x ),
 jsonify::to_json( x ),
 yyjsonr::to_json_str(x),
 check = FALSE
)


knitr::kable(res[,1:5])
```

| expression              |   min |  median |   itr/sec | mem_alloc |
|:------------------------|------:|--------:|----------:|----------:|
| jsonlite::toJSON(x)     | 9.4ms |  9.61ms | 102.28295 |    3.34MB |
| jsonify::to_json(x)     |  13ms | 13.44ms |  73.93904 |    1.88MB |
| yyjsonr::to_json_str(x) | 3.4ms |  3.57ms | 277.51309 |    1.34MB |

</details>

<img src="man/figures/README-unnamed-chunk-21-1.png" width="100%" />

<details>
<summary>
Show/Hide benchmark code
</summary>

``` r
jlt <- jsonlite::toJSON( x )

res <- bench::mark(
  jsonlite::fromJSON( jlt ),
  jsonify::from_json( jlt ),
  yyjsonr::from_json_str(jlt),
  check = FALSE
)

knitr::kable(res[,1:5])
```

| expression                  |     min |  median |   itr/sec | mem_alloc |
|:----------------------------|--------:|--------:|----------:|----------:|
| jsonlite::fromJSON(jlt)     | 32.91ms | 33.69ms |  29.54601 |    2.37MB |
| jsonify::from_json(jlt)     | 32.13ms | 32.26ms |  31.00022 |    1.34MB |
| yyjsonr::from_json_str(jlt) |  1.82ms |  1.93ms | 512.44484 |  547.25KB |

</details>

<img src="man/figures/README-unnamed-chunk-23-1.png" width="100%" />

# Parsing differences compared to `{jsonlite}`

## Numeric types retained in presence of other strings

`{yyjsonr}` does not promote numeric values in arrays to strings if the
array contains a string. Instead the R container is promoted to a
`list()` in order to retain original types.

Note: this could be controlled by a flag if desired. Open an issue and
let me know what you need!

``` r
json <- '[1,2,3,"apple"]'
jsonlite::fromJSON(json)
#> [1] "1"     "2"     "3"     "apple"
yyjsonr::from_json_str(json)
#> [[1]]
#> [1] 1
#> 
#> [[2]]
#> [1] 2
#> 
#> [[3]]
#> [1] 3
#> 
#> [[4]]
#> [1] "apple"
```

## 3-d arrays are parsed as multiple 2-d matrices and combined

In `{yyjsonr}` the order in which elements in an array are serialized to
JSON correspond to an array of row-major matrices in human-readable
order.

`{jsonlite}` does things differently. The array formats are internally
consistent within each package, but not cross-compatible between them
i.e. you cannot serialize an array in `{yyjsonr}` and re-create it
exactly using `{jsonlite}`.

``` r
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

str <- jsonlite::toJSON(mat)
str
#> [[[1,7],[3,9],[5,11]],[[2,8],[4,10],[6,12]]]
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


str <- yyjsonr::to_json_str(mat)
str
#> [1] "[[[1,3,5],[2,4,6]],[[7,9,11],[8,10,12]]]"
yyjsonr::from_json_str(str)
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

## Limitiations

- Some datatypes not currently supported:
  - Complex numbers
  - POSIXlt
  - Matrices of POSIXct / Date

## Acknowledgements

- R Core for developing and maintaining the language.
- CRAN maintainers, for patiently shepherding packages onto CRAN and
  maintaining the repository
