/*
  Implementation based on C99 Section 7.15 Variable arguments
*/

#ifndef _STDARG_H_
#define _STDARG_H_

typedef __builtin_va_list va_list;

#define va_arg(ap, type) __builtin_va_arg((ap), type)
#define va_copy(dest, src) __builtin_va_copy((ap), type)
#define va_end(ap) __builtin_va_end((ap))
#define va_start(ap, parmN) __builtin_stdarg_start((ap), (parmN))

#endif /* _STDARG_H_ */
