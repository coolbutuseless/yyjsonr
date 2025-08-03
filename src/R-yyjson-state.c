
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "zlib.h"
#include "yyjson.h"
#include "R-yyjson-state.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create state
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
state_t *create_state(void) {
  state_t *state = calloc(1, sizeof(state_t));
  
  return state;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Free all things in the state
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void destroy_state(state_t *state) {
  if (state == NULL) return;
  
  if (state->doc) {
    yyjson_doc_free(state->doc);
  }
  
  for (int i = 0; i < state->ncols; i++) {
    free(state->colnames[i]);
  }
  
  free(state);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUFSIZE 4096
void error_and_destroy_state(state_t *state, const char *fmt, ...) {
  
  char *buf = R_alloc(1, BUFSIZE);
  
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, BUFSIZE, fmt, args);
  va_end(args);
  
  destroy_state(state);
  Rf_error("%s", (const char *)buf);
}


