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
#endif //_MSC_VER
#ifndef FBXCONV_READERS_FBXCONVERTER_H
#define FBXCONV_READERS_FBXCONVERTER_H

#include <fbxsdk.h>
#include "../Settings.h"
#include "Reader.h"
#include <sstream>
#include <map>
#include <algorithm>
#include "util.h"
#include "FbxMeshInfo.h"
#include "../log/log.h"


using namespace fbxconv::modeldata;

namespace fbxconv {
namespace readers {
	struct TextureFileInfo {
		std::string path;
		// The uv bounds of this texture that are actually used (x1, y1, x2, y2)
		float bounds[4];
		// The number of nodes that use this texture
		unsigned int nodeCount;
		// The material textures that reference this texture
		std::vector<Material::Texture *> textures;
		TextureFileInfo() : nodeCount(0) {
			memset(bounds, -1, sizeof(float) * 4);
		}
	};

	typedef void (*TextureInfoCallback)(std::map<std::string, TextureFileInfo> &textures);

	bool FbxConverter_ImportCB(void *pArgs, float pPercentage, const char *pStatus);

	class FbxConverter : public Reader {
	public:
		FbxScene *scene;
		FbxManager *manager;

		// Resources (will be disposed)
		std::vector<FbxMeshInfo *> meshInfos;

		// Helper maps/lists, resources in those will not be disposed
		std::map<FbxGeometry *, std::vector<FbxMeshInfo *> > fbxMeshMap;
		std::map<FbxSurfaceMaterial *, Material *> materialsMap;
		std::map<std::string, TextureFileInfo> textureFiles;
		std::map<FbxMeshInfo *, std::vector<std::vector<MeshPart *> > > meshParts; //[FbxMeshInfo][materialIndex][boneIndex]
		std::map<const FbxNode *, Node *> nodeMap;

		Settings *settings;
		fbxconv::log::Log *log;
		TextureInfoCallback textureCallback;

		/** Temp array for transforming uvs, needs to be better defined. */
		Matrix3<float> uvTransforms[8];
		/** The original axis system the FBX file used (always converted defaultUpAxis, defaultFrontAxis and defaultCoordSystem) */
		FbxAxisSystem axisSystem;
		/** The original system units the FBX file used */
		FbxSystemUnit systemUnits;
		static const FbxAxisSystem::EUpVector defaultUpAxis = FbxAxisSystem::eYAxis;
		static const FbxAxisSystem::EFrontVector defaultFrontAxis = FbxAxisSystem::eParityOdd;
		static const FbxAxisSystem::ECoordSystem defaultCoordSystem = FbxAxisSystem::eRightHanded;

		//const char * const &filename, 
		//const bool &packColors = false, const unsigned int &maxVertexCount = (1<<15)-1, const unsigned int &maxIndexCount = (1<<15)-1,
			//const unsigned int &maxVertexBoneCount = 8, const bool &forceMaxVertexBoneCount = false, const unsigned int &maxNodePartBoneCount = (1 << 15)-1, 
			//const bool &flipV = false

		FbxConverter(fbxconv::log::Log *log, TextureInfoCallback textureCallback) 
			:	log(log), scene(0), textureCallback(textureCallback) {

			manager = FbxManager::Create();
			manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));
			manager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
		}

		bool importCallback(float pPercentage, const char *pStatus) {
			log->progress(log::pSourceLoadFbxImport, pPercentage, pStatus);
			return true;
		}

		bool load(Settings *settings) {
			this->settings = settings;

			FbxImporter* const &importer = FbxImporter::Create(manager, "");

			if (settings->verbose)
				importer->SetProgressCallback(FbxConverter_ImportCB, this);

			importer->ParseForGlobalSettings(true);
			importer->ParseForStatistics(true);

			if (importer->Initialize(settings->inFile.c_str(), -1, manager->GetIOSettings())) {
				importer->GetAxisInfo(&axisSystem, &systemUnits);
				scene = FbxScene::Create(manager,"__FBX_SCENE__");
				importer->Import(scene);
			} else {
				log->error(fbxconv::log::eSourceLoadFbxSdk, "Unknown");
            }

			importer->Destroy();

			if (scene) {
				FbxAxisSystem axis(defaultUpAxis, defaultFrontAxis, defaultCoordSystem);
				axis.ConvertScene(scene);
			}
			if (scene)
				checkNodes();
			if (scene)
				prefetchMeshes();
			if (scene)
				fetchMaterials();
			if (scene)
				fetchTextureBounds();
			return !(scene == 0);
		}

		virtual ~FbxConverter() {
			for (std::vector<FbxMeshInfo *>::iterator itr = meshInfos.begin(); itr != meshInfos.end(); ++itr)
				delete (*itr);
			manager->Destroy();
		}

		/** Check all the nodes within the scene for any incompatibility issues. */
		void checkNodes() {
			FbxNode * root = scene->GetRootNode();
			for (int i = 0; i < root->GetChildCount(); i++)
				checkNode(root->GetChild(i));
		}

		/** Recursively check the node for any incompatibility issues. */
		void checkNode(FbxNode * const &node) {
			FbxTransform::EInheritType inheritType;
			node->GetTransformationInheritType(inheritType);
			if (inheritType == FbxTransform::eInheritRrSs) {
				log->warning(log::wSourceLoadFbxNodeRrSs, node->GetName());
				node->SetTransformationInheritType(FbxTransform::eInheritRSrs);
			}
			for (int i = 0; i < node->GetChildCount(); i++)
				checkNode(node->GetChild(i));
		}

		virtual bool convert(Settings *settings, Model * const &model) {
			if (!scene) {
                log->error(log::eSourceLoadGeneral, settings->inFile.c_str());
				return false;
			}
			if (textureCallback)
				textureCallback(textureFiles);
			for (int i = 0; i < 8; i++) {
				uvTransforms[i].idt();
				if (settings->flipV)
					uvTransforms[i].translate(0.f, 1.f).scale(1.f, -1.f);
			}

			for (std::map<FbxSurfaceMaterial *, Material *>::iterator it = materialsMap.begin(); it != materialsMap.end(); ++it) {
				model->materials.push_back(it->second);
				for (std::vector<Material::Texture *>::iterator tt = it->second->textures.begin(); tt != it->second->textures.end(); ++tt)
					(*tt)->path = textureFiles[(*tt)->path].path;
			}
			addMesh(settings,model);
			addNode(model);
			for (std::vector<Node *>::iterator itr = model->nodes.begin(); itr != model->nodes.end(); ++itr)
				updateNode(model, *itr);
			addAnimations(model, scene);
			return true;
		}

		// Only recusively adds the node, doesnt extract any information
		void addNode(Model * const &model, Node * const &parent = 0, FbxNode * const &node = 0) {
			if (node == 0) {
				FbxNode * root = scene->GetRootNode();
				for (int i = 0; i < root->GetChildCount(); i++)
					addNode(model, parent, root->GetChild(i));
				return;
			}
            int i= 0;
            char nodeName[256];
            sprintf(nodeName,"%s",node->GetName());
            while(model->getNode(nodeName))
            {
                 i++;
                 sprintf(nodeName,"%s%d",node->GetName(),i);
                
            }
            if(i>0)
            {
                node->SetName(nodeName);
            }
			/*if (model->getNode(node->GetName())) {

				log->warning(log::wSourceConvertFbxDuplicateNodeId, node->GetName());
				return;
			}*/
			Node *n = new Node(node->GetName());
			n->source = node;
			nodeMap[node] = n;
			if (parent == 0)
				model->nodes.push_back(n);
			else
				parent->children.push_back(n);

			for (int i = 0; i < node->GetChildCount(); i++)
				addNode(model, n, node->GetChild(i));
		}

		void updateNode(Model * const &model, Node * const &node) {
			FbxAMatrix &m = node->source->EvaluateLocalTransform();
			set<3>(node->transform.translation, m.GetT().mData);
			set<4>(node->transform.rotation, m.GetQ().mData);
			set<3>(node->transform.scale, m.GetS().mData);
            for(int i= 0; i < 4; i++)
			{
				for(int j = 0; j < 4; j++)
				node->transforms[i*4 + j] = m.Get(i,j); 
			}

			if (fbxMeshMap.find(node->source->GetGeometry()) != fbxMeshMap.end()) {
                for( const auto& iter: fbxMeshMap[node->source->GetGeometry()]){
                    FbxMeshInfo *meshInfo = iter;
                    std::vector<std::vector<MeshPart *> > &parts = meshParts[meshInfo];
                    const int matCount = node->source->GetMaterialCount();
                    for (int i = 0; i < matCount && i < parts.size(); i++) {
                        Material *material = materialsMap[node->source->GetMaterial(i)];
                        for (int j = 0; j < parts[i].size(); j++) {
                            if (parts[i][j]) {
                                NodePart *nodePart = new NodePart();
                                node->parts.push_back(nodePart);
                                nodePart->material = material;
                                nodePart->meshPart = parts[i][j];
                                for (int k = 0; k < nodePart->meshPart->sourceBones.size(); k++) {
                                    if (nodeMap.find(nodePart->meshPart->sourceBones[k]->GetLink()) != nodeMap.end()) {
                                        std::pair<Node*, FbxAMatrix> p;
                                        p.first = nodeMap[nodePart->meshPart->sourceBones[k]->GetLink()];
                                        getBindPose(node->source, nodePart->meshPart->sourceBones[k], p.second);
                                        nodePart->bones.push_back(p);
                                    }
                                    else {
                                        FbxNode* linknode = nodePart->meshPart->sourceBones[k]->GetLink();
                                        if(linknode)
                                            log->warning(log::wSourceConvertFbxInvalidBone, node->id.c_str(), linknode->GetName());
                                    }
                                }

                                nodePart->uvMapping.resize(meshInfo->uvCount);
                                for (unsigned int k = 0; k < meshInfo->uvCount; k++) {
                                    for (std::vector<Material::Texture *>::iterator it = material->textures.begin(); it != material->textures.end(); ++it) {
                                        FbxFileTexture *texture = (*it)->source;
                                        TextureFileInfo &info = textureFiles[texture->GetFileName()];
                                        if (meshInfo->uvMapping[k] == texture->UVSet.Get().Buffer()) {
                                            nodePart->uvMapping[k].push_back(*it);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
			}

			for (std::vector<Node *>::iterator itr = node->children.begin(); itr != node->children.end(); ++itr)
				updateNode(model, *itr);
		}

		FbxAMatrix convertMatrix(const FbxMatrix& mat)
		{
			FbxVector4 trans, shear, scale;
			FbxQuaternion rot;
			double sign;
			mat.GetElements(trans, rot, shear, scale, sign);
			FbxAMatrix ret;
			ret.SetT(trans);
			ret.SetQ(rot);
			ret.SetS(scale);
			return ret;
		}

		// Get the geometry offset to a node. It is never inherited by the children.
		FbxAMatrix GetGeometry(FbxNode* pNode)
		{
			const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
			const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
			const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

			return FbxAMatrix(lT, lR, lS);
		}

		void getBindPose(FbxNode * target, FbxCluster *cluster, FbxAMatrix &out) {
			if (cluster->GetLinkMode() == FbxCluster::eAdditive)
				log->warning(log::wSourceConvertFbxAdditiveBones, target->GetName());

			FbxAMatrix reference;
			cluster->GetTransformMatrix(reference);
			FbxAMatrix refgem = GetGeometry(target);
			reference *= refgem;
			FbxAMatrix init;
			cluster->GetTransformLinkMatrix(init);
			FbxAMatrix relinit = init.Inverse() * reference;
			out = relinit;
		}

		// Iterate throught the nodes (from the leaves up) and the meshes it references. This might help that meshparts that are closer together are more likely to be merged
		// Note that in the end this is just another way of adding all items in meshInfos.
		void addMesh(Settings *settings, Model * const &model, FbxNode * node = 0) {
			if (node == 0)
				node = scene->GetRootNode();
			const int childCount = node->GetChildCount();
			for (int i = 0; i < childCount; i++)
				addMesh(settings, model, node->GetChild(i));

			FbxGeometry *geometry = node->GetGeometry();
			if (geometry) {
                if (fbxMeshMap.find(geometry) != fbxMeshMap.end()){
                    std::vector<FbxMeshInfo *> meshInfoList = fbxMeshMap.find(geometry)->second;
                    for(const auto& iter : meshInfoList){
                        addMesh(settings, model, iter, node);
                    }
                }
				else
					log->debug("Geometry(%X) of %s not found in fbxMeshMap[size=%d]", (unsigned long)(geometry), node->GetName(), fbxMeshMap.size());
			}
			
		}

		void addMesh(Settings *settings, Model * const &model, FbxMeshInfo * const &meshInfo, FbxNode * const &node) {
			if (meshParts.find(meshInfo) != meshParts.end())
				return;
            
            // if export normalMap
            if(!settings->normalMap)
            {
                meshInfo->attributes.remove(ATTRIBUTE_TANGENT);
                meshInfo->attributes.remove(ATTRIBUTE_BINORMAL);
            }
      
			Mesh *mesh = findReusableMesh(model, meshInfo->attributes, meshInfo->polyCount * 3);
			if (mesh == 0) {
				mesh = new Mesh();
				model->meshes.push_back(mesh);
				mesh->attributes = meshInfo->attributes;
				mesh->vertexSize = mesh->attributes.size();
			}
			std::vector<std::vector<MeshPart *> > &parts = meshParts[meshInfo];
			parts.resize(meshInfo->meshPartCount);
			for (int i = 0; i < meshInfo->meshPartCount; i++) {
				const int n = meshInfo->partBones[i].size();
				const int m = n == 0 ? 1 : n;
				parts[i].resize(m);
				for (int j = 0; j < m; j++) {
					MeshPart *part = new MeshPart();
					part->primitiveType = PRIMITIVETYPE_TRIANGLES;
					parts[i][j] = part;
					mesh->parts.push_back(part);
					if (j < n)
						for (int k = 0; k < meshInfo->partBones[i][j].size(); k++)
							part->sourceBones.push_back(meshInfo->getBone(meshInfo->partBones[i][j][k]));
				}
			}

			float *vertex = new float[mesh->vertexSize];
			unsigned int pidx = 0;
            bool is_error = false;
			for (unsigned int poly = 0; poly < meshInfo->segmentPolyCount; poly++) {
                unsigned int tpoly = meshInfo->polyBegin + poly;
				unsigned int ps = meshInfo->mesh->GetPolygonSize(tpoly);
				int index_1 = meshInfo->polyPartMap[tpoly];
				auto index_2 = meshInfo->polyPartBonesMap[tpoly];
				if(index_1 >= 0 && index_2 >= 0 )
				{
				MeshPart * const &part = parts[index_1][index_2];
				//Material * const &material = materialsMap[node->GetMaterial(meshInfo->polyPartMap[poly])];

					for (unsigned int i = 0; i < ps; i++) {
						const unsigned int v = meshInfo->mesh->GetPolygonVertex(tpoly, i);
						meshInfo->getVertex(vertex, tpoly, pidx, v, uvTransforms);
						part->indices.push_back(mesh->add(vertex));
						pidx++;
					}
				}else {
                    is_error = true;
					}
			}
            if(is_error)
            {
                char warning_str[100];
                sprintf(warning_str,"%s",getGeometryName(meshInfo->mesh));
                log->warning(log::wSourceConvertFbxPolyMaterialInvalid,warning_str);
            }
			int idx = 0;
			for (int i = parts.size() - 1; i >= 0; --i) {
				for (int j = parts[i].size() - 1; j >= 0; --j) {
					MeshPart *part = parts[i][j];
					if (!part->indices.size()) {
						parts[i][j] = 0;
						mesh->parts.erase(std::remove(mesh->parts.begin(), mesh->parts.end(), part), mesh->parts.end());
						log->warning(log::wSourceConvertFbxEmptyMeshpart, node->GetName(), node->GetMaterial(i)->GetName());
						delete part;
					}
					else {
						std::stringstream ss;
						ss << meshInfo->id.c_str() << "_part" << (++idx);
						part->id = ss.str();
					}
				}
			}
            
            if (settings->normalizeVertexNormal) {
                mesh->calcNormal();
            }

            mesh->calcAABB();

			delete[] vertex;
		}

		Mesh *findReusableMesh(Model * const &model, const Attributes &attributes, const unsigned int &vertexCount) {
            if (settings->needReusableMesh) {
                for (std::vector<Mesh *>::iterator itr = model->meshes.begin(); itr != model->meshes.end(); ++itr)
                    if ((*itr)->attributes == attributes && 
                        ((*itr)->vertices.size() / (*itr)->vertexSize) + vertexCount <= settings->maxVertexCount && 
                        (*itr)->indexCount() + vertexCount <= settings->maxIndexCount)
                        return (*itr);
            }
			return 0;
		}

		void fetchTextureBounds(FbxNode *node = 0) {
			if (node == 0)
				node = scene->GetRootNode();
			const int childCount = node->GetChildCount();
			for (int i = 0; i < childCount; i++)
				fetchTextureBounds(node->GetChild(i));

			FbxGeometry *geometry = node->GetGeometry();
			if (fbxMeshMap.find(geometry) == fbxMeshMap.end())
				return;
			FbxMeshInfo *meshInfo = fbxMeshMap[geometry].front();
			const int matCount = node->GetMaterialCount();
			for (int i = 0; i < matCount; i++) {
				FbxSurfaceMaterial *material = node->GetMaterial(i);
				Material *mat = materialsMap[material];
				for (std::vector<Material::Texture *>::iterator it = mat->textures.begin(); it != mat->textures.end(); ++it) {
					FbxFileTexture *texture = (*it)->source;
					TextureFileInfo &info = textureFiles[texture->GetFileName()];
					for (unsigned int k = 0; k < meshInfo->uvCount; k++) {
						if (meshInfo->uvMapping[k] == texture->UVSet.Get().Buffer()) {
							const int idx = 4 * (i * meshInfo->uvCount + k);
							if (*(int*)&info.bounds[0] == -1 || meshInfo->partUVBounds[idx] < info.bounds[0])
								info.bounds[0] = meshInfo->partUVBounds[idx];
							if (*(int*)&info.bounds[1] == -1 || meshInfo->partUVBounds[idx+1] < info.bounds[1])
								info.bounds[1] = meshInfo->partUVBounds[idx+1];
							if (*(int*)&info.bounds[2] == -1 || meshInfo->partUVBounds[idx+2] > info.bounds[2])
								info.bounds[2] = meshInfo->partUVBounds[idx+2];
							if (*(int*)&info.bounds[3] == -1 || meshInfo->partUVBounds[idx+3] > info.bounds[3])
								info.bounds[3] = meshInfo->partUVBounds[idx+3];
							info.nodeCount++;
							break;
						}
					}
				}
			}
		}

		const char *getGeometryName(const FbxGeometry * const &g) {
			static char buff[512];
			const char *name = g->GetName();
			if (name && strlen(name) > 0)
				return name;
			int c = g->GetNodeCount();
			strcpy(buff, "shape(");
			int idx = strlen(buff);
			for (int i = 0; i < c; i++) {
				const char *v = g->GetNode(i)->GetName();
				const int l = strlen(v);
				if (idx + l >= sizeof(buff))
					break;
				if (i > 0)
					buff[idx++] = ',';
				strcpy(&buff[idx], v);
				idx += l;
			}
			buff[idx++] = ')';
			buff[idx] = '\0';
			return buff;
		}

		void prefetchMeshes() {
			{
				int cnt = scene->GetGeometryCount();
				FbxGeometryConverter converter(manager);
				std::vector<FbxGeometry *> triangulate;
				for (int i = 0; i < scene->GetGeometryCount(); ++i) {
					FbxGeometry * geometry = scene->GetGeometry(i);
					if (!geometry->Is<FbxMesh>() || !((FbxMesh*)geometry)->IsTriangleMesh())
						triangulate.push_back(geometry);
				}
				for (std::vector<FbxGeometry *>::iterator it = triangulate.begin(); it != triangulate.end(); ++it)
				{
					log->status(log::sSourceConvertFbxTriangulate, getGeometryName(*it), (*it)->GetClassId().GetName());
					FbxNodeAttribute * const attr = converter.Triangulate(*it, true);
				}
			}
			int cnt = scene->GetGeometryCount();
			for (int i = 0; i < cnt; ++i) {
				FbxGeometry * geometry = scene->GetGeometry(i);
				if (fbxMeshMap.find(geometry) == fbxMeshMap.end()) {
					if (!geometry->Is<FbxMesh>()) {
						log->warning(log::wSourceConvertFbxCantTriangulate, geometry->GetClassId().GetName());
						continue;
					}
					FbxMesh *mesh = (FbxMesh*)geometry;
					int indexCount = (mesh->GetPolygonCount() * 3);
					log->verbose(log::iSourceConvertFbxMeshInfo, getGeometryName(mesh), mesh->GetPolygonCount(), indexCount, mesh->GetControlPointsCount());
					if (indexCount > settings->maxIndexCount)
						log->warning(log::wSourceConvertFbxExceedsIndices, indexCount, settings->maxIndexCount);
					if (mesh->GetElementMaterialCount() <= 0) {
						log->error(log::wSourceConvertFbxNoMaterial, getGeometryName(mesh));
						continue;
					}
                    
                    // if indexCount > maxIndexCount, we will split this mesh.
                    if(indexCount > settings->maxIndexCount){
                        unsigned int polyBegin = 0;
                        std::vector<unsigned int> hashes;
                        for (unsigned int poly = 0; poly < mesh->GetPolygonCount(); poly++) {
                            int ps = mesh->GetPolygonSize(poly);
                            for (unsigned int i = 0; i < ps; i++) {
                                const unsigned int v = mesh->GetPolygonVertex(poly, i);
                                std::vector<unsigned int>::iterator iter = std::find(hashes.begin(), hashes.end(), v);
                                if(iter == hashes.end())
                                {
                                    hashes.push_back(v);
                                }
                            }
                            
                            unsigned int polyCount = 0;
                            if(hashes.size() == settings->maxIndexCount)
                            polyCount = poly - polyBegin;
                            else if(hashes.size() > settings->maxIndexCount)
                            polyCount = poly - polyBegin - 1;
                            
                            if(hashes.size() >= settings->maxIndexCount)
                            {
                                FbxMeshInfo * const info = new FbxMeshInfo(log, mesh, settings->packColors, settings->maxVertexBonesCount, settings->forceMaxVertexBoneCount, settings->maxNodePartBonesCount, polyBegin, polyCount);
                                meshInfos.push_back(info);
                                fbxMeshMap[mesh].push_back(info);
                                if (info->bonesOverflow)
                                log->warning(log::wSourceConvertFbxExceedsBones);
                                
                                polyBegin = polyBegin + polyCount + 1;
                                hashes.clear();
                            }
                        }
                        
                        if(hashes.size() > 0)
                        {
                            unsigned int polyCount = mesh->GetPolygonCount() - polyBegin;
                            FbxMeshInfo * const info = new FbxMeshInfo(log, mesh, settings->packColors, settings->maxVertexBonesCount, settings->forceMaxVertexBoneCount, settings->maxNodePartBonesCount, polyBegin, polyCount);
                            meshInfos.push_back(info);
                            fbxMeshMap[mesh].push_back(info);
                            if (info->bonesOverflow)
                            log->warning(log::wSourceConvertFbxExceedsBones);
                        }
                    }
                    else{
                        FbxMeshInfo * const info = new FbxMeshInfo(log, mesh, settings->packColors, settings->maxVertexBonesCount, settings->forceMaxVertexBoneCount, settings->maxNodePartBonesCount, 0, mesh->GetPolygonCount());
                        meshInfos.push_back(info);
                        fbxMeshMap[mesh].push_back(info);
                        if (info->bonesOverflow)
                        log->warning(log::wSourceConvertFbxExceedsBones);
                    }
				}
				else {
					log->warning(log::wSourceConvertFbxDuplicateMesh, getGeometryName(geometry));
				}
			}
		}

		void fetchMaterials() {
			int cnt = scene->GetMaterialCount();
			for (int i = 0; i < cnt; i++) {
				FbxSurfaceMaterial * const &material = scene->GetMaterial(i);
				if (materialsMap.find(material) == materialsMap.end())
					materialsMap[material] = createMaterial(material);
			}
		}

		Material *createMaterial(FbxSurfaceMaterial * const &material) {	
			Material * const result = new Material();
			result->source = material;
			result->id = material->GetName();
            
			if ((!material->Is<FbxSurfaceLambert>()) || GetImplementation(material, FBXSDK_IMPLEMENTATION_HLSL) || GetImplementation(material, FBXSDK_IMPLEMENTATION_CGFX)) {
				if (!material->Is<FbxSurfaceLambert>())
					log->warning(log::wSourceConvertFbxMaterialUnknown, result->id.c_str());
				if (GetImplementation(material, FBXSDK_IMPLEMENTATION_HLSL))
					log->warning(log::wSourceConvertFbxMaterialHLSL, result->id.c_str());
				if (GetImplementation(material, FBXSDK_IMPLEMENTATION_CGFX))
					log->warning(log::wSourceConvertFbxMaterialCgFX, result->id.c_str());
				result->diffuse.set(1.f, 0.f, 0.f);
				return result;
			}

			FbxSurfaceLambert * const &lambert = (FbxSurfaceLambert *)material;
			if (lambert->Ambient.IsValid())
				result->ambient.set(lambert->Ambient.Get().mData);
			if (lambert->Diffuse.IsValid())
				result->diffuse.set(lambert->Diffuse.Get().mData);
			if (lambert->Emissive.IsValid())
				result->emissive.set(lambert->Emissive.Get().mData);

			addTextures(result->textures, lambert->Ambient, Material::Texture::Ambient);
			addTextures(result->textures, lambert->Diffuse, Material::Texture::Diffuse);
			addTextures(result->textures, lambert->Emissive, Material::Texture::Emissive);
			addTextures(result->textures, lambert->Bump, Material::Texture::Bump);
			addTextures(result->textures, lambert->NormalMap, Material::Texture::Normal);
            addTextures(result->textures, lambert->TransparentColor, Material::Texture::Transparency);

			if (lambert->TransparencyFactor.IsValid() && lambert->TransparentColor.IsValid()) {
				FbxDouble factor = 1.f - lambert->TransparencyFactor.Get();
				FbxDouble3 color = lambert->TransparentColor.Get();
				FbxDouble trans = (color[0] * factor + color[1] * factor + color[2] * factor) / 3.0;
				result->opacity.set((float)trans);
			}
			else if (lambert->TransparencyFactor.IsValid())
				result->opacity.set(1.f - lambert->TransparencyFactor.Get());
			else if (lambert->TransparentColor.IsValid()) {
				FbxDouble3 color = lambert->TransparentColor.Get();
				result->opacity.set((color[0] + color[1] + color[2]) / 3.0);
			}

			if (!material->Is<FbxSurfacePhong>())
				return result;

			FbxSurfacePhong * const &phong = (FbxSurfacePhong *)material;

			if (phong->Specular.IsValid())
				result->specular.set(phong->Specular.Get().mData);
			if (phong->Shininess.IsValid())
				result->shininess.set((float)phong->Shininess.Get());

			addTextures(result->textures, phong->Specular, Material::Texture::Specular);
			addTextures(result->textures, phong->Reflection, Material::Texture::Reflection);
			return result;
		}

        inline void RecursivefindTexture(std::vector<Material::Texture *> &textures,FbxObject * obj,int index,const Material::Texture::Usage &usage )
        {
            auto result = obj->GetSrcObject<FbxFileTexture>(index);
            if(result){
                add_if_not_null(textures, createTexture(result, usage));
                return ;
            }
            else if( obj->GetSrcObject(index)){
                unsigned int n = obj->GetSrcObject(index)->GetSrcObjectCount();
                for(int i = 0; i < n ; i++)
                {
                    auto a = obj->GetSrcObject<FbxTexture>(index);
                    RecursivefindTexture(textures,a,i,usage);
                }
            }
            else if(!obj->GetSrcObjectCount())
            {
                return ;
            }
        }

        inline void findTextureFile(std::vector<Material::Texture *> &textures,const FbxProperty &prop,int index,  const Material::Texture::Usage &usage)
        {
            auto result = prop.GetSrcObject<FbxFileTexture>(index);
            if(result){
                add_if_not_null(textures, createTexture(result, usage));
                return ;
            }
            else if( prop.GetSrcObject(index)){
                unsigned int n = prop.GetSrcObject(index)->GetSrcObjectCount();
                for(int i = 0; i < n ; i++)
                {
                    auto a = prop.GetSrcObject<FbxTexture>(index);
                    RecursivefindTexture(textures,a,i,usage);
                    return;
                }
            }
            else if(!prop.GetSrcObjectCount())
            {
                return ;
            }
        }

		inline void addTextures(std::vector<Material::Texture *> &textures, const FbxProperty &prop,  const Material::Texture::Usage &usage) {
			const unsigned int n = prop.GetSrcObjectCount();
            FbxObject * a = prop.GetSrcObject();
			for (unsigned int i = 0; i < n; i++)
            {
                findTextureFile(textures,prop,i,usage);
            }
		}

		Material::Texture *createTexture(FbxFileTexture * const &texture, const Material::Texture::Usage &usage = Material::Texture::Unknown) {
			if (texture == 0)
				return 0;
			Material::Texture * const result = new Material::Texture();
			result->source = texture;
			result->id = texture->GetName();
			result->path = texture->GetFileName();
			set<2>(result->uvTranslation, texture->GetUVTranslation().mData);
			set<2>(result->uvScale, texture->GetUVScaling().mData);
			result->usage = usage;
            result->wrapModeU = texture->GetWrapModeU();
            result->wrapModeV = texture->GetWrapModeV();
			if (textureFiles.find(result->path) == textureFiles.end())
				textureFiles[result->path].path = result->path;
			textureFiles[result->path].textures.push_back(result);
			return result;
		}

		/** Add the animations if any */
		void addAnimations(Model * const &model, const FbxScene * const &source) {
			const unsigned int animCount = source->GetSrcObjectCount<FbxAnimStack>();
			for (unsigned int i = 0; i < animCount; i++)
				addAnimation(model, source->GetSrcObject<FbxAnimStack>(i));
		}

		/** Add the specified animation to the model */
		void addAnimation(Model *const &model, FbxAnimStack * const &animStack) {
			static std::vector<Keyframe *> frames;
			static std::map<FbxNode *, AnimInfo> affectedNodes;
			affectedNodes.clear();

			FbxTimeSpan animTimeSpan = animStack->GetLocalTimeSpan();
			float animStart = (float)(animTimeSpan.GetStart().GetMilliSeconds());
			float animStop = (float)(animTimeSpan.GetStop().GetMilliSeconds());
			if (animStop <= animStart)
				animStop = 999999999.0f;
            
            //
            static std::map<FbxNode *, std::list<float> > keyframesTimeMap;
            
			// Could also use animStack->GetLocalTimeSpan and animStack->BakeLayers, but its not guaranteed to be correct
			const int layerCount = animStack->GetMemberCount<FbxAnimLayer>();
			for (int l = 0; l < layerCount; l++) {
				FbxAnimLayer *layer = animStack->GetMember<FbxAnimLayer>(l);
				// For each layer check which node is affected and within what time frame and rate
				const int curveNodeCount = layer->GetSrcObjectCount<FbxAnimCurveNode>();
				for (int n = 0; n < curveNodeCount; n++) {
					FbxAnimCurveNode *curveNode = layer->GetSrcObject<FbxAnimCurveNode>(n);
					// Check which properties on this curve are changed
					const int nc = curveNode->GetDstPropertyCount();
					for (int o = 0; o < nc; o++) {
						FbxProperty prop = curveNode->GetDstProperty(o);
						FbxNode *node = static_cast<FbxNode *>(prop.GetFbxObject());
						if (node) {
							FbxString propName = prop.GetName();
							// Only add translation, scaling or rotation
							if ((!node->LclTranslation.IsValid() || propName != node->LclTranslation.GetName()) && 
								(!node->LclScaling.IsValid() || propName != node->LclScaling.GetName()) &&
								(!node->LclRotation.IsValid() || propName != node->LclRotation.GetName()))
								continue;
							FbxAnimCurve *curve;
							AnimInfo ts;
							ts.translate = propName == node->LclTranslation.GetName();
							ts.rotate = propName == node->LclRotation.GetName();
							ts.scale = propName == node->LclScaling.GetName();
                            
                            bool find = false;
                            std::list<float> keyframesTimeList;
                            std::map<FbxNode *, std::list<float> >::iterator iter = keyframesTimeMap.find(node);
                            if (iter != keyframesTimeMap.end())
                                find = true;
                            
							if (curve = prop.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X)){
								
                                updateAnimTime(curve, ts, animStart, animStop);
                                
                                if(settings->compressLevel >= COMPRESS_LEVEL_1)
                                {
                                    if(find)
                                        collectKeyFrames(curve, keyframesTimeMap[node]);
                                    else
                                        collectKeyFrames(curve, keyframesTimeList);
                                }
                            }
                            
							if (curve = prop.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y)){
								
                                updateAnimTime(curve, ts, animStart, animStop);
                                
                                if(settings->compressLevel >= COMPRESS_LEVEL_1)
                                {
                                    if(find)
                                        collectKeyFrames(curve, keyframesTimeMap[node]);
                                    else
                                        collectKeyFrames(curve, keyframesTimeList);
                                }
                            }
                            
							if (curve = prop.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z)){
								
                                updateAnimTime(curve, ts, animStart, animStop);
                                
                                if(settings->compressLevel >= COMPRESS_LEVEL_1)
                                {
                                    if(find)
                                        collectKeyFrames(curve, keyframesTimeMap[node]);
                                    else
                                        collectKeyFrames(curve, keyframesTimeList);
                                }
                            }
                            
							//if (ts.start < ts.stop)
								affectedNodes[node] += ts;
                            
                            if(settings->compressLevel >= COMPRESS_LEVEL_1)
                            {
                                if(!find)
                                    keyframesTimeMap[node] = keyframesTimeList;
                                
                                keyframesTimeMap[node].sort();
                            }
						}
					}
				}
			}

			if (affectedNodes.empty())
				return;

			Animation *animation = new Animation();
			model->animations.push_back(animation);
			animation->id = animStack->GetName();
			animStack->GetScene()->SetCurrentAnimationStack(animStack);
            
			// Add the NodeAnimations to the Animation
			for (std::map<FbxNode *, AnimInfo>::const_iterator itr = affectedNodes.begin(); itr != affectedNodes.end(); itr++) {
				Node *node = model->getNode((*itr).first->GetName());
				if (!node)
					continue;
				frames.clear();
				NodeAnimation *nodeAnim = new NodeAnimation();
				nodeAnim->node = node;
				nodeAnim->translate = (*itr).second.translate;
				nodeAnim->rotate = (*itr).second.rotate;
				nodeAnim->scale = (*itr).second.scale;
				
				FbxTime fbxTime;
				// Calculate all keyframes upfront
                
                if(settings->compressLevel == COMPRESS_LEVEL_1){
                    std::list<float> keytimeList = keyframesTimeMap[(*itr).first];
                    for (const auto& val : keytimeList) {
                        float time = val;
                        time = std::min(time, (*itr).second.stop);
                        if(time < 0) continue;// discard time < 0
                        fbxTime.SetMilliSeconds((FbxLongLong)time);
                        Keyframe *kf = new Keyframe();
                        kf->time = time;// - animStart;
                        FbxAMatrix *m = &(*itr).first->EvaluateLocalTransform(fbxTime);
                        FbxVector4 v = m->GetT();
                        kf->translation[0] = (float)v.mData[0];
                        kf->translation[1] = (float)v.mData[1];
                        kf->translation[2] = (float)v.mData[2];
                        FbxQuaternion q = m->GetQ();
                        kf->rotation[0] = (float)q.mData[0];
                        kf->rotation[1] = (float)q.mData[1];
                        kf->rotation[2] = (float)q.mData[2];
                        kf->rotation[3] = (float)q.mData[3];
                        v = m->GetS();
                        kf->scale[0] = (float)v.mData[0];
                        kf->scale[1] = (float)v.mData[1];
                        kf->scale[2] = (float)v.mData[2];
                        frames.push_back(kf);
                    }
                }
                else{
                    const float stepSize = (*itr).second.framerate <= 0.f ? (*itr).second.stop - (*itr).second.start : 1000.f / (*itr).second.framerate;
                    const float last = (*itr).second.stop + stepSize * 0.5f;
                    
                    for (float time = (*itr).second.start; time <= last; time += stepSize) {
                        time = std::min(time, (*itr).second.stop);
                        if(time < 0) continue;// discard time < 0
                        fbxTime.SetMilliSeconds((FbxLongLong)time);
                        Keyframe *kf = new Keyframe();
                        kf->time = time;// - animStart;
                        FbxAMatrix *m = &(*itr).first->EvaluateLocalTransform(fbxTime);
                        FbxVector4 v = m->GetT();
                        kf->translation[0] = (float)v.mData[0];
                        kf->translation[1] = (float)v.mData[1];
                        kf->translation[2] = (float)v.mData[2];
                        FbxQuaternion q = m->GetQ();
                        kf->rotation[0] = (float)q.mData[0];
                        kf->rotation[1] = (float)q.mData[1];
                        kf->rotation[2] = (float)q.mData[2];
                        kf->rotation[3] = (float)q.mData[3];
                        v = m->GetS();
                        kf->scale[0] = (float)v.mData[0];
                        kf->scale[1] = (float)v.mData[1];
                        kf->scale[2] = (float)v.mData[2];
                        frames.push_back(kf);
                    }
                }
                
                if(frames.size() == 0)
                    continue;

                float length = frames[frames.size()-1]->time ;
                float lengthSec = length / 1000.f;
                if(lengthSec > animation->length)
                    animation->length = lengthSec;

				// Only add keyframes really needed
                addKeyframes(nodeAnim, frames, length);
				if (nodeAnim->rotate || nodeAnim->scale || nodeAnim->translate)
					animation->nodeAnimations.push_back(nodeAnim);
				else
					delete nodeAnim;
			}
		}

		inline void updateAnimTime(FbxAnimCurve *const &curve, AnimInfo &ts, const float &animStart, const float &animStop) {
			FbxTimeSpan fts;
			curve->GetTimeInterval(fts);
			const FbxTime start = fts.GetStart();
			const FbxTime stop = fts.GetStop();
			ts.start = std::max(animStart, std::min(ts.start, (float)(start.GetMilliSeconds())));
			ts.stop = std::min(animStop, std::max(ts.stop, (float)stop.GetMilliSeconds()));
			// Could check the number and type of keys (ie curve->KeyGetInterpolation) to lower the framerate
			ts.framerate = std::max(ts.framerate, (float)stop.GetFrameRate(FbxTime::eDefaultMode));
		}
        
        inline void collectKeyFrames(FbxAnimCurve *const &curve, std::list<float>& keyframesTime)
        {
            int key_cout = curve->KeyGetCount();
            for (int i = 0; i < key_cout; ++i)
            {
                FbxAnimCurveKey curveKey = curve->KeyGet(i);
                FbxTime fbx_time = curveKey.GetTime();
                float keytime = (float)fbx_time.GetMilliSeconds();
                if(keytime < 0) continue;
                
                std::list<float>::iterator iter = std::find(keyframesTime.begin(), keyframesTime.end(), keytime);
                if (iter == keyframesTime.end())
                {
                    keyframesTime.push_back(keytime);
                }
            }
        }

//		void addKeyframes(NodeAnimation *const &anim, std::vector<Keyframe *> &keyframes) {
//			bool translate = false, rotate = false, scale = false;
//			// Check which components are actually changed
//			for (std::vector<Keyframe *>::const_iterator itr = keyframes.begin(); itr != keyframes.end(); ++itr) {
//				if (!translate && !cmp(anim->node->transform.translation, (*itr)->translation, 3))
//					translate = true;
//				if (!rotate && !cmp(anim->node->transform.rotation, (*itr)->rotation, 3))
//					rotate = true;
//				if (!scale && !cmp(anim->node->transform.scale, (*itr)->scale, 3))
//					scale = true;
//			}
//			// This allows to only export the values actual needed
//			anim->translate = translate;
//			anim->rotate = rotate;
//			anim->scale = scale;
//			for (std::vector<Keyframe *>::const_iterator itr = keyframes.begin(); itr != keyframes.end(); ++itr) {
//				(*itr)->hasRotation = rotate;
//				(*itr)->hasScale = scale;
//				(*itr)->hasTranslation = translate;
//			}
//
//			if (!keyframes.empty()) {
//				anim->keyframes.push_back(keyframes[0]);
//				const int last = (int)keyframes.size()-1;
//                float max = keyframes[last]->time;
//				Keyframe *k1 = keyframes[0], *k2, *k3;
//				for (int i = 1; i < last; i++) {
//					k2 = keyframes[i];
//					k3 = keyframes[i+1];
//					// Check if the middle keyframe can be calculated by information, if so dont add it
//					if ((translate && !isLerp(k1->translation, k1->time, k2->translation, k2->time, k3->translation, k3->time, 3)) ||
//						(rotate && !isLerp(k1->rotation, k1->time, k2->rotation, k2->time, k3->rotation, k3->time, 3)) || // FIXME use slerp for quaternions
//						(scale && !isLerp(k1->scale, k1->time, k2->scale, k2->time, k3->scale, k3->time, 3))) {
//                            k2->time /= max;
//							anim->keyframes.push_back(k2);
//							k1 = k2;
//					} else
//						delete k2;
//				}
//				if (last > 0)
//                {
//                    keyframes[last]->time = 1;
//					anim->keyframes.push_back(keyframes[last]);
//                }
//			}
//		}
        
        void addKeyframes(NodeAnimation *const &anim, std::vector<Keyframe *> &keyframes, float timeLength)
        {
            if (!keyframes.empty()) {
                keyframes[0]->hasTranslation = true;
                keyframes[0]->hasRotation = true;
                keyframes[0]->hasScale = true;
                keyframes[0]->time /= timeLength;
                anim->keyframes.push_back(keyframes[0]);
                
                // translation frame
				const int last = (int)keyframes.size()-1;
                //float max = keyframes[last]->time;
				Keyframe *k1 = keyframes[0], *k2, *k3;
				for (int i = 1; i < last; i++) {
					k2 = keyframes[i];
					k3 = keyframes[i+1];
					// Check if the middle keyframe can be calculated by information, if so dont add it
					if (!isLerp(k1->translation, k1->time, k2->translation, k2->time, k3->translation, k3->time, 3)) {
                        k2->hasTranslation = true;
                        k1 = k2;
					}
                    else{
                        k2->hasTranslation = false;
                    }
				}
                
                std::vector<Keyframe *> keyframesTemp;
                // rotation frame
				k1 = keyframes[0];
				for (int i = 1; i < last; i++) {
					k2 = keyframes[i];
					k3 = keyframes[i+1];
					// Check if the middle keyframe can be calculated by information, if so dont add it
					//if (!isLerp(k1->rotation, k1->time, k2->rotation, k2->time, k3->rotation, k3->time, 3))// FIXME use slerp for quaternions
                    if (!isSlerp(k1->rotation, k1->time, k2->rotation, k2->time, k3->rotation, k3->time, 4))
                    {
                        k2->hasRotation = true;
                        k1 = k2;
                        keyframesTemp.push_back(k2);
					}
                    else{
                        k2->hasRotation = false;
                    }
				}
                
                // scale frame
				k1 = keyframes[0];
				for (int i = 1; i < last; i++) {
					k2 = keyframes[i];
					k3 = keyframes[i+1];
					// Check if the middle keyframe can be calculated by information, if so dont add it
					if (!isLerp(k1->scale, k1->time, k2->scale, k2->time, k3->scale, k3->time, 3)) {
                        k2->hasScale = true;
                        k1 = k2;
					} else{
						k2->hasScale = false;
                    }
				}
                
                // check, if keyframe has translation or scale or rotation then push it to anim's key frames
                for (int i = 1; i < last; i++) {
					k2 = keyframes[i];
                    if(k2->hasTranslation || k2->hasScale || k2->hasRotation){
                        k2->time /= timeLength;
                        anim->keyframes.push_back(k2);
                    }
                    else{
                        delete k2;
                    }
				}
                
				if (last > 0)
                {
                    keyframes[last]->time /= timeLength;
                    keyframes[last]->hasTranslation = true;
                    keyframes[last]->hasRotation = true;
                    keyframes[last]->hasScale = true;
					anim->keyframes.push_back(keyframes[last]);
                }
			}
        }

		inline bool cmp(const float &v1, const float &v2, const float &epsilon = 0.000001) {
			const double d = v1 - v2;
			return ((d < 0.f) ? -d : d) < epsilon;
		}

		inline bool cmp(const float *v1, const float *v2, const unsigned int &count) {
			for (unsigned int i = 0; i < count; i++)
				if (!cmp(v1[i],v2[i]))
					return false;
			return true;
		}

		inline bool isLerp(const float *v1, const float &t1, const float *v2, const float &t2, const float *v3, const float &t3, const int size) {
			const double d = (t2 - t1) / (t3 - t1);
			for (int i = 0; i < size; i++)
				if (!cmp(v2[i], v1[i] + d * (v3[i] - v1[i])))
					return false;
			return true;
		}
        
        inline bool isSlerp(const float *v1, const float &t1, const float *v2, const float &t2, const float *v3, const float &t3, const int size)
        {
            assert(size==4);
            const double d = (t2 - t1) / (t3 - t1);
            float q[4] = {0};
            slerp(v1[0], v1[1], v1[2], v1[3], v3[0], v3[1], v3[2], v3[3], d, &q[0], &q[1], &q[2], &q[3]);
            
            if (!cmp(q, v2, size))
                return false;
            return true;
        }
        
        void slerp(float q1x, float q1y, float q1z, float q1w, float q2x, float q2y, float q2z, float q2w, float t, float* dstx, float* dsty, float* dstz, float* dstw)
        {
            assert(dstx && dsty && dstz && dstw);
            assert(!(t < 0.0f || t > 1.0f));
            
            if (t == 0.0f)
            {
                *dstx = q1x;
                *dsty = q1y;
                *dstz = q1z;
                *dstw = q1w;
                return;
            }
            else if (t == 1.0f)
            {
                *dstx = q2x;
                *dsty = q2y;
                *dstz = q2z;
                *dstw = q2w;
                return;
            }
            
            if (q1x == q2x && q1y == q2y && q1z == q2z && q1w == q2w)
            {
                *dstx = q1x;
                *dsty = q1y;
                *dstz = q1z;
                *dstw = q1w;
                return;
            }
            
            float halfY, alpha, beta;
            float u, f1, f2a, f2b;
            float ratio1, ratio2;
            float halfSecHalfTheta, versHalfTheta;
            float sqNotU, sqU;
            
            float cosTheta = q1w * q2w + q1x * q2x + q1y * q2y + q1z * q2z;
            
            // As usual in all slerp implementations, we fold theta.
            alpha = cosTheta >= 0 ? 1.0f : -1.0f;
            halfY = 1.0f + alpha * cosTheta;
            
            // Here we bisect the interval, so we need to fold t as well.
            f2b = t - 0.5f;
            u = f2b >= 0 ? f2b : -f2b;
            f2a = u - f2b;
            f2b += u;
            u += u;
            f1 = 1.0f - u;
            
            // One iteration of Newton to get 1-cos(theta / 2) to good accuracy.
            halfSecHalfTheta = 1.09f - (0.476537f - 0.0903321f * halfY) * halfY;
            halfSecHalfTheta *= 1.5f - halfY * halfSecHalfTheta * halfSecHalfTheta;
            versHalfTheta = 1.0f - halfY * halfSecHalfTheta;
            
            // Evaluate series expansions of the coefficients.
            sqNotU = f1 * f1;
            ratio2 = 0.0000440917108f * versHalfTheta;
            ratio1 = -0.00158730159f + (sqNotU - 16.0f) * ratio2;
            ratio1 = 0.0333333333f + ratio1 * (sqNotU - 9.0f) * versHalfTheta;
            ratio1 = -0.333333333f + ratio1 * (sqNotU - 4.0f) * versHalfTheta;
            ratio1 = 1.0f + ratio1 * (sqNotU - 1.0f) * versHalfTheta;
            
            sqU = u * u;
            ratio2 = -0.00158730159f + (sqU - 16.0f) * ratio2;
            ratio2 = 0.0333333333f + ratio2 * (sqU - 9.0f) * versHalfTheta;
            ratio2 = -0.333333333f + ratio2 * (sqU - 4.0f) * versHalfTheta;
            ratio2 = 1.0f + ratio2 * (sqU - 1.0f) * versHalfTheta;
            
            // Perform the bisection and resolve the folding done earlier.
            f1 *= ratio1 * halfSecHalfTheta;
            f2a *= ratio2;
            f2b *= ratio2;
            alpha *= f1 + f2a;
            beta = f1 + f2b;
            
            // Apply final coefficients to a and b as usual.
            float w = alpha * q1w + beta * q2w;
            float x = alpha * q1x + beta * q2x;
            float y = alpha * q1y + beta * q2y;
            float z = alpha * q1z + beta * q2z;
            
            // This final adjustment to the quaternion's length corrects for
            // any small constraint error in the inputs q1 and q2 But as you
            // can see, it comes at the cost of 9 additional multiplication
            // operations. If this error-correcting feature is not required,
            // the following code may be removed.
            f1 = 1.5f - 0.5f * (w * w + x * x + y * y + z * z);
            *dstw = w * f1;
            *dstx = x * f1;
            *dsty = y * f1;
            *dstz = z * f1;
        }

		template<int n> inline static void set(float * const &dest, const FbxDouble * const &source) {
			for (int i = 0; i < n; i++)
				dest[i] = (float)source[i];
		}

		template<int n> inline static void set(double * const &dest, const FbxDouble * const &source) {
			for (int i = 0; i < n; i++)
				dest[i] = source[i];
		}

		template<class T> inline static void add_if_not_null(std::vector<T *> &dst, T * const &value) {
			if (value != 0)
				dst.push_back(value);
		}
	};

	bool FbxConverter_ImportCB(void *pArgs, float pPercentage, const char *pStatus) {
		return ((FbxConverter*)pArgs)->importCallback(pPercentage, pStatus);
	}
} }
#endif //FBXCONV_READERS_FBXCONVERTER_H