//--------------------------------------------------------------------------------------
// File: Animation.cpp
//
// Simple animation playback system for CMO and SDKMESH for DirectX Tool Kit
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Animation.h"
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <windows.h>
using namespace DX;
using namespace DirectX;

//--------------------------------------------------------------------------------------
// DirectX SDK SDKMESH animation
//--------------------------------------------------------------------------------------
namespace
{
#pragma pack(push,8)

	static constexpr uint32_t SDKMESH_FILE_VERSION = 101;
	static constexpr uint32_t MAX_FRAME_NAME = 100;

	struct SDKANIMATION_FILE_HEADER
	{
		uint32_t Version;
		uint8_t  IsBigEndian;
		uint32_t FrameTransformType;
		uint32_t NumFrames;
		uint32_t NumAnimationKeys;
		uint32_t AnimationFPS;
		uint64_t AnimationDataSize;
		uint64_t AnimationDataOffset;
	};

	static_assert(sizeof(SDKANIMATION_FILE_HEADER) == 40, "SDK Mesh structure size incorrect");

	struct SDKANIMATION_DATA
	{
		XMFLOAT3 Translation;
		XMFLOAT4 Orientation;
		XMFLOAT3 Scaling;
	};

	static_assert(sizeof(SDKANIMATION_DATA) == 40, "SDK Mesh structure size incorrect");

	struct SDKANIMATION_FRAME_DATA
	{
		char FrameName[MAX_FRAME_NAME];
		union
		{
			uint64_t DataOffset;
			SDKANIMATION_DATA* pAnimationData;
		};
	};

	static_assert(sizeof(SDKANIMATION_FRAME_DATA) == 112, "SDK Mesh structure size incorrect");

#pragma pack(pop)
}

// コンストラクタ
AnimationSDKMESH::AnimationSDKMESH() noexcept
	:
	m_startTime(0.0),				// 開始時間
	m_endTime(0.0),				// 終了時間
	m_animTime(0.0),				// アニメーション時間
	m_animData{},					// アニメーションデータ
	m_animSize(0),					// アニメーションサイズ
	m_boneToTrack{},				// ボーンからトラック 
	m_animBones{},					// アニメーションボーン
	m_boneNumber{},				// ボーン数
	m_boneTransforms{},		// ボーントランスフォーム
	m_loopCount(0),        // ループカウント
	m_loopOnce(false),    // 一度だけ再生するかどうか
	m_finished(false)    // アニメーションが終了したかどうか
{
}

// アニメーションデータをロードする
HRESULT AnimationSDKMESH::Load(_In_z_ const wchar_t* fileName)
{
	// 解放する
	Release();

	if (!fileName)
		return E_INVALIDARG;
	// ファイルストリームを宣言する
	std::ifstream inFile(fileName, std::ios::in | std::ios::binary | std::ios::ate);
	if (!inFile)
		return E_FAIL;
	// ファイルのサイズを取得する
	std::streampos length = inFile.tellg();
	if (!inFile)
		return E_FAIL;

	if (length < sizeof(SDKANIMATION_FILE_HEADER))
		return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

	if (length > UINT32_MAX)
		return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
	// ファイルを読み込む領域を確保する
	std::unique_ptr<uint8_t[]> blob(new (std::nothrow) uint8_t[size_t(length)]);
	if (!blob)
		return E_OUTOFMEMORY;
	// シークポインタを先頭に移動する
	inFile.seekg(0, std::ios::beg);
	if (!inFile)
		return E_FAIL;
	// ファイルを読み込む
	inFile.read(reinterpret_cast<char*>(blob.get()), length);
	if (!inFile)
		return E_FAIL;
	// ファイルストリームをクローズする
	inFile.close();
	// ヘッダーを取得する
	auto header = reinterpret_cast<const SDKANIMATION_FILE_HEADER*>(blob.get());

	if (header->Version != SDKMESH_FILE_VERSION
		|| header->IsBigEndian != 0
		|| header->FrameTransformType != 0 /*FTT_RELATIVE*/
		|| header->NumAnimationKeys == 0
		|| header->NumFrames == 0
		|| header->AnimationFPS == 0)
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	// データサイズを計算する
	uint64_t dataSize = header->AnimationDataOffset + header->AnimationDataSize;
	if (dataSize > uint64_t(length))
		return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

	m_animData.swap(blob);
	// アニメーションサイズを計算する
	m_animSize = static_cast<size_t>(length);
	return S_OK;
}

