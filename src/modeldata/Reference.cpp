#include "Reference.h"
#include "FileIO.h"
//#include <stdio.h>

namespace fbxconv {

Reference::Reference():
_ref(nullptr),
_xref(""),
_type(0),
_offset(0),
_fPosition(0.f)
{

}

Reference::Reference(std::string xref, ObjRef* ref):
_ref(ref),
_xref(xref),
_type(ref->tpyeid),
_offset(0),
_fPosition(0.f)
{

}

void Reference::writeBinary(FILE* file)
{
	saveFilePosition(file);
	write(_xref, file);
	write(_type,file);
	write(_offset, file);
}

bool Reference::updateOffset(FILE* file)
{
	long newOffset = _ref->fPosition;
	return updateOffset(file,newOffset);
}

bool Reference::updateOffset(FILE* file, long newOffset)
{
	if(getFilePosition() > 0)
	{
		// save the current offset
		long saveOffset = ftell(file);

		// update the offset data for us
		_offset = newOffset;
		// seek this Reference object in the file.
		fseek(file, getFilePosition(), SEEK_SET);

		//Skip over the id string
		skipString(file);

		// Skip TypeID unsigned int
		skipUint(file);

		// write over the old offset.
		write(_offset, file);

		// restore the offset.
		fseek(file, saveOffset, SEEK_SET);
		return true;
	}

	return false;
}

ObjRef* Reference::getRef()
{
	return _ref;
}

unsigned int Reference::getFilePosition()
{
	return (unsigned int)_fPosition;
}

void Reference::saveFilePosition(FILE* file)
{
	_fPosition = ftell(file);
}

}