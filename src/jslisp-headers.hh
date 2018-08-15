//#define DEBUG
#include "js-config.h"
#include "jsapi.h"

#include "js/Class.h"
#include "js/Initialization.h"
#include "js/RootingAPI.h"
#include "js/Conversions.h" // as of SpiderMonkey 38; previously in jsapi.h

#ifdef HAVE_GMP
#include <gmp.h>
#else
#include "mini-gmp.h"
#endif

#define EXTERN_C extern "C" {
#define EXTERN_C_END };

#define ARG(x) (x ## _arg)
#define MODIFY_ARG(x) do { } while (0)
#define L(v) ELisp_Value (v)
#define LVH(v) ELisp_Handle (v)
#define LHH(v) (v)
//#define LRH(v) (v)
//#define LSH(v) (v)
#define LRH(v) ELisp_Handle (ELisp_Value (v))
#define LSH(v) ELisp_Handle (ELisp_Value (v))

extern _Noreturn void emacs_abort (void) NO_INLINE;
extern bool js_init();
