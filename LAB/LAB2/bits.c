/*
 * CS:APP Data Lab
 *
 * <Please put your name and userid here>
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */

#endif
/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* We do not support C11 <threads.h>.  */
/*
 * negate - return -x
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
   // Definition of -x is ~x+1 when x is integer
   return ~x + 1;
}
/*
 * isLess - if x < y  then return 1, else return 0
 *   Example: isLess(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLess(int x, int y) {
   int sign_x = (x >> 31) & 1;
   int sign_y = (y >> 31) & 1;
   int sign_x_minus_y = ((x + (~y + 1)) >> 31) & 1;
   // Check the cases where x < y:
   // Case 1: x is negative (sign_x == 1), y is positive (sign_y == 0)
   // Case 2: Both x and y are negative or positive, but x - y is negative
   // (sign_x_minus_y == 1)
   // !(sign_x ^ sign_y) is equal to (sign_x == 1 && sign_y == 1) || (sign_x == 0
   // && sign_y == 0)
   int is_sign_same = !(sign_x ^ sign_y);
   return (sign_x & !sign_y) | (is_sign_same & sign_x_minus_y);
}
/*
 * float_abs - Return bit-level equivalent of absolute value of f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument..
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_abs(unsigned uf) {
   unsigned SIGN_MASK = 0x80000000;
   unsigned EXPONENT_MASK = 0x7F800000;
   unsigned FRACTION_MASK = 0x007FFFFF;
   unsigned exponent_part = uf & EXPONENT_MASK;
   unsigned fraction_part = uf & FRACTION_MASK;
   // Check if expoenent part is [1111 1111] and fraction part is NOT ZERO
   if (exponent_part == EXPONENT_MASK && fraction_part) {
      return uf;
   }
   // Reverse MSB
   return uf & ~SIGN_MASK;
}
/*
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
   unsigned SIGN_MASK = 0x80000000;
   unsigned EXPONENT_MASK = 0x7F800000;
   unsigned FRACTION_MASK = 0x007FFFFF;
   unsigned sign_part = uf & SIGN_MASK;
   unsigned exponent_part = uf & EXPONENT_MASK;
   unsigned fraction_part = uf & FRACTION_MASK;
   unsigned NaN_VALUE = 0x7FC00000;
   // Check for NaN or infinity
   if (exponent_part == EXPONENT_MASK) {
      return uf;
   }
   // Denormalized number or zero
   if (exponent_part == 0) {
      return (sign_part | exponent_part | (fraction_part << 1));
   }
   // Normalized number
   // Adding 1 to exponent part is equal to double the value
   // Because it follows IEEE Floating Point Standard
   exponent_part += 0x00800000;
   if (exponent_part == EXPONENT_MASK && fraction_part) {
      return NaN_VALUE;
   }
   return (sign_part | exponent_part | fraction_part);
}
/*
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
   unsigned SIGN_MASK = 0x80000000;
   unsigned sign_part = x & SIGN_MASK;
   unsigned ux, exponent_part, fraction_part, round_bits;
   // When x is 0
   if (x == 0) return 0;
   // When x is TMIN, return -TMIN which is 2^31
   if (x == 0x80000000) return 0xCF000000;
   // When x is negative, convert x to its absolute value for convenience
   if (sign_part) x = -x;
   // Derive 8bits exponent part
   ux = x;
   // At first, set exponent_part = bias + # of bits - 1 = 2^7 - 1 + 32 - 1
   exponent_part = 158;
   // Find exponent_part by shifting left until MSB becoming 1
   while (ux < 0x80000000) {
      exponent_part--;
      ux <<= 1;
   }
   // Derive fraction part, 23bits except MSB of 24bits
   fraction_part = (ux & 0x7FFFFF00) >> 8;  // Note that use ux which has been already shifted to 1.xxx form
   // Keep last 8bits(1byte) for rounding case
   round_bits = ux & 0xFF;
   // Round to even
   if (round_bits > 0x80 || (round_bits == 0x80 && fraction_part & 1)) {
      fraction_part++;
      // Overflow occured
      if (fraction_part == 0x800000) {
         fraction_part = 0;
         exponent_part++;
      }
   }
   // Return bit-level equivalent of expression (float) x
   return sign_part | (exponent_part << 23) | fraction_part;
}
/*
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
   unsigned SIGN_MASK = 0x80000000;
   unsigned EXPONENT_MASK = 0x7F800000;
   unsigned FRACTION_MASK = 0x007FFFFF;
   unsigned sign_part = uf & SIGN_MASK;
   unsigned exponent_part = uf & EXPONENT_MASK;
   unsigned fraction_part = uf & FRACTION_MASK;
   unsigned NaN_OR_INF = 0x80000000u;
   unsigned BIAS = 127;  // 2^7 - 1
   int result;
   // Check for NaN or infinity
   if (exponent_part == EXPONENT_MASK) {
      return NaN_OR_INF;
   }
   // Check zero
   if (exponent_part == 0) {
      return 0;
   }
   // Now, consider IEEE Floating Point Standard M*2^E
   exponent_part >>= 23;
   // When exponent part - BIAS < 0, int value is always zero
   if (exponent_part < BIAS) {
      return 0;
   }
   exponent_part -= BIAS;
   // When exponent part is more than 30, out of int range
   if (exponent_part > 30) {
      return NaN_OR_INF;
   }
   // Normalize the fraction by adding the hidden bit
   fraction_part = fraction_part | 0x00800000u;
   // Adjust fraction_part to integer
   // Compare with length of fraction part to determine which way to shift
   if (exponent_part < 23) {
      // When exponent is less than 23, make M*2^E by shifting fraction part right
      fraction_part >>= 23 - exponent_part;
   } else {
      // When exponent is more than 23, make M*2^E by shifting fraction part left
      fraction_part <<= exponent_part - 23;
   }
   // Finally IEEE Floating Point Standard (-1)^s * M * 2^E made by considering
   // sign part
   result = sign_part ? -fraction_part : fraction_part;
   return result;
}
