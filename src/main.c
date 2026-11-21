// Project: prekladac-ifj26
// File: main.c
// Author: Martin Vaško (vaskom00)

// Entry point of compiler

#include "error_codes.h"
#include "parser.h"

int main(void) {

  // Calls function from parser
  ErrCode err = compile();
  return err;
}