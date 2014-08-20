#include "FileIO.h"
#include <cassert>
namespace fbxconv
{

	
// Writing out a binary file //
//void write(size_t value, FILE* file)
//{
//	size_t r = fwrite(&value, sizeof(size_t), 1, file);
//}


void write(unsigned char value, FILE* file)
{
    size_t r = fwrite(&value, sizeof(unsigned char), 1, file);
    assert(r == 1);
}

void write(char value, FILE* file)
{
    size_t r = fwrite(&value, sizeof(char), 1, file);
    assert(r == 1);
}

void write(const char* str, FILE* file)
{
    size_t length = strlen(str);
    size_t r = fwrite(str, 1, length, file);
    assert(r == length);
}

void write(unsigned int value, FILE* file)
{
    size_t r = fwrite(&value, sizeof(unsigned int), 1, file);
    assert(r == 1);
}

void write(unsigned short value, FILE* file)
{
    size_t r = fwrite(&value, sizeof(unsigned short), 1, file);
    assert(r == 1);
}

void write(bool value, FILE* file)
{
    // write booleans as a unsigned char
    unsigned char b = value;
    write(b, file);
}
void write(float value, FILE* file)
{
    fwrite(&value, sizeof(float), 1, file);
}
void write(const float* values, int length, FILE* file)
{
    fwrite(values, sizeof(float), length, file);
    /*for (int i = 0; i < length; ++i)
    {
        write(values[i], file);
    }*/
}
void write(const std::string& str, FILE* file)
{
    // Write the length of the string
    write((unsigned int)str.size(), file);
    
    if (str.size() > 0)
    {
        // Write the array of characters of the string
        write(str.c_str(), file);
    }
}

void writeZero(FILE* file)
{
    write((unsigned int)0, file);
}

void skipString(FILE* file)
{
    // get the size of the char array
    unsigned int length = 0;
    fread(&length, sizeof(unsigned int), 1, file);
    // skip over the unsigned int length
    //fseek(file, sizeof(unsigned int), SEEK_CUR);
    if (length > 0)
    {
        // Skip over the array of chars
        long seek = (long)(length * sizeof(char));
        fseek(file, seek, SEEK_CUR);
    }
}

void skipUint(FILE* file)
{
    fseek(file, sizeof(unsigned int), SEEK_CUR);
}

}
