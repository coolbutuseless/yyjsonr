
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <zlib.h>

#include "yyjson.h"
#include "R-yyjson-serialize.h"

#include "utils.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// inner yyjson version
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP yyjson_version_(void) {
  return Rf_mkString(YYJSON_VERSION_STRING);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Double the length of a list by
//   - allocating space for a list which is twice the length
//   - copy across all elements one-by-one
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP grow_list(SEXP oldlist) {
  R_xlen_t len = XLENGTH(oldlist);
  SEXP newlist = PROTECT(Rf_allocVector(VECSXP, 2 * len));
  for (R_xlen_t i=0; i < len; i++) {
    SET_VECTOR_ELT(newlist, i, VECTOR_ELT(oldlist, i));
  }
  UNPROTECT(1);
  return newlist;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Count the number of newlines in a file
// 'gz' lib handles compressed and uncompressed files.
//
// Two options for parsing streaming input from a file:
//   (1) calculate num of lines. Allocate this exactly. Parse file.
//       - PRO: Minimise re-allocation as data grows
//       - CON: have to traverse the file twice
//   (2) Start parsing and just double memory allocation whenever we run out
//     of room in the data.frame or list.
//       - PRO: Only traverse the file once
//       - CON: Have to spend effort re-allocating R object as it grows
//
//  For LIST objects, it's really easy to  groww their size. See 'grow_list_()'
//  and then just truncate it to the actual data size at the end.
//
// For data.frames, growing its size involves growing the size of every 
// column individually. For int/double, this is an easy re-allocation.
// For string STRSXP you'd have to grow them in the same manner as 'grow_list_()'
// grows VECSXP objects.  This seems like a lot of work in order to figure
// out if its any faster than just traversing the file twice in order to 
// count number of newlines.
//
// For now (2023-08-09), ndjson->list will use method 2 and 
// ndjson->data.frame will use method 1
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int count_lines(const char *filename) {
  char buf[MAX_LINE_LENGTH] = {0};
  int counter = 0;
  
  gzFile file = gzopen(filename, "r");
  
  for(;;) {
    size_t res = gzread(file, buf, MAX_LINE_LENGTH);
    
    int i;
    for(i = 0; i < res; i++) {
      if (buf[i] == '\n')
        counter++;
    }
    
    if (gzeof(file))
      break;
  }
  
  gzclose(file);
  return counter;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Truncate all the vectors in a list to the same length.
// This function operates in-situ
//
// @param df_ data.frame
// @param data_length the actual length of the data in the vector.
//        data_length <= allocated_length
// @param allocated_length the full length allocated for this vector
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void truncate_list_of_vectors(SEXP df_, int data_length, int allocated_length) {
  if (data_length != allocated_length) {
    for (int i=0; i < Rf_length(df_); i++) {
      SEXP trunc_ = PROTECT(Rf_lengthgets(VECTOR_ELT(df_, i), data_length));
      SET_VECTOR_ELT(df_, i, trunc_);
      UNPROTECT(1);
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Given a list of vectors (of all the same length), convert it to a 
// data.frame
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP promote_list_to_data_frame(SEXP df_, char **colname, int ncols) {
  
  int nprotect = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set colnames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, ncols)); nprotect++;
  for (unsigned int i = 0; i < ncols; i++) {
    SET_STRING_ELT(nms_, i, Rf_mkChar(colname[i]));
  }
  Rf_setAttrib(df_, R_NamesSymbol, nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set empty rownames on data.frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ncols == 0) {
    SEXP rownames = PROTECT(Rf_allocVector(INTSXP, 0)); nprotect++;
    Rf_setAttrib(df_, R_RowNamesSymbol, rownames);
  } else {  
    int nrows = Rf_length(VECTOR_ELT(df_, 0));
    SEXP rownames = PROTECT(Rf_allocVector(INTSXP, 2)); nprotect++;
    SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
    SET_INTEGER_ELT(rownames, 1, -nrows);
    Rf_setAttrib(df_, R_RowNamesSymbol, rownames);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 'data.frame' class
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SET_CLASS(df_, Rf_mkString("data.frame"));
  
  UNPROTECT(nprotect);
  return df_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Return the index of the named column which matches 'nm'
// return -1 when not found
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int df_col_idx(SEXP df_, const char *nm) {
  
  SEXP nms_ = Rf_getAttrib(df_, R_NamesSymbol);
  if (nms_ == R_NilValue) return -1;
  
  for (int i = 0; i < Rf_length(nms_); i++) {
    SEXP nm_ = STRING_ELT(nms_, i);
    if (strcmp(CHAR(nm_), nm) == 0) return i;
  }
  
  return -1;
}


