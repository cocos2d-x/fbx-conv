#include "CKBFile.h"
#include "Animation.h"
#include "Model.h"

namespace fbxconv{
	using namespace modeldata;

	CKBFile::CKBFile(void)
		:_file(NULL)
	{

	}

	CKBFile::~CKBFile(void)
	{

	}

	bool CKBFile::saveBinary(const std::string& filepath)
	{
		_file = fopen(filepath.c_str(), "w+b");

		// identifier
		char identifier[] = {'C','3','B','\0'};

		fwrite(identifier, 1, sizeof(identifier), _file);

		//version
		fwrite(GPB_VERSION, 1, sizeof(GPB_VERSION), _file);

		// static or skeletal
		/*if(_models->animations[0]->length == 0)
		{
			write((bool)true, _file);
		}
		else
		{
			write((bool)false, _file);
		}*/
		
		_refTable.writeBinary(_file);

		if(_models)
		{
			_models->writeBinary(_file);
		}
		
		_refTable.updateOffset(_file);

		fclose(_file);
		return true;
	}

	void CKBFile::addToRefTable(ObjRef* obj)
	{
		if(obj)
		{
			std::string& id = obj->id;
			if(id.length() > 0)
			{
				if(_refTable.get(id) == NULL)
				{
					_refTable.add(id, obj);
				}
			}
		}
	}

	void CKBFile::AddModel(modeldata::Model* model)
	{
		_models = model;

		//Add Mesh
		Mesh* mesh = model->meshes[0];
		addToRefTable(mesh->GetObj());

		// Add MeshPart
		MeshPart* meshpart = mesh->parts[0];
		addToRefTable(meshpart->GetObj());

		// Add Material
		Material* mat = model->materials[0];
		addToRefTable(mat->GetObj());

		// Add Skin
		Node* node = model->nodes[0];
		addToRefTable(node->GetObj());
		
		// Add bone part


		// Add Animation.
		//Animation *anims = 
		auto itr = model->animations.begin(); 
		for(; itr != model->animations.end(); itr++) 
		{
			Animation* anim = *itr;
			addToRefTable(anim->GetObj());

			/*std::vector<NodeAnimation*>::iterator itr = anim->nodeAnimations.begin();
			for(; itr != anim->nodeAnimations.end(); itr++)
			{
				NodeAnimation* nodeanim = *itr;
				addToRefTable(nodeanim->GetObj());
	
			}*/

		}

	}

}