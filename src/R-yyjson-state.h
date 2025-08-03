

//===========================================================================
// max columns currently allocated for parsing data.frames
// TODO: Instead of static value, switch to dynamic allocation and grow
//       as-needed.  Did not want to think about this at the moment, so 
//       just set a high-ish value.  Code throws an error if this is 
//       exceeded during a parse.
//===========================================================================
#define MAX_DF_COLS 2048

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// State
// 
// There are various bits of state which need to be kept around, but
// properly freed when an error occurs.
//   - parse options - this doesn't need to be freed, but does need to be 
//                      passed to all functions, so just make it part of the state
//   - 
//  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  yyjson_doc *doc; // Pass around a referece to the doc so it can be freed on error
  
  char *colnames[MAX_DF_COLS];
  int ncols;
  
} state_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
state_t *create_state(void);
void destroy_state(state_t *state);
[[noreturn]] void error_and_destroy_state(state_t *state, const char *fmt, ...);
