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
 #include "Reference.h"
namespace fbxconv {
namespace modeldata {
    static const char* getTextureUseString(const Material::Texture::Usage &textureUse) {
	switch(textureUse){
	case Material::Texture::Ambient:
		return "AMBIENT";
	case Material::Texture::Bump:
		return "BUMP";
	case Material::Texture::Diffuse:
		return "DIFFUSE";
	case Material::Texture::Emissive:
		return "EMISSIVE";
	case Material::Texture::None:
		return "NONE";
	case Material::Texture::Normal:
		return "NORMAL";
	case Material::Texture::Reflection:
		return "REFLECTION";
	case Material::Texture::Shininess:
		return "SHININESS";
	case Material::Texture::Specular:
		return "SPECULAR";
	case Material::Texture::Transparency:
		return "TRANSPARENCY";
	default:
		return "UNKNOWN";
	}
}
    static const char* getWrapModeUseString(const FbxFileTexture::EWrapMode &textureUse)
    {
        switch(textureUse){
        case FbxFileTexture::eRepeat:
            return "REPEAT";
        case FbxFileTexture::eClamp:
            return "CLAMP";
        default:
            return "UNKNOWN";
        }
    }
	void Model::writeBinary(FILE* file)
	{
        if(exportPart == EXPORT_PART_ALL || exportPart == EXPORT_PART_MODEL)
        {
            std::list<std::string> _bonenames;
            for (std::vector<Node *>::const_iterator itr = nodes.begin(); itr != nodes.end(); ++itr)
            {
                (*itr)->loadBoneNames(_bonenames);
            }
            for (std::vector<Node *>::const_iterator itr = nodes.begin(); itr != nodes.end(); ++itr)
            {
                bool skeleton=false;
                (*itr)->checkIsSkeleton(skeleton,_bonenames);
                (*itr)->setSkeleton(skeleton);
            }
            unsigned int size = meshes.size();
            if(size>0)
            {
                meshes[0]->object.fPosition = ftell(file);	
            }
            write(size, file);
            // write mesh
            for(auto itr = meshes.begin(); itr != meshes.end(); itr++)
            {
                (*itr)->writeBinary(file);
            }
            
            // write material
            size = materials.size();
            if(size>0)
            {
                materials[0]->object.fPosition = ftell(file);	
            }
            write(size, file);
            for(auto itr = materials.begin(); itr != materials.end(); itr++)
            {
                (*itr)->writeBinary(file);
            }
            // node num
            size = nodes.size();
            if(size>0)
            {
                nodes[0]->object.fPosition = ftell(file);	
            }
            write(size, file);
            for(auto itr = nodes.begin(); itr != nodes.end(); itr++)
            {
                (*itr)->writeBinary(file);
            }
        }

		// animations
        if (exportPart == EXPORT_PART_ALL || exportPart == EXPORT_PART_ANIMATION)
        {
            for(auto itr : animations)
            {
                itr->object.fPosition = ftell(file);
                itr->writeBinary(file);
            }
        }
	}

	void Mesh::writeBinary(FILE* file)
	{
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
            (*itr)->writeBinary(file);
		}
	}
    void  MeshPart::writeBinary(FILE* file)
    {
        write(id, file);
        // indices size
        unsigned int size = indices.size();
        write(size, file);
        // indices.
        for(auto itr1 = indices.begin(); itr1 != indices.end(); itr1++)
            write(*itr1,file);
        //aabb
        write(aabb, 6, file);
    }
	void Attributes::writeBinary(FILE* file)
	{

		std::vector<MeshVertexAttrib> attribs;
		MeshVertexAttrib attrib;
        for (unsigned int i = 0; i < length(); i++)
        {
            std::string key = name(i);
            attrib = attributemap.find(key)->second;
            attribs.push_back(attrib);
            if(key == "VERTEX_ATTRIB_BLEND_INDEX")
            {
                break;
            }
        }
		unsigned int size = attribs.size();
        write(size, file);
        for( int i = 0 ; i < size ; i++ )
        {
            write(attribs[i].size, file);
            write(attribs[i].type, file);
            write(attribs[i].name, file);
        }
	}
	
	void Material::writeBinary(FILE* file)
	{
		write(id, file);
        write(diffuse.value, 3, file);
        write(ambient.value, 3, file);
        write(emissive.value, 3, file);
        write(opacity.value,file);
        write(specular.value, 3, file);
        write(shininess.value,file);
        unsigned int size = textures.size();
        write(size, file);
        for(auto itr = textures.begin(); itr != textures.end(); itr++)
        {
             write((*itr)->id, file);
             write((*itr)->path, file);
             write((*itr)->uvTranslation, 2, file);
             write((*itr)->uvScale, 2, file);
             std::string  wrapModeU=getWrapModeUseString((*itr)->wrapModeU);
             std::string  wrapModeV=getWrapModeUseString((*itr)->wrapModeV);
             std::string  type= getTextureUseString((*itr)->usage);
             write(type,file);
             write(wrapModeU,file);
             write(wrapModeV,file);
        }
	}
	
	void Node::writeBinary(FILE* file)
	{
		write(id, file);
        write(_skeleton, file);
        // rotation scale translation
        write(transforms, 16, file);
		//write(transform.scale, 3, file);
		//write(transform.translation, 3, file);
        // node part
        unsigned int partsSize = parts.size();
		write(partsSize,file);
		if(parts.size()>0)
		{
            for(int i = 0 ; i < parts.size() ; i++ )
            {
                NodePart* nodepart = parts[i];
                if(nodepart)
                {
                    if(nodepart->meshPart)
                    {
                        //meshpartid
                        write(nodepart->meshPart->id, file);
                    }
                    else
                    {
                         write("null", file);
                    }
                    //materialid
                    if(nodepart->material)
                    {
                        write(nodepart->material->id, file);
                    }
                    else
                    {
                         write("null", file);
                    }
                    // bone num
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
                    // uvMapping
                    size = nodepart->uvMapping.size();
                    write(size, file);
                    for(auto itr = nodepart->uvMapping.begin(); itr != nodepart->uvMapping.end(); itr++)
                    {
                        unsigned int size = itr->size();
                        write(size, file);
                        //TextureIndex
                        for (auto tt = (*itr).begin(); tt != (*itr).end(); ++tt)
                        {
                            unsigned int index = nodepart->material->getTextureIndex(*tt);
                            write(index, file);
                        }
                    }
                }
            }
		}
        // children
        unsigned int childrenSize = children.size();
        write(childrenSize,file);
		for(auto itr = children.begin(); itr != children.end(); itr++)
		{
			Node* node = *itr;
            if(node)
            {
                node->writeBinary(file);
            }
		}
	}
	void Animation::writeBinary(FILE* file)
	{
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
                
                // write transform flag
                unsigned char transformflag(0);
                if (keyframe->hasRotation)
                    transformflag |= 0x01;
                if (keyframe->hasScale)
                    transformflag |= (0x01 << 1);
                if (keyframe->hasTranslation)
                    transformflag |= (0x01 << 2);
                
                write(transformflag, file);
                
                // write rotation val
                if (keyframe->hasRotation)
                    write(keyframe->rotation, 4, file);
                
                // write scale val
                if (keyframe->hasScale)
                    write(keyframe->scale, 3, file);
                
                // write translation val
                if (keyframe->hasTranslation)
                    write(keyframe->translation, 3, file);
			}
		}
    }
} 
}