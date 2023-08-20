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
| yyjsonr      | Fast!     | Fast!     | Fast!       | Fast!        | In progress       |
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

| expression                 |      min |   median |   itr/sec | mem_alloc |
|:---------------------------|---------:|---------:|----------:|----------:|
| jsonlite::toJSON(iris)     |   1.11ms |   1.17ms |  846.8955 |    1.14MB |
| jsonify::to_json(iris)     | 519.23µs | 564.41µs | 1716.7874 |    4.92MB |
| yyjsonr::to_json_str(iris) |  81.79µs |  98.78µs | 9793.0623 |        0B |

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

| expression                       |      min |   median |    itr/sec | mem_alloc |
|:---------------------------------|---------:|---------:|-----------:|----------:|
| jsonlite::fromJSON(json_str)     |   1.16ms |   1.27ms |   766.8049 |  224.58KB |
| jsonify::from_json(json_str)     |   1.63ms |   1.83ms |   544.5950 |    36.2KB |
| RcppSimdJson::fparse(json_str)   | 132.76µs | 144.05µs |  6693.4783 |  107.09KB |
| yyjsonr::from_json_str(json_str) |  56.72µs |   60.9µs | 15715.9177 |    6.09KB |

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

| expression                             |      min | median |   itr/sec | mem_alloc |
|:---------------------------------------|---------:|-------:|----------:|----------:|
| jsonlite::write_json(iris, json_file)  |   1.41ms | 1.49ms |  654.4262 |   42.84KB |
| yyjsonr::to_json_file(iris, json_file) | 255.04µs |  293µs | 3347.8006 |    7.06KB |

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

| expression                          |      min |  median |    itr/sec | mem_alloc |
|:------------------------------------|---------:|--------:|-----------:|----------:|
| jsonlite::fromJSON(file(json_file)) |   1.33ms |  1.39ms |   693.5924 |   137.8KB |
| RcppSimdJson::fload(json_file)      | 242.12µs | 256.8µs |  3729.1160 |   136.4KB |
| yyjsonr::from_json_file(json_file)  |  88.41µs | 89.99µs | 10642.3835 |    12.1KB |

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

## Benchmark: Parsing from ndjson

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

| expression                                                  |     min |  median |   itr/sec | mem_alloc |
|:------------------------------------------------------------|--------:|--------:|----------:|----------:|
| ndjson::stream_in(ndjson_filename)                          | 30.76ms | 32.78ms |  30.61573 |    3.62MB |
| jsonlite::stream_in(file(ndjson_filename), verbose = FALSE) | 55.83ms | 57.45ms |  17.43018 |    1.22MB |
| jsonify::from_ndjson(ndjson_filename)                       | 24.35ms | 25.14ms |  39.71336 |  568.56KB |
| yyjsonr::from_ndjson_file_as_list(ndjson_filename)          |  3.33ms |  3.68ms | 272.81654 |  404.22KB |
| yyjsonr::from_ndjson_file_as_df(ndjson_filename)            |  1.84ms |  2.31ms | 405.79201 |   95.03KB |

</details>

<img src="man/figures/README-unnamed-chunk-15-1.png" width="100%" />

## Benchmark: Writing to ndjson file

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

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' benchmark
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
res <- bench::mark(
  jsonlite = jsonlite::stream_out(df, file(ndjson_filename), verbose = FALSE),
  yyjsonr  = yyjsonr::to_ndjson_file(df, ndjson_filename),
  check = FALSE
)

knitr::kable(res[, 1:5])
```

| expression |     min |  median |   itr/sec | mem_alloc |
|:-----------|--------:|--------:|----------:|----------:|
| jsonlite   | 13.03ms | 14.22ms |  70.11931 |    1.16MB |
| yyjsonr    |  4.68ms |  5.21ms | 190.89286 |   12.97KB |

</details>

<img src="man/figures/README-unnamed-chunk-17-1.png" width="100%" />

## Benchmark: Writing to ndjson string

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

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' benchmark
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
res <- bench::mark(
  jsonify  = jsonify::to_ndjson(df),
  yyjsonr  = yyjsonr::to_ndjson_str(df),
  check = FALSE
)

knitr::kable(res[, 1:5])
```

| expression |     min |  median |   itr/sec | mem_alloc |
|:-----------|--------:|--------:|----------:|----------:|
| jsonify    | 15.03ms | 15.74ms |  62.79824 |     309KB |
| yyjsonr    |  3.48ms |  3.74ms | 260.72084 |     314KB |

</details>

<img src="man/figures/README-unnamed-chunk-19-1.png" width="100%" />

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

| expression                   |     min |  median |   itr/sec | mem_alloc |
|:-----------------------------|--------:|--------:|----------:|----------:|
| jsonlite::fromJSON(json)     | 58.74ms | 60.54ms |  16.58897 |     818KB |
| RcppSimdJson::fparse(json)   |  4.38ms |  4.73ms | 207.77343 |     182KB |
| yyjsonr::from_json_str(json) |  2.61ms |  3.12ms | 327.18385 |     214KB |

</details>

<img src="man/figures/README-unnamed-chunk-21-1.png" width="100%" />

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

| expression               |     min |  median |   itr/sec | mem_alloc |
|:-------------------------|--------:|--------:|----------:|----------:|
| jsonlite::toJSON(df)     | 159.4ms | 159.9ms |  6.247967 |   20.81MB |
| jsonify::to_json(df)     | 256.9ms | 271.2ms |  3.687766 |     8.3MB |
| yyjsonr::to_json_str(df) |  40.9ms |  43.6ms | 22.978469 |    6.01MB |

</details>

<img src="man/figures/README-unnamed-chunk-23-1.png" width="100%" />

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

| expression              |     min |  median |   itr/sec | mem_alloc |
|:------------------------|--------:|--------:|----------:|----------:|
| jsonlite::toJSON(x)     | 25.18ms | 29.02ms |  34.56706 |    3.33MB |
| jsonify::to_json(x)     |  27.3ms | 28.77ms |  34.50968 |    1.88MB |
| yyjsonr::to_json_str(x) |  6.58ms |  7.64ms | 130.15639 |    1.34MB |

</details>

<img src="man/figures/README-unnamed-chunk-25-1.png" width="100%" />

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
| jsonlite::fromJSON(jlt)     | 81.97ms |  83.8ms |  11.97260 |    2.37MB |
| jsonify::from_json(jlt)     | 86.34ms | 86.34ms |  11.58228 |    1.34MB |
| yyjsonr::from_json_str(jlt) |  3.58ms |  3.79ms | 256.80834 |  547.25KB |

</details>

<img src="man/figures/README-unnamed-chunk-27-1.png" width="100%" />

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
