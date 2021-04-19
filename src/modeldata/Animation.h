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
#ifndef MODELDATA_ANIMATION_H
#define MODELDATA_ANIMATION_H

#include "../json/BaseJSONWriter.h"
#include "Node.h"
#include "NodeAnimation.h"
#include "Reference.h"
#include <vector>
namespace fbxconv {
namespace modeldata {
struct Node;

struct Animation final : public json::ConstSerializable {
    std::string id;
    float length;
    std::vector<NodeAnimation *> nodeAnimations;

    Animation() : length(0) {}

    Animation(const Animation &copyFrom) {
        id = copyFrom.id;
        for (std::vector<NodeAnimation *>::const_iterator itr =
                 copyFrom.nodeAnimations.begin();
             itr != copyFrom.nodeAnimations.end(); ++itr)
            nodeAnimations.push_back(new NodeAnimation(*(*itr)));
    }

    ~Animation() {
        for (std::vector<NodeAnimation *>::iterator itr =
                 nodeAnimations.begin();
             itr != nodeAnimations.end(); ++itr)
            if ((*itr) != 0)
                delete *itr;
    }
    ObjRef object;
    ObjRef *GetObj() {
        object.tpyeid = ANIMATIONS_ID;
        object.fPosition = 0;
        object.id = id + "animation";
        return &object;
    }
    virtual void serialize(json::BaseJSONWriter &writer) const;
    void writeBinary(FILE *file);
};
} // namespace modeldata
} // namespace fbxconv

#endif // MODELDATA_ANIMATION_H
