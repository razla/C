/* caspl-lab-1.c
 * Limited versions of printf
 *
 * Programmer: Mayer Goldberg, 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* the states in the printf state-machine */
enum printf_state {
  st_printf_init,
  st_printf_meta_char,
  st_printf_percent,
  st_printf_octal2,
  st_printf_finish,
  st_printf_octal3
};

#define MAX_NUMBER_LENGTH 64
#define is_octal_char(ch) ('0' <= (ch) && (ch) <= '7')

int toy_printf(char *fs, ...);

const char *digit = "0123456789abcdef";
const char *DIGIT = "0123456789ABCDEF";

int print_int_helper(unsigned int n, int radix, const char *digit) {
  int result;

  if (n < radix) {
    putchar(digit[n]);
    return 1;
  }
  else {
    result = print_int_helper(n / radix, radix, digit);
    putchar(digit[n % radix]);
    return 1 + result;
  }
}

int print_int(unsigned int n, int radix, const char * digit) {
  if (radix < 2 || radix > 16) {
    toy_printf("Radix must be in [2..16]: Not %d\n", radix);
    exit(-1);
  }
  
  if (n > 0) {
    return print_int_helper(n, radix, digit);
  }
  if (n == 0) {
    putchar('0');
    return 1;
  }
  else {
    putchar('-');
    return 1 + print_int_helper(-n, radix, digit);
  }
}

int print_int_d(int n, int radix, const char * digit) {
  if (radix < 2 || radix > 16) {
    toy_printf("Radix must be in [2..16]: Not %d\n", radix);
    exit(-1);
  }
  
  if (n > 0) {
    return print_int_helper(n, radix, digit);
  }
  if (n == 0) {
    putchar('0');
    return 1;
  }
  else {
    putchar('-');
    return 1 + print_int_helper(-n, radix, digit);
  }
}

/* SUPPORTED:
 *   \f, \t, \n, \r -- formfeed, tab, newline, return
 *   \F, \T, \N, \R -- extensions: visible versions of the above
 *   \c -- extension: CASPL'2018
 *   \C -- extension: visible version of the above
 *   %b, %d, %o, %x, %X -- 
 *     integers in binary, decimal, octal, hex, and HEX
 *   %s -- strings
 */

