#ifndef _GPBFILE_H_
#define _GPBFILE_H_

#include "FileIO.h"
#include "Model.h"
#include "ReferenceTable.h"

namespace fbxconv {

const unsigned char GPB_VERSION[2] = {0, 1};



class CKBFile
{
public:
	// Construct.
	CKBFile(void);

	~CKBFile(void);

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