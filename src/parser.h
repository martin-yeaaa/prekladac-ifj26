// Project: prekladac-ifj26
// File: parser.h
// Author: Martin Vaško (vaskom00)

// Header for file parser function

#ifndef PARSER_H
#define PARSER_H

#include "error_codes.h"

/**
 * @brief Compiles program. Input program is in language IFJ26. Output program
 * is in language IFJcode26. Input is read from stdin. Output is written into
 * stdout.
 * @return `SUCCESS` if program successfuly compiles, otherwise one of the
 * error codes, depending on what kind of error the program has.
 */
ErrCode compile(void);

#endif