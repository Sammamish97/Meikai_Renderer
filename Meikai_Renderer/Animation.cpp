#include "Animation.h"
#include "DXApp.h"

#include <cassert>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Animation::Animation(DXApp* appPtr, const std::string& file_path)
	:mApp(appPtr)
{
	pScene = mImporter.ReadFile(file_path, 0);
    if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
    {
        assert("Fail to Load %s", file_path.c_str());
    }

    if(pScene->mNumAnimations > 1)
    {
        assert("Need to split animations");
    }
    //Animation instance only care one animation.
    mAnimation = pScene->mAnimations[0];
}

aiNodeAnim* Animation::FindNodeAnim(const std::string& nodeName)
{
    for (UINT i = 0; i < mAnimation->mNumChannels; i++) 
    {
        aiNodeAnim* pNodeAnim = mAnimation->mChannels[i];

        if (std::string(pNodeAnim->mNodeName.data) == nodeName) 
        {
            return pNodeAnim;
        }
    }
    return nullptr;
}

aiMatrix4x4 Animation::CalcNodeTransformation(aiNodeAnim* pNodeAnim, float AnimationTimeTicks)
{
    // Interpolate scaling and generate scaling transformation matrix
    aiVector3D Scaling;
    CalcInterpolatedScaling(Scaling, AnimationTimeTicks, pNodeAnim);
    aiMatrix4x4 ScalingM;
    ScalingM.Scaling(Scaling, ScalingM);

    // Interpolate rotation and generate rotation transformation matrix
    aiQuaternion RotationQ;
    CalcInterpolatedRotation(RotationQ, AnimationTimeTicks, pNodeAnim);
    aiMatrix4x4 RotationM = aiMatrix4x4(RotationQ.GetMatrix());
    //마지막 원소 1로 되는지 확인 해야함.

    // Interpolate translation and generate translation transformation matrix
    aiVector3D Translation;
    CalcInterpolatedPosition(Translation, AnimationTimeTicks, pNodeAnim);
    aiMatrix4x4 TranslationM;
    TranslationM.Translation(Translation, TranslationM);

    // Combine the above transformations
    return TranslationM * RotationM * ScalingM;
}

void Animation::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumPositionKeys == 1) 
    {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    unsigned PositionIndex = FindPosition(AnimationTimeTicks, pNodeAnim);
    unsigned NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float)pNodeAnim->mPositionKeys[PositionIndex].mTime;
    float t2 = (float)pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
    float DeltaTime = t2 - t1;
    float Factor = (AnimationTimeTicks - t1) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}

unsigned Animation::FindPosition(float animationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    for (unsigned i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) 
    {
        float t = (float)pNodeAnim->mPositionKeys[i + 1].mTime;
        if (animationTimeTicks < t)
        {
            return i;
        }
    }

    return 0;
}

void Animation::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    unsigned RotationIndex = FindRotation(AnimationTimeTicks, pNodeAnim);
    unsigned NextRotationIndex = RotationIndex + 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float)pNodeAnim->mRotationKeys[RotationIndex].mTime;
    float t2 = (float)pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
    float DeltaTime = t2 - t1;
    float Factor = (AnimationTimeTicks - t1) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out.Normalize();
}

unsigned Animation::FindRotation(float animationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (unsigned i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
    {
        float t = (float)pNodeAnim->mRotationKeys[i + 1].mTime;
        if (animationTimeTicks < t)
        {
            return i;
        }
    }

    return 0;
}

void Animation::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    unsigned ScalingIndex = FindScaling(AnimationTimeTicks, pNodeAnim);
    unsigned NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    float t2 = (float)pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
    float DeltaTime = t2 - t1;
    float Factor = (AnimationTimeTicks - (float)t1) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}

unsigned Animation::FindScaling(float animationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (unsigned i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
    {
        float t = (float)pNodeAnim->mScalingKeys[i + 1].mTime;
        if (animationTimeTicks < t)
        {
            return i;
        }
    }

    return 0;
}