// モデルとボーンをバインドする
bool AnimationSDKMESH::Bind(const Model& model)
{
	assert(m_animData && m_animSize > 0);

	if (model.bones.empty())
		return false;

	auto header = reinterpret_cast<const SDKANIMATION_FILE_HEADER*>(m_animData.get());
	assert(header->Version == SDKMESH_FILE_VERSION);
	auto frameData = reinterpret_cast<SDKANIMATION_FRAME_DATA*>(m_animData.get() + header->AnimationDataOffset);

	uint32_t fps = header->AnimationFPS;
	uint32_t keys = header->NumAnimationKeys;

	//  Byte swap if values look corrupted (too large)
	if (fps > 1000)
		fps = _byteswap_ulong(fps);
	if (keys > 100000)
		keys = _byteswap_ulong(keys);

	if (keys > 0 && fps > 0)
		m_endTime = static_cast<double>(keys) / static_cast<double>(fps);
	else
		m_endTime = 0.0;

	m_boneToTrack.resize(model.bones.size());
	for (auto& it : m_boneToTrack)
	{
		it = ModelBone::c_Invalid;
	}

	bool result = false;

	for (size_t j = 0; j < header->NumFrames; ++j)
	{
		uint64_t offset = sizeof(SDKANIMATION_FILE_HEADER) + frameData[j].DataOffset;
		uint32_t keysFixed = header->NumAnimationKeys;
		if (keysFixed > 100000) keysFixed = _byteswap_ulong(keysFixed);
		uint64_t end = offset + sizeof(SDKANIMATION_DATA) * uint64_t(keysFixed);

		if (end > UINT32_MAX
			|| end > m_animSize)
			throw std::runtime_error("Animation file invalid");

		frameData[j].pAnimationData = reinterpret_cast<SDKANIMATION_DATA*>(m_animData.get() + offset);

		wchar_t frameName[MAX_FRAME_NAME] = {};
		MultiByteToWideChar(CP_UTF8, 0, frameData[j].FrameName, -1, frameName, MAX_FRAME_NAME);

		size_t count = 0;
		for (const auto it : model.bones)
		{
			if (_wcsicmp(frameName, it.name.c_str()) == 0)
			{
#if(_DEBUG)
				wchar_t buffer[128];
				swprintf_s(buffer, _countof(buffer), L"%3zu: %-40ws \n", count, it.name.c_str());
				OutputDebugString(buffer);
#endif
				m_boneToTrack[count] = static_cast<uint32_t>(j);
				result = true;
				break;
			}
			count++;
		}
	}
	// ボーン配列を確保する
	m_animBones = ModelBone::MakeArray(model.bones.size());
	return result;
}

// 更新する
void AnimationSDKMESH::Update(float delta)
{
	
	if (m_finished) return; //  freeze on last frame

	m_animTime += delta;

	if (m_endTime > 0.0 && m_animTime >= m_endTime)
	{
		if (m_loopOnce)
		{
			//  Clamp to last frame and stop
			m_animTime = m_endTime - 0.001;
			m_finished = true;
			m_loopCount++;
		}
		else
		{
			//  Normal loop
			m_animTime -= m_endTime;
			m_loopCount++;
		}
	}
}

