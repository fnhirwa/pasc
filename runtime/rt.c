/* Run-time support for programs compiled by pasc.
 *
 * Calling convention: System V AMD64. Every entry point that can detect an
 * error takes the source line as its last argument.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* 5.1 f): an error reported during execution terminates the program. */
static void p_error(int64_t line, const char* msg) {
  fflush(stdout);
  fprintf(stderr, "runtime error at line %" PRId64 ": %s\n", line, msg);
  exit(1);
}

void p_err_overflow(int64_t line) { p_error(line, "integer overflow"); }
void p_err_divzero(int64_t line) { p_error(line, "division by zero"); }
void p_err_mod(int64_t line) {
  p_error(line, "the right operand of mod is not positive");
}

static void pad(int64_t n) {
  for (; n > 0; --n) putchar(' ');
}

/* 6.9.3.1: TotalWidth shall be greater than or equal to one. */
static void check_width(int64_t width, int64_t line) {
  if (width < 1) p_error(line, "field width is less than one");
}

/* 6.9.3.2: right-justified; more positions are used if the width is short. */
void p_write_int(int64_t value, int64_t width, int64_t line) {
  char buf[32];
  check_width(width, line);
  int n = snprintf(buf, sizeof buf, "%" PRId64, value);
  pad(width - n);
  fputs(buf, stdout);
}

/* 6.9.3.6: right-justified, or truncated to the first width characters. */
void p_write_str(const char* s, int64_t len, int64_t width, int64_t line) {
  check_width(width, line);
  pad(width - len);
  fwrite(s, 1, (size_t)(width < len ? width : len), stdout);
}

/* 6.9.3.3 */
void p_write_char(int64_t c, int64_t width, int64_t line) {
  check_width(width, line);
  pad(width - 1);
  putchar((int)c);
}

/* 6.9.3.5: written as the string 'true' or 'false'. */
void p_write_bool(int64_t value, int64_t width, int64_t line) {
  if (value) p_write_str("true", 4, width, line);
  else p_write_str("false", 5, width, line);
}

void p_writeln(void) { putchar('\n'); }