int toy_printf(char *fs, ...) {
  int chars_printed = 0; // num of chars printed
  int int_value = 0; // int value of given input
  int isCharA = 0; // if we got A for an array
  int width = 0; // width of the given output
  int nDigits = 0; // num of digits
  int temp = 0; // temp variable for nDigits
  int sizeOfArray; // size of an array given as args
  char **arrStr; // array of strings
  char *arrChar;
  int *arrInt; // array of ints
  char *string_value; // string value of given input
  char char_value; // chars value of given input
  char octal_char; // octal char of given input
  va_list args;
  enum printf_state state;

  va_start(args, fs);

  state = st_printf_init; 

  for (; *fs != '\0'; ++fs) {
    switch (state) {
    case st_printf_finish:
        break;
    case st_printf_init:
      switch (*fs) {
      case '\\':
	state = st_printf_meta_char;
	break;
	
      case '%':
	state = st_printf_percent;
	break;

      default:
	putchar(*fs);
	++chars_printed;
      }
      break;

    case st_printf_meta_char:
      switch (*fs) {
      case '\\':
	putchar('\\');
	++chars_printed;
	state = st_printf_init;
	break;
	
      case '\"':
	putchar('\"');
	++chars_printed;
	state = st_printf_init;
	break;
	
      case 't':
	putchar('\t');
	++chars_printed;
	state = st_printf_init;
	break;

      case 'T':
	chars_printed += toy_printf("<tab>\t");
	state = st_printf_init;
	break;

      case 'f':
	putchar('\f');
	++chars_printed;
	state = st_printf_init;
	break;

      case 'F':
	chars_printed += toy_printf("<formfeed>\f");
	state = st_printf_init;
	break;

      case 'n':
	putchar('\n');
	++chars_printed;
	state = st_printf_init;
	break;

      case 'N':
	chars_printed += toy_printf("<newline>\n");
	state = st_printf_init;
	break;

      case 'r':
	putchar('\r');
	++chars_printed;
	state = st_printf_init;
	break;

      case 'R':
	chars_printed += toy_printf("<return>\r");
	state = st_printf_init;
	break;

      case 'c':
	chars_printed += toy_printf("CASPL'2018");
	state = st_printf_init;
	break;

      case 'C':
	chars_printed += toy_printf("<caspl magic>");
	chars_printed += toy_printf("\\c");
	state = st_printf_init;
	break;

      default:
	if (is_octal_char(*fs)) {
	  octal_char = *fs - '0';
	  state = st_printf_octal2;
	}
	else {
	  toy_printf("Unknown meta-character: \\%c", *fs);
	  exit(-1);
	}
      }
      break;

    case st_printf_octal2:
      if (is_octal_char(*fs)) {
	octal_char = (octal_char << 3) + (*fs - '0');
	state = st_printf_octal3;
	break;
      }
      else {
	toy_printf("Missing second octal char. Found: \\%c", *fs);
	exit(-1);
      }

    case st_printf_octal3:
      if (is_octal_char(*fs)) {
	octal_char = (octal_char << 3) + (*fs - '0');
	putchar(octal_char);
	++chars_printed;
	state = st_printf_init;
	break;
      }
      else {
	toy_printf("Missing third octal char. Found: \\%c", *fs);
	exit(-1);
      }

    case st_printf_percent:
      switch (*fs) {
      case '%':
	putchar('%');
	++chars_printed;
	state = st_printf_init;
	break;

      case 'd':
        if (isCharA){
            arrInt = va_arg(args, int*);
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += print_int_d(arrInt[i], 10, digit);
                if (i<sizeOfArray-1){
                    putchar(',');
                    putchar(' ');
                    chars_printed++;
                    chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }
        else {
            int_value = va_arg(args, int);
            chars_printed += print_int_d(int_value, 10, digit);
            state = st_printf_init;
            break;
        }

      case 'b':
        if (isCharA){
            arrInt = va_arg(args, int*);
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += print_int(arrInt[i], 2, digit);
                if (i<sizeOfArray-1){
                        putchar(',');
                        putchar(' ');
                        chars_printed++;
                        chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }  
        else{
            int_value = va_arg(args, int);
            chars_printed += print_int(int_value, 2, digit);
            state = st_printf_init;
            break;
        }

      case 'o':
        if (isCharA){
            arrInt = va_arg(args, int*);
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += print_int(arrInt[i], 8, digit);
                if (i<sizeOfArray-1){
                    putchar(',');
                    putchar(' ');
                    chars_printed++;
                    chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }
        else{
            int_value = va_arg(args, int);
            chars_printed += print_int(int_value, 8, digit);
            state = st_printf_init;
            break;
        }
	
      case 'x':
        if (isCharA){
            arrInt = va_arg(args, int*);
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += print_int(arrInt[i], 16, digit);
                if (i<sizeOfArray-1){
                    putchar(',');
                    putchar(' ');
                    chars_printed++;
                    chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }
        else {
            int_value = va_arg(args, int);
            chars_printed += print_int(int_value, 16, digit);
            state = st_printf_init;
            break;
        }

      case 'X':
        if (isCharA){
            arrInt = va_arg(args, int*);
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += print_int(arrInt[i], 16, DIGIT);
                if (i<sizeOfArray-1){
                    putchar(',');
                    putchar(' ');
                    chars_printed++;
                    chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }
        else {
            int_value = va_arg(args, int);
            chars_printed += print_int(int_value, 16, DIGIT);
            state = st_printf_init;
            break;
        }
      case 's':
        if (isCharA){
            arrStr = va_arg(args, char**);
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += toy_printf("%c", '"');
                for (int j=0; j<strlen(arrStr[i]); j++){
                    chars_printed += toy_printf("%c",arrStr[i][j]);
                }
                chars_printed += toy_printf("%c", '"');
                if (i<sizeOfArray-1){
                        putchar(',');
                        putchar(' ');
                        chars_printed++;
                        chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }
        else {
            string_value = va_arg(args, char *);
            chars_printed += toy_printf(string_value);
            state = st_printf_init;
            break;
        }

      case 'c':
        if (isCharA){
            arrChar = va_arg(args, char*);   
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += toy_printf("%s", "'");
                chars_printed += putchar((char)arrChar[i]);
                chars_printed += toy_printf("%s", "'");
                if (i<sizeOfArray-1){
                        putchar(',');
                        putchar(' ');
                        chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }
        else {
            char_value = (char)va_arg(args, int);
            putchar(char_value);
            ++chars_printed;
            state = st_printf_init;
            break;
        }
            
      case 'u':
        if (isCharA){
            arrInt = va_arg(args, int*);   
            sizeOfArray = va_arg(args, int);
            putchar('{');
            chars_printed++;
            for (int i=0; i<sizeOfArray; i++){
                chars_printed += print_int(arrInt[i], 10, digit);
                if (i<sizeOfArray-1){
                    putchar(',');
                    putchar(' ');
                    chars_printed++;
                    chars_printed++;
                }
            }
            putchar('}');
            chars_printed++;
            state = st_printf_init;
            isCharA=0;
            break;
        }
        else {
            int_value = va_arg(args, int);
            chars_printed += print_int(int_value, 10, digit);
            state = st_printf_init;
            break;
        }
            
      case 'A': // array case
          isCharA=1;
          break;
    
      case '1' ... '9': // width str right padding
        width = atoi(fs);
        string_value = va_arg(args, char *);
        for (int i=0; i<strlen(string_value) && width > 0; i++){
            chars_printed += toy_printf("%c", string_value[i]);
            width--;
        }
        if (width > 0){
            while (width > 0){
                chars_printed += toy_printf("%c", ' ');
                width--;
            }
            chars_printed += toy_printf("%c", '#');
        }
        chars_printed += toy_printf("%c", '\n');
        chars_printed += toy_printf("%c", '\n');
        state = st_printf_finish;
        break;
      case '0': // width int
        ++fs;
        width = atoi(fs);
        int_value = va_arg(args, int);
        if (int_value<0){
            chars_printed += toy_printf("%c", '-');
            int_value=int_value*(-1);
            width--;
        }
        temp = int_value;
        while (temp!=0){
            nDigits++;
            temp=temp/10;
        }
        if (width>nDigits){
            while(width-nDigits>0){
                chars_printed += toy_printf("%c", '0');
                width--;
            }
        }
        chars_printed += print_int(int_value, 10, digit);
        chars_printed += toy_printf("%c", '\n');
        chars_printed += toy_printf("%c", '\n');
        state = st_printf_finish;
        break;
      case '-': // width str left padding
        ++fs;
        width = atoi(fs);
        string_value = va_arg(args, char *);
        while (width-strlen(string_value) > 0){
            chars_printed += toy_printf("%c", ' ');
            width--;
        }
        for (int i=0; i<strlen(string_value); i++){
            chars_printed += toy_printf("%c", string_value[i]);
        }
        chars_printed += toy_printf("%c", '\n');
        chars_printed += toy_printf("%c", '\n');
        state = st_printf_finish;
        break;
        
    break;
    default:
    toy_printf("Unhandled format %%%c...\n", *fs);
	exit(-1);
    }
      break;

    default:
      toy_printf("toy_printf: Unknown state -- %d\n", (int)state);
      exit(-1);
    }
  }

  va_end(args);

  return chars_printed;
}