_Use_decl_annotations_
void AnimationSDKMESH::Apply(const DirectX::Model& model, size_t nbones, XMMATRIX* boneTransforms) const
{
	assert(m_animData && m_animSize > 0);

	const auto* header = reinterpret_cast<const SDKANIMATION_FILE_HEADER*>(m_animData.get());
	auto* frameData = reinterpret_cast<SDKANIMATION_FRAME_DATA*>(m_animData.get() + header->AnimationDataOffset);

	uint32_t fps = header->AnimationFPS;
	uint32_t keys = header->NumAnimationKeys;

	if (fps > 1000)
		fps = _byteswap_ulong(fps);
	if (keys > 100000)
		keys = _byteswap_ulong(keys);

	auto timeInTicks = static_cast<float>(fps) * static_cast<float>(m_animTime);
	auto tick1 = static_cast<uint32_t>(timeInTicks) % keys;
	auto tick2 = (tick1 + 1 < keys) ? (tick1 + 1) : 0;
	float lerpFactor = (float)timeInTicks - std::floor((float)timeInTicks);

	for (size_t j = 0; j < nbones; ++j)
	{
		if (m_boneToTrack[j] == ModelBone::c_Invalid)
		{
			m_animBones[j] = model.boneMatrices[j]; 
		}

		else
		{
			auto* frame = &frameData[m_boneToTrack[j]];
			const auto* data1 = &frame->pAnimationData[tick1];
			const auto* data2 = &frame->pAnimationData[tick2];

			// 位置の線形補間 (Lerp)
			XMVECTOR position1 = XMLoadFloat3(&data1->Translation);
			XMVECTOR position2 = XMLoadFloat3(&data2->Translation);
			XMVECTOR interpolatedPosition = XMVectorLerp(position1, position2, lerpFactor);

			//  Zero root bone translation to prevent loop snap
			bool isRootBone = false;
			if (j < model.bones.size())
			{
				const auto& boneName = model.bones[j].name;
				if (boneName == L"Root" ||
					boneName == L"Skl_Root" ||
					//boneName == L"root" ||
					boneName == L"ROOT")
				{
					isRootBone = true;
				}
			}
			if (isRootBone)
				interpolatedPosition = XMVectorZero(); //  no translation for root

			// 回転の球面線形補間 (Slerp)
			XMVECTOR q1 = XMLoadFloat4(&data1->Orientation);
			XMVECTOR q2 = XMLoadFloat4(&data2->Orientation);
			XMVECTOR interpolatedRotation = XMQuaternionSlerp(q1, q2, lerpFactor);

			// 修正: クォータニオンの正規化(変形防止)
			interpolatedRotation = XMQuaternionNormalize(interpolatedRotation);

			// スケールの線形補間(Lerp)
			XMVECTOR scale1 = XMLoadFloat3(&data1->Scaling);
			XMVECTOR scale2 = XMLoadFloat3(&data2->Scaling);
			XMVECTOR interpolatedScale = XMVectorLerp(scale1, scale2, lerpFactor);

			// 行列の掛け合わせ順番を修正 (スケール×回転×位置)
			XMMATRIX scale = XMMatrixScalingFromVector(interpolatedScale);
			XMMATRIX rotation = XMMatrixRotationQuaternion(interpolatedRotation);
			XMMATRIX translation = XMMatrixTranslationFromVector(interpolatedPosition);
			m_animBones[j] = scale * rotation * translation;
		}
	}

	// 絶対ボーン変換の適用
	model.CopyAbsoluteBoneTransforms(nbones, m_animBones.get(), boneTransforms);
	// 逆バインドポーズ行列を適用する(変形防止)
	//for (size_t index = 0; index < nbones; ++index)
	//{
		//boneTransforms[index] = XMMatrixMultiply(model.invBindPoseMatrices[index], boneTransforms[index]);
	//}
}

// スキンマトリックスを適用する(逆バインドポーズ行列を適用する(変形防止))
void AnimationSDKMESH::ApplySkinMatrix(const DirectX::Model& model, size_t nbones, DirectX::XMMATRIX* outSkinningBones)
{
	// 逆バインドポーズ行列を適用する(変形防止)
	for (size_t index = 0; index < nbones; ++index)
	{
		outSkinningBones[index] = XMMatrixMultiply(model.invBindPoseMatrices[index], outSkinningBones[index]);
	}
}

void AnimationSDKMESH::Release()
{
	m_animTime = 0.0;
	m_animSize = 0;
	m_loopCount = 0;
	m_finished = false;
	m_loopOnce = false;
	m_animData.reset();
	m_boneToTrack.clear();
	m_animBones.reset();
}

//--------------------------------------------------------------------------------------
// Visual Studio Starter Kit CMO animation
//--------------------------------------------------------------------------------------
namespace
{
#pragma pack(push,1)

    struct Clip
    {
        float StartTime;
        float EndTime;
        uint32_t keys;
    };

    static_assert(sizeof(Clip) == 12, "CMO Mesh structure size incorrect");

    struct Keyframe
    {
        uint32_t BoneIndex;
        float Time;
        DirectX::XMFLOAT4X4 Transform;
    };

    static_assert(sizeof(Keyframe) == 72, "CMO Mesh structure size incorrect");

#pragma pack(pop)
}

AnimationCMO::AnimationCMO() noexcept :
	m_animTime(0.f),
	m_startTime(0.f),
	m_endTime(0.f)
{
}

_Use_decl_annotations_
HRESULT AnimationCMO::Load(const wchar_t* fileName, size_t offset, const wchar_t* clipName)
{
    if (!fileName || !offset)
        return E_INVALIDARG;

    std::ifstream inFile(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (!inFile)
        return E_FAIL;

    std::streampos len = inFile.tellg();
    if (!inFile)
        return E_FAIL;

    if (len > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    inFile.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!inFile)
        return E_FAIL;

    auto remaining = len - static_cast<std::streamoff>(offset);

    if (remaining < sizeof(uint32_t))
        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

    auto dataSize = static_cast<size_t>(remaining);
    std::unique_ptr<uint8_t[]> blob(new (std::nothrow) uint8_t[dataSize]);
    if (!blob)
        return E_OUTOFMEMORY;

    inFile.read(reinterpret_cast<char*>(blob.get()), remaining);
    if (!inFile)
        return E_FAIL;

    inFile.close();

    auto nClips = reinterpret_cast<const uint32_t*>(blob.get());
    size_t usedSize = sizeof(uint32_t);
    if (dataSize < usedSize)
        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

    if (!nClips)
        return E_FAIL;

    for (size_t j = 0; j < *nClips; ++j)
    {
        // Clip name
        auto nName = reinterpret_cast<const uint32_t*>(blob.get() + usedSize);
        usedSize += sizeof(uint32_t);
        if (dataSize < usedSize)
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

        auto name = reinterpret_cast<const wchar_t*>(blob.get() + usedSize);

        usedSize += sizeof(wchar_t) * (*nName);
        if (dataSize < usedSize)
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

        auto clip = reinterpret_cast<const Clip*>(blob.get() + usedSize);
        usedSize += sizeof(Clip);
        if (dataSize < usedSize)
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

        if (!clip->keys)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        auto keys = reinterpret_cast<const Keyframe*>(blob.get() + usedSize);
        usedSize += sizeof(Keyframe) * clip->keys;
        if (dataSize < usedSize)
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

        if (!clipName || _wcsicmp(clipName, name) == 0)
        {
            m_startTime = clip->StartTime;
            m_endTime = clip->EndTime;

            m_keys.resize(clip->keys);
            m_transforms = ModelBone::MakeArray(clip->keys);

            for (size_t k = 0; k < clip->keys; ++k)
            {
                m_keys[k].first = keys[k].BoneIndex;
                m_keys[k].second = keys[k].Time;
                m_transforms[k] = XMLoadFloat4x4(&keys[k].Transform);
            }

            return S_OK;
        }
    }

    return E_FAIL;
}

void AnimationCMO::Bind(const Model& model)
{
    assert(!m_keys.empty());

    m_animBones = ModelBone::MakeArray(model.bones.size());
}

void AnimationCMO::Update(float delta)
{
    m_animTime += delta;
    if (m_animTime > m_endTime)
    {
        m_animTime -= m_endTime;
    }
}

_Use_decl_annotations_
void AnimationCMO::Apply(
    const Model& model,
    size_t nbones,
    XMMATRIX* boneTransforms) const
{
    assert(!m_keys.empty());

    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    if (nbones < model.bones.size())
    {
        throw std::invalid_argument("Bone transforms array is too small");
    }

    if (model.bones.empty())
    {
        throw std::runtime_error("Model is missing bones");
    }

    // Compute local bone transforms
    model.CopyBoneTransformsTo(nbones, m_animBones.get());


    // Apply keyframes
    if (m_animTime >= m_startTime)
    {
        size_t k = 0;
        for (auto kit : m_keys)
        {
            if (kit.second > m_animTime)
            {
                break;
            }

            m_animBones[kit.first] = m_transforms[k];
            ++k;
        }
    }

    // Compute absolute locations
    model.CopyAbsoluteBoneTransforms(nbones, m_animBones.get(), boneTransforms);

    // Adjust for model's bind pose.
    for (size_t j = 0; j < nbones; ++j)
    {
        boneTransforms[j] = XMMatrixMultiply(model.invBindPoseMatrices[j], boneTransforms[j]);
    }
}
