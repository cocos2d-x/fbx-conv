/*******************************************************************************
 * Copyright 2011 See AUTHORS file.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/
/** @author Xoppa */
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

namespace fbxconv {
namespace modeldata {

std::string versions = "0.2";

static const char* getPrimitiveTypeString(const int &primitiveTypeId) {
	switch(primitiveTypeId){
	case 0:
		return "POINTS";
	case 1:
		return "LINES";
	case 3:
		return "LINE_STRIP";
	case 4:
		return "TRIANGLES";
	case 5:
		return "TRIANGLE_STRIP";
	default:
		return "UNKNOWN";
	}
}

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

void Model::serialize(json::BaseJSONWriter &writer) const {
	writer.obj(6);
	writer << "version" = versions;
	//writer << "file_type" = "c3t";
	writer << "mesh" = meshes;
	writer << "material" = materials;
	writer << "skin" = nodes;
	writer << "animation" = animations;
	writer.end();
}

void Mesh::serialize(json::BaseJSONWriter &writer) const {
	writer << json::obj;
	//writer << "version" = version;
	writer << "attributes" = attributes;

	writer.val("vertex").is().arr();
	writer << json::obj;
	writer.val("vertices").is().data(vertices, vertexSize);
	
	writer << "vertexsize" = vertices.size();

	writer << json::end;
	writer.end();
	writer << "submesh" = parts;


	
		writer << json::end;

}

void Attributes::serialize(json::BaseJSONWriter &writer) const {
	const unsigned int len = length();
	writer.arr(len, 8);
	for (unsigned int i = 0; i < len; i++)
	{
		writer << json::obj;
		//const char *test = name(i);
		//writer.val(name(i));

		int size;
		std::string type;
		//std::string name;

		MeshVertexAttrib v;
		std::string key = name(i);
		v = attributemap.find(key)->second;
		writer << "size" << v.size;
		writer << "type" << v.type;
		writer << "attribute" << v.name;
		writer << json::end;
		if(i >=4)
			break;
	}
	writer.end();
}

void MeshPart::serialize(json::BaseJSONWriter &writer) const {
	writer.obj(3);
	writer << "id" = id;
	//writer << "type" = getPrimitiveTypeString(primitiveType);
	writer.val("indices").is().data(indices, 12);
	writer << "indexnum" << indices.size();
	writer << json::end;
}

void Material::serialize(json::BaseJSONWriter &writer) const {
	writer << json::obj;
	//writer << "version" = version;
	//writer << "id" = id;
	//if (ambient[0] != 0.f || ambient[1] != 0.f || ambient[2] != 0.f)
	//	writer << "ambient" = ambient;
	//if (diffuse[0] != 0.f || diffuse[1] != 0.f || diffuse[2] != 0.f)
	//	writer << "diffuse" = diffuse;
	//if (emissive[0] != 0.f || emissive[1] != 0.f || emissive[2] != 0.f)
	//	writer << "emissive" = emissive;
	//if (opacity != 1.f)
	//	writer << "opacity" = opacity;
	//if (specular[0] != 0.f || specular[1] != 0.f || specular[2] != 0.f)
	//	writer << "specular" = specular;
	//if (shininess != 0.f)
	//	writer << "shininess" = shininess;
	if (!textures.empty())
	{
		//writer << ""<< textures;
		for(auto i = 0; i < textures.size(); i++)
		{
			writer << "id" = textures[i]->id;
			writer << "textures" << textures[i]->path;
		}
	}
	writer << json::end;
}

void Material::Texture::serialize(json::BaseJSONWriter &writer) const {
	writer << json::obj;
	//writer << "id" = id;
	writer << "filename" = path;
	//if (uvTranslation[0] != 0.f || uvTranslation[1] != 0.f)
	//	writer << "uvtranslation" = uvTranslation;
	//if (uvScale[0] != 1.f || uvScale[1] != 1.f)
	//	writer << "uvscaling" = uvScale;
	//writer << "type" = getTextureUseString(usage);
	writer << json::end;
}


template<class T, size_t n> void writeAsFloat(json::BaseJSONWriter &writer, const char *k, const T(&v)[n]) {
	static float tmp[n];
	for (int i = 0; i < n; ++i)
		tmp[i] = (float)v[i];
	writer << k << tmp;
}

void Node::serialize(json::BaseJSONWriter &writer) const {
	writer << json::obj;
//	writer << "version" = 1.2;
	writer << "id" = id;
	//if (transform.rotation[0] != 0. || transform.rotation[1] != 0. || transform.rotation[2] != 0. || transform.rotation[3] != 1.)
	//	writer << "rotation" = transform.rotation;
	//if (transform.scale[0] != 1. || transform.scale[1] != 1. || transform.scale[2] != 1.)
	//	writer << "scale" = transform.scale;
	//if (transform.translation[0] != 0. || transform.translation[1] != 0. || transform.translation[2] != 0.)
	//	writer << "translation" = transform.translation;

	writer << "tansform" << transforms; 


	if (!parts.empty())
	{
		//writer << "parts" = parts;
		if (!parts[0]->bones.empty()) {
		writer.val("bones").is().arr();
		for (std::vector<std::pair<Node *, FbxAMatrix> >::const_iterator it = parts[0]->bones.begin(); it != parts[0]->bones.end(); ++it) {
			writer << json::obj;
			writer << "node" = it->first->id;
			//writeAsFloat(writer, "translation", it->second.GetT().mData);
			//writeAsFloat(writer, "rotation", it->second.GetQ().mData);
			//writeAsFloat(writer, "scale", it->second.GetS().mData);
			//writeAsFloat(writer, "transform", it->second.Double44());
			float tmp[16];
			for(int i = 0; i < 4; i++)
			{
				for(int j = 0; j < 4; j++)
				{
					tmp[i*4 + j] = it->second.Double44()[i][j];
				}
			}
			writeAsFloat(writer, "bindshape", tmp);

			writer << json::end;
		}
		writer.end();
	}
	}
	
	if (!children.empty())
		writer << "children" = children;
	writer << json::end;
}


void NodePart::serialize(json::BaseJSONWriter &writer) const {
	writer << json::obj;
	writer << "meshpartid" = meshPart->id;
	writer << "materialid" = material->id;
	if (!bones.empty()) {
		writer.val("bones").is().arr();
		for (std::vector<std::pair<Node *, FbxAMatrix> >::const_iterator it = bones.begin(); it != bones.end(); ++it) {
			writer << json::obj;
			writer << "node" = it->first->id;
			//writeAsFloat(writer, "translation", it->second.GetT().mData);
			//writeAsFloat(writer, "rotation", it->second.GetQ().mData);
			//writeAsFloat(writer, "scale", it->second.GetS().mData);
			//writeAsFloat(writer, "transform", it->second.Double44());
			float tmp[16];
			for(int i = 0; i < 4; i++)
			{
				for(int j = 0; j < 4; j++)
				{
					tmp[i*4 + j] = it->second.Double44()[i][j];
				}
			}
			writeAsFloat(writer, "transform", tmp);

			writer << json::end;
		}
		writer.end();
	}
	//if (!uvMapping.empty()) {
	//	writer.val("uvMapping").is().arr(uvMapping.size(), 16);
	//	for (std::vector<std::vector<Material::Texture *> >::const_iterator it = uvMapping.begin(); it != uvMapping.end(); ++it) {
	//		writer.arr((*it).size(), 16);
	//		for (std::vector<Material::Texture *>::const_iterator tt = (*it).begin(); tt != (*it).end(); ++tt)
	//			writer << material->getTextureIndex(*tt);
	//		writer.end();
	//	}
	//	writer.end();
	//}
	writer << json::end;
}

void Animation::serialize(json::BaseJSONWriter &writer) const {
	writer.obj(3);
	//writer << "version" << version;
	writer << "id" = id;
	writer << "length" = length;
	writer << "bones" = nodeAnimations;
	writer.end();
}

void NodeAnimation::serialize(json::BaseJSONWriter &writer) const {
	writer.obj(2);
	writer << "boneId" = node->id;
	writer << "keyframes" = keyframes;
	writer.end();
}

void Keyframe::serialize(json::BaseJSONWriter &writer) const {
	writer << json::obj;
	writer << "keytime" = time;
	//if (hasRotation)
		writer << "rotation" = rotation;
	//if (hasScale)
		writer << "scale" = scale;
	//if (hasTranslation)
		writer << "translation" = translation;
	writer << json::end;
}

} }