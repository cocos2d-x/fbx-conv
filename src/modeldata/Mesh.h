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
#ifdef _MSC_VER 
#pragma once
#endif
#ifndef MODELDATA_MESH_H
#define MODELDATA_MESH_H

#include <vector>
#include "MeshPart.h"
#include "Attributes.h"
#include "../json/BaseJSONWriter.h"
#include "Reference.h"

namespace fbxconv {
namespace modeldata {
    
    struct PN{
        FbxVector4 position;
        FbxVector4 normal;
    };
    
	/** A mesh is responsable for freeing all parts and vertices it contains. */
	struct Mesh : public json::ConstSerializable {
		/** the attributes the vertices in this mesh describe */
		Attributes attributes;
		/** the size (in number of floats) of each vertex */
		unsigned int vertexSize;
		/** the vertices that this mesh contains */
		std::vector<float> vertices;
		/** hash lookup table for faster duplicate vertex checking */
		std::vector<unsigned int> hashes;
		/** the indexed parts of this mesh */
		std::vector<MeshPart *> parts;
        std::string id;
        
        /** save position and normal */
        std::list<PN> pnList;
        
		/** ctor */
		Mesh() : attributes(0), vertexSize(0) {
		}

		/** copy constructor */
		Mesh(const Mesh &copyFrom) {
			attributes = copyFrom.attributes;
			vertexSize = copyFrom.vertexSize;
			vertices.insert(vertices.end(), copyFrom.vertices.begin(), copyFrom.vertices.end());
			for (std::vector<MeshPart *>::const_iterator itr = copyFrom.parts.begin(); itr != copyFrom.parts.end(); ++itr)
				parts.push_back(new MeshPart(**itr));
		}

		~Mesh() {
			clear();
		}

		void clear() {
			vertices.clear();
			hashes.clear();
			attributes = vertexSize = 0;
			for (std::vector<MeshPart *>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
				delete (*itr);
			parts.clear();
            pnList.clear();
		}

		inline unsigned int indexCount() {
			unsigned int result = 0;
			for (std::vector<MeshPart *>::const_iterator itr = parts.begin(); itr != parts.end(); ++itr)
				result += (unsigned int)(*itr)->indices.size();
			return result;
		}

		inline unsigned int add(const float *vertex) {
			const unsigned int hash = calcHash(vertex, vertexSize);
			const unsigned int n = (unsigned int)hashes.size();
			for (unsigned int i = 0; i < n; i++)
				if ((hashes[i] == hash) && compare(&vertices[i*vertexSize], vertex, vertexSize))
					return i;
			hashes.push_back(hash);
			vertices.insert(vertices.end(), &vertex[0], &vertex[vertexSize]);
			return (unsigned int)hashes.size() - 1;
		}

		inline unsigned int calcHash(const float *vertex, const unsigned int size) {
			unsigned int result = 0;
			for (unsigned int i = 0; i < size; i++)
				result += ((*((unsigned int *)&vertex[i])) & 0xffffff00) >> 8;
			return result;
		}

		inline bool compare(const float* lhs, const float* rhs, const unsigned int &n) {
			for (unsigned int i = 0; i < n; i++)
				if ((*(unsigned int*)&lhs[i] != *(unsigned int*)&rhs[i]) && lhs[i] != rhs[i])
					return false;
			return true;
		}
        
        inline bool addN(const FbxVector4& position, const FbxVector4& normal){
            std::list<PN>::iterator iter = pnList.begin();
            for (; iter != pnList.end(); iter++) {
                if (iter->position == position) {
                    iter->normal += normal;
                    return true;
                }
            }
            
            PN pn;
            pn.position = position;
            pn.normal = normal;
            pnList.push_back(pn);
            return false;
        }
        
        inline bool getN(const FbxVector4& position, FbxVector4& normal){
            std::list<PN>::iterator iter = pnList.begin();
            for (; iter != pnList.end(); iter++) {
                if (iter->position == position) {
                    normal = iter->normal;
                    return true;
                }
            }
            return false;
        }
        
        inline void calcNormal(){
            int stride = attributes.size();
            unsigned int num = vertices.size();
            for (int i = 0; i < num; i += stride) {
                // get position
                float *position = &vertices[i];
                FbxVector4 pos(position[0], position[1], position[2]);
                
                // get normal
                float *normal = &vertices[i + 3];
                FbxVector4 tnormal(normal[0], normal[1], normal[2]);
            
                // sum the target position's normal
                addN(pos, tnormal);
            }
            
            // normalize the normal in pnList
            std::list<PN>::iterator iter = pnList.begin();
            for (; iter != pnList.end(); iter++) {
                iter->normal.Normalize();
            }
            
            // modify the vertex buffer data
            for (int i = 0; i < num; i += stride) {
                // get position
                float *position = &vertices[i];
                FbxVector4 pos(position[0], position[1], position[2]);
                
                float *normal = &vertices[i + 3];
                FbxVector4 tnormal;
                getN(pos, tnormal);
                normal[0] = tnormal[0];
                normal[1] = tnormal[1];
                normal[2] = tnormal[2];
            }
        }

        inline void calcAABB(){
            int stride = attributes.size();
            unsigned int num = vertices.size();

            for (unsigned int i = 0; i < parts.size(); ++i){
                auto meshPart = parts[i];
                for (auto iter : meshPart->indices){
                    float *position = &vertices[iter * stride];
                    //update aabb min
                    meshPart->aabb[0] = position[0] < meshPart->aabb[0]? position[0]: meshPart->aabb[0];
                    meshPart->aabb[1] = position[1] < meshPart->aabb[1]? position[1]: meshPart->aabb[1];
                    meshPart->aabb[2] = position[2] < meshPart->aabb[2]? position[2]: meshPart->aabb[2];
                    //update aabb max
                    meshPart->aabb[3] = position[0] > meshPart->aabb[3]? position[0]: meshPart->aabb[3];
                    meshPart->aabb[4] = position[1] > meshPart->aabb[4]? position[1]: meshPart->aabb[4];
                    meshPart->aabb[5] = position[2] > meshPart->aabb[5]? position[2]: meshPart->aabb[5];
                }
            }
        }
        
        ObjRef object;
		ObjRef* GetObj() 
		{
			object.tpyeid = MESH_ID;
			object.fPosition = 0;
            object.id = id+"mesh";
			return &object;
		}
		virtual void serialize(json::BaseJSONWriter &writer) const;
        void writeBinary(FILE* file);
	};
}
}

#endif //MODELDATA_MESH_H
