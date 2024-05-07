test_that("json_verbatim basic behavior", {
    chr1 <- '[1,2,3]'
    chr3 <- rep(chr1, 3)
    json1 <- chr1; class(json1) <- "json"
    json3 <- chr3; class(json3) <- "json"

    expect_identical(
        write_json_str(chr1, auto_unbox = FALSE, json_verbatim = FALSE),
        '["[1,2,3]"]'
    )
    expect_identical(
        write_json_str(chr1, auto_unbox = TRUE, json_verbatim = FALSE),
        '"[1,2,3]"'
    )
    expect_identical(
        write_json_str(chr1, auto_unbox = FALSE, json_verbatim = TRUE),
        '["[1,2,3]"]'
    )
    expect_identical(
        write_json_str(chr1, auto_unbox = TRUE, json_verbatim = TRUE),
        '"[1,2,3]"'
    )

    expect_identical(
        write_json_str(json1, auto_unbox = FALSE, json_verbatim = FALSE),
        '["[1,2,3]"]'
    )
    expect_identical(
        write_json_str(json1, auto_unbox = TRUE, json_verbatim = FALSE),
        '"[1,2,3]"'
    )
    expect_identical(
        write_json_str(json1, auto_unbox = FALSE, json_verbatim = TRUE),
        '[[1,2,3]]'
    )
    expect_identical(
        write_json_str(json1, auto_unbox = TRUE, json_verbatim = TRUE),
        '[1,2,3]'
    )

    expect_identical(
        write_json_str(chr3, auto_unbox = FALSE, json_verbatim = FALSE),
        '["[1,2,3]","[1,2,3]","[1,2,3]"]'
    )
    expect_identical(
        write_json_str(chr3, auto_unbox = TRUE, json_verbatim = FALSE),
        '["[1,2,3]","[1,2,3]","[1,2,3]"]'
    )
    expect_identical(
        write_json_str(chr3, auto_unbox = FALSE, json_verbatim = TRUE),
        '["[1,2,3]","[1,2,3]","[1,2,3]"]'
    )
    expect_identical(
        write_json_str(chr3, auto_unbox = TRUE, json_verbatim = TRUE),
        '["[1,2,3]","[1,2,3]","[1,2,3]"]'
    )

    expect_identical(
        write_json_str(json3, auto_unbox = FALSE, json_verbatim = FALSE),
        '["[1,2,3]","[1,2,3]","[1,2,3]"]'
    )
    expect_identical(
        write_json_str(json3, auto_unbox = TRUE, json_verbatim = FALSE),
        '["[1,2,3]","[1,2,3]","[1,2,3]"]'
    )
    expect_identical(
        write_json_str(json3, auto_unbox = FALSE, json_verbatim = TRUE),
        '[[1,2,3],[1,2,3],[1,2,3]]'
    )
    expect_identical(
        write_json_str(json3, auto_unbox = TRUE, json_verbatim = TRUE),
        '[[1,2,3],[1,2,3],[1,2,3]]'
    )    
})

test_that("json_verbatim failures", {
    x <- list(
        foo = "bar"
    )
    class(x$foo) <- "json"  ## this is a bad class assignment, since "bar" isn't quote-wrapped.

    valid_json <- write_json_str(x, auto_unbox = TRUE, json_verbatim = FALSE)
    invalid_json <- write_json_str(x, auto_unbox = TRUE, json_verbatim = TRUE)

    expect_true(yyjsonr::validate_json_str(valid_json))
    expect_false(yyjsonr::validate_json_str(invalid_json))
})