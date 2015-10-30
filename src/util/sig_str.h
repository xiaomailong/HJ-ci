#ifndef PSNPRINTF_H
#define PSNPRINTF_H

#include <stdarg.h>
#include <stdlib.h>

#include "util/ci_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/*Like unix strnlen function*/
CIAPI_FUNC(size_t) CISigStr_Strnlen(const char *s, size_t count);

/*Like unix snprintf function,return should be formated string length as snprintf*/
CIAPI_FUNC(int32_t) CISigStr_Snprintf(char *str, size_t n, const char *format, ...);

/*Like unix vsnprintf function,return should be formated string length as vsnprintf*/
CIAPI_FUNC(int) CISigStr_Vsnprintf(char *str, size_t n, const char *format, va_list ap);

/* Use these directly if you want to avoid overhead of psnprintf
 * Return value is number of characters printed (or number printed
 * if there had been enough room).
 */
int pvsnfmt_char(char **pinsertion, size_t *nmax, const char fmt, int flags, 
                 int32_t width, int precision, char prefix, va_list *ap);

int pvsnfmt_int(char **pinsertion, size_t *nmax, char fmt, int flags, 
                 int32_t width, int precision, char prefix, va_list *ap);

int pvsnfmt_str(char **pinsertion, size_t *nmax, const char fmt, int flags, 
                 int32_t width, int precision, char prefix, va_list *ap);

int pvsnfmt_double(char **pinsertion, size_t *nmax, const char fmt, int flags, 
                 int32_t width, int precision, char prefix, va_list *ap);
   
/* These are the flags you need (use logical OR) for the flags parameter of
 * fmt functions above.
 */
#define FLAG_DEFAULT         0x00
#define FLAG_LEFT_ALIGN      0x01 /* - */
#define FLAG_SIGNED          0x02 /* + */
#define FLAG_ZERO_PAD        0x04 /* 0 */
#define FLAG_SIGN_PAD        0x08 /* ' ' */
#define FLAG_HASH            0x10 /* #  */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifdef PSNPRINTF_H */


