#ifndef _GPBFILE_H_
#define _GPBFILE_H_

#include "FileIO.h"
#include "Model.h"
#include "ReferenceTable.h"

namespace fbxconv {

class C3BFile
{
public:
	// Construct.
	C3BFile(void);

	~C3BFile(void);

	// Destruct.
	bool saveBinary(const std::string& filepath);

	void addToRefTable(ObjRef* obj);

	void AddModel(modeldata::Model* model);

private:
	FILE* _file;
	modeldata::Model* _models;
	ReferenceTable _refTable;

};
}

#endif
