#ifndef _REFERENCE_H_
#define _REFERENCE_H_

#include <string>

namespace fbxconv {

// TypeID's
enum TypeID
{
    SCENE_ID = 1,
    NODE_ID = 2,
    ANIMATIONS_ID = 3,
    ANIMATION_ID = 4,
    ANIMATIONCHANNEL_ID = 5,
    NODEINSTANCE_ID = 8,
    MODEL_ID = 11,
    MATERIAL_ID = 16,
    EFFECT_ID = 17,
    CAMERA_ID = 32,
    LIGHT_ID = 33,
    MESH_ID = 34,
    MESHPART_ID = 35,
    MESHSKIN_ID = 36,
    BONEPART_ID = 37,
    FONT_ID = 128,
};

struct ObjRef
{
	unsigned int tpyeid;
	long fPosition;
	std::string id;
};


class Reference
{
public:
	// Construct
	Reference();
	Reference(std::string xref, ObjRef* ref);

	// Destruct
	~Reference() {;}
	void writeBinary(FILE* file);
	void saveFilePosition(FILE* file);
	unsigned int getFilePosition();

	// Updates the offset of the reference object if it has already need written to files.
	bool updateOffset(FILE* file);
	bool updateOffset(FILE* file, long newOffset);

	ObjRef* getRef();
	ObjRef* _ref;
private:
	std::string _xref;
	unsigned int _type;
	unsigned int _offset;
	long _fPosition;
	
};


}

#endif