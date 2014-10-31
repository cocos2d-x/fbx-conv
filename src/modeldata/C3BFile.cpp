#include "C3BFile.h"
#include "Animation.h"
#include "Model.h"
namespace fbxconv{
	using namespace modeldata;
    unsigned char GPB_VERSION[2] = {VERSION_HI, VERSION_LO};
	C3BFile::C3BFile(void)
		:_file(NULL)
	{

	}

	C3BFile::~C3BFile(void)
	{

	}

	bool C3BFile::saveBinary(const std::string& filepath)
	{
		_file = fopen(filepath.c_str(), "w+b");

		// identifier
		char identifier[] = {'C','3','B','\0'};

		fwrite(identifier, 1, sizeof(identifier), _file);

		//version
		fwrite(GPB_VERSION, 1, sizeof(GPB_VERSION), _file);

		_refTable.writeBinary(_file);

		if(_models)
		{
			_models->writeBinary(_file);
		}
		
		_refTable.updateOffset(_file);

		fclose(_file);
		return true;
	}

	void C3BFile::addToRefTable(ObjRef* obj)
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

	void C3BFile::AddModel(modeldata::Model* model)
	{
		_models = model;

		//Add Mesh
        if(model->meshes.size()>0)
        {
            Mesh* mesh = model->meshes[0];
		    addToRefTable(mesh->GetObj());
        }
		//MeshPart* meshpart = mesh->parts[0];
		//addToRefTable(meshpart->GetObj());

		// Add Material
        if(model->materials.size()>0)
        {
            Material* mat = model->materials[0];
            addToRefTable(mat->GetObj());
        }
		// Add node
        if(model->nodes.size()>0)
        {
            Node* node = model->nodes[0];
            addToRefTable(node->GetObj());
        }
        
        // Add animations
	    if(model->animations.size()>0)
        {
            for (int i = 0; i < model->animations.size(); i++)
            {
                Animation* anim = model->animations[i];
                addToRefTable(anim->GetObj());
            }
        }

	}

}
