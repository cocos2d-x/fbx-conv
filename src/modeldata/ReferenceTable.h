#ifndef _REFTABLE_H_
#define _REFTABLE_H_

#include <string>
#include <map>
#include "Reference.h"

namespace fbxconv {

class ReferenceTable
{
public:
	// Construct
	ReferenceTable(){};

	// Destruct
	virtual ~ReferenceTable(){};

	
	void add(std::string &xref, ObjRef* obj);

	ObjRef* get(const std::string& xref);

	void writeBinary(FILE* file);

	void updateOffset(FILE* file);

	std::map<std::string, Reference>::iterator begin();
	std::map<std::string, Reference>::iterator end();

private:
	std::map<std::string, Reference> _tables;
};

}
#endif