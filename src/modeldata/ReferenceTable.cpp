#include "ReferenceTable.h"
#include "FileIO.h"

namespace fbxconv {


void ReferenceTable::add(std::string &xref, ObjRef* obj)
{
	_tables[xref] = Reference(xref, obj);
	_tables[xref]._ref = obj;

}

ObjRef* ReferenceTable::get(const std::string& xref)
{
	auto itr = _tables.find(xref);
	if(itr != _tables.end())
	{
		Reference ref = itr->second;
		return ref.getRef();
	}

	return NULL;
}

void ReferenceTable::writeBinary(FILE* file)
{
    unsigned int size = _tables.size();
	write(size,file);
	for(auto itr = _tables.begin(); itr != _tables.end(); itr++)
	{
		itr->second.writeBinary(file);
	}
}

void ReferenceTable::updateOffset(FILE* file)
{
	for(auto itr = _tables.begin(); itr != _tables.end(); ++itr)
	{
		Reference& ref = itr->second;
		ref.updateOffset(file);
	}
}

std::map<std::string, Reference>::iterator ReferenceTable::begin()
{
	return _tables.begin();
}

std::map<std::string, Reference>::iterator ReferenceTable::end()
{
	return _tables.end();	
}

}