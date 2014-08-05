#include "Node.h"
#include "NodePart.h"
#include "Animation.h"
#include "NodeAnimation.h"
#include "Keyframe.h"
#include "Material.h"
#include "Attributes.h"
#include "MeshPart.h"
#include "Mesh.h"
#include "Model.h"
#include "Reference.h"
#include "FileIO.h"


namespace fbxconv {
namespace modeldata {

	void Model::writeBinary(FILE* file)
	{
		// write mesh
		for(auto itr = meshes.begin(); itr != meshes.end(); itr++)
		{
			(*itr)->writeBinary(file);
		}
		
		// write material
		materials[0]->object.fPosition = ftell(file);
		unsigned int size = materials.size();
		write(size, file);
		for(auto itr = materials.begin(); itr != materials.end(); itr++)
		{
			(*itr)->writeBinary(file);
		}

		// skin
		for(auto itr = nodes.begin(); itr != nodes.end(); itr++)
		{
	//	auto itr = nodes.begin();
		(*itr)->writeBinary(file);
		}

		// animation
		for(auto itr = animations.begin(); itr != animations.end(); itr++)
		{
			(*itr)->writeBinary(file);
		}
	}

	void Mesh::writeBinary(FILE* file)
	{
		object.fPosition = ftell(file);
									
		// attribute
		attributes.writeBinary(file);
		
		// write vertices
		if(vertices.size() > 0)
		{
			unsigned int size = vertices.size();
			write(size, file);
			write(&vertices[0],size,file);
		}
		else
		{
			write((unsigned int)0, file);
		}

		// write parts.
		unsigned int size = parts.size();
		write(size, file);
		for(auto itr = parts.begin(); itr != parts.end(); itr++)
		{
			// indices size
			unsigned int size = (*itr)->indices.size();
			write(size, file);
			
			// indices.
			for(auto itr1 = (*itr)->indices.begin(); itr1 != (*itr)->indices.end(); itr1++)
				write(*itr1,file);

		}

	}
	
	void Attributes::writeBinary(FILE* file)
	{
		
		 enum
		{
        VERTEX_ATTRIB_POSITION,
        VERTEX_ATTRIB_COLOR,
        VERTEX_ATTRIB_TEX_COORD,
        VERTEX_ATTRIB_NORMAL,
        VERTEX_ATTRIB_BLEND_WEIGHT,
        VERTEX_ATTRIB_BLEND_INDEX,

        VERTEX_ATTRIB_MAX,

        // backward compatibility
        VERTEX_ATTRIB_TEX_COORDS = VERTEX_ATTRIB_TEX_COORD,
		};


		unsigned int size = 0;
		std::vector<MeshVertexAttrib> attribs;
		MeshVertexAttrib attrib;
		// Has Position
		if(hasPosition())
		{
			size++;
			attrib.usage = VERTEX_ATTRIB_POSITION;
			attrib.attribSizeBytes = 3;
			attribs.push_back(attrib);
		}

		// Has Color
		if(hasColor())
		{
			size++;
			attrib.usage = VERTEX_ATTRIB_COLOR;
			attrib.attribSizeBytes = 4;
			attribs.push_back(attrib);
		}

		// Has Normal
		if(hasNormal())
		{
			size++;
			attrib.usage = VERTEX_ATTRIB_NORMAL;
			attrib.attribSizeBytes = 3;
			attribs.push_back(attrib);
		}

		if(hasUV(0))
		{
			size++;
			attrib.usage = VERTEX_ATTRIB_TEX_COORD;
			attrib.attribSizeBytes = 2;
			attribs.push_back(attrib);
		}

		// Has Tangent
		if(hasTangent())
		{
			attrib.usage = 4;
			size++;
			attribs.push_back(attrib);
		}
		// Has Binormal
		if(hasBinormal())
		{
			attrib.usage = 5;
			size++;
			attribs.push_back(attrib);
		}
		// Has Blend weight
		for(int i = 0; i < 8; i++)
		{
			if(!hasBlendWeight(i))
			{
				if(i>0)
				{
					size++;
					attrib.usage = VERTEX_ATTRIB_BLEND_WEIGHT;
					attrib.attribSizeBytes = 4;
					attribs.push_back(attrib);

					attrib.usage = VERTEX_ATTRIB_BLEND_INDEX;
					attrib.attribSizeBytes = 4;
					attribs.push_back(attrib);
				}
				break;
			}
		}

        size = attribs.size();
		write(size, file);
		for(int i = 0; i <attribs.size(); i++)
		{
			write(attribs[i].usage, file);
			write(attribs[i].attribSizeBytes, file);	
		}

	}
	
	void Material::writeBinary(FILE* file)
	{
		//object.fPosition = ftell(file);
		//const std::string matname = textures[0]->path;
		write(textures[0]->path, file);
	}
	
	void Node::writeBinary(FILE* file)
	{
		object.fPosition = ftell(file);
		// node part
		write(id, file);
		write(transforms, 16, file);
		//write(parts.size(),file);

		//for(auto itr = parts.begin(); itr != parts.end(); itr++)
		if(parts.size()>0)
		{
			NodePart* nodepart = parts[0];
            unsigned int size = nodepart->bones.size();
			write(size, file);
			for(auto itr = nodepart->bones.begin(); itr != nodepart->bones.end(); itr++)
			{
				// write name
				write(itr->first->id, file);
				// write transform
				float tmp[16];
				for(int i = 0; i < 4; i++)
				{
					for(int j = 0; j < 4; j++)
					{
						tmp[i*4 + j] = itr->second.Double44()[i][j];
					}
				}
				write(tmp, 16, file);
			}
		}	


		// links
		if(links.size() > 0)
			write(static_cast<unsigned int>(links.size()), file);
		else
			return;
		for(auto itr = links.begin(); itr != links.end(); itr++)
		{
			Node* node = *itr;
			// idq
			write(node->id, file);

			// parent id
			if(node->parent)
				write(node->parent->id, file);
			else
				write(std::string(""), file);

			// transform
			write(node->transforms, 16,file);
		}
	}

	void Animation::writeBinary(FILE* file)
	{
		object.fPosition = ftell(file);
		// for NodeAnimation each keyframe
		write(id, file);
		write(length, file);
		write(static_cast<unsigned int>(nodeAnimations.size()), file);

		for(auto itr = nodeAnimations.begin(); itr != nodeAnimations.end(); itr++)
		{
			NodeAnimation* nodeanim = *itr;
			write(nodeanim->node->id, file);
			
			write(static_cast<unsigned int>(nodeanim->keyframes.size()), file);
			
			for(auto itr1 = nodeanim->keyframes.begin(); itr1 != nodeanim->keyframes.end(); itr1++)
			{
				Keyframe* keyframe = *itr1;
				// write time
				write(keyframe->time, file);

				// write
			//	if(keyframe->hasRotation)
				write(keyframe->rotation, 4, file);
			//	if(keyframe->hasScale)
				write(keyframe->scale, 3, file);
			//	if(keyframe->hasTranslation);
				write(keyframe->translation, 3, file);
			}
		}
	}


} }