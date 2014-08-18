#ifndef FILEIO_H_
#define FILEIO_H_

#include <cstdio>
#include <list>
#include <vector>
#include <stdio.h>
#include <string>

namespace fbxconv
{

/**
 * Writes binary data to the given file stream.
 * 
 * @param value The value to be written
 * @param file The binary file stream.
 */
//void write(size_t value, FILE* file);
void write(unsigned char value, FILE* file);
void write(char value, FILE* file);
void write(const char* str, FILE* file);
void write(unsigned int value, FILE* file);
void write(unsigned short value, FILE* file);
void write(bool value, FILE* file);
void write(float value, FILE* file);
void write(const float* values, int length, FILE* file);

/**
 * Writes the length of the string and the string bytes to the binary file stream.
 */
void write(const std::string& str, FILE* file);

void writeZero(FILE* file);

/**
 * Skips over the string at the current file stream offset by moving the file position.
 * Assumes the current position points to the unsigned int length of the string.
 * The string is assumed to be a char array.
 * 
 * @param file The file stream.
 */
void skipString(FILE* file);

void skipUint(FILE* file);


}

#endif
