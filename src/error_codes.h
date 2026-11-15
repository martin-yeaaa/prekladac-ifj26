// Project: prekladac-ifj26
// File: error_codes.h
// Author: Martin Vaško (vaskom00)

// File containing macros for all possible errors.

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

typedef int ErrCode;

// Code for successful execution
#define SUCCESS 0

// Error occured during lexical analysis
#define ERR_LEXICAL 1

// Error occured during syntactic analysis
#define ERR_SYNTAX 2

// Use of undefined function, variable, or type.
// Missing @main function.
#define ERR_UNDEF_SYM 3

// Invalid redefinition of function, variable, or type.
#define ERR_REDEFINITION 4

// Wrong amount or type of arguments when calling function.
// Invalid parameters of @main function.
#define ERR_FUN_ARGS 5

// Incompatible types in expression, assignment, initialization, return value
// or control flow structures.
#define ERR_INCOMPATIBLE_TYPE 6

// Assignment to unmodifiable variable or formal parameter of function.
#define ERR_INVALID_ASSIGN 7

// Other semantic errors, for example unallowed return statement.
#define ERR_OTHER_SEMANTIC 10

// Internal error of compiler, for example malloc failure.
#define ERR_INTERNAL 99

#endif