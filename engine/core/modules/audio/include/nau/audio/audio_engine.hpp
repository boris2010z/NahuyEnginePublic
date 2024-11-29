// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "audio_source.hpp"
#include "audio_container.hpp"
#include "audio_asset.hpp"


NAU_AUDIO_BEGIN

// ** IAudioEngine

class NAU_AUDIO_EXPORT IAudioEngine
{
public:

	/**
	 * @brief Enumerates supported audio engine backends.
	 */
	enum Backend {
		Miniaudio = 0,
	};

	/**
	 * @brief Creates the audio engine and initializes the backend.
	 * 
	 * @param [in] Identifier of the backend to initialize.
	 * 
	 * @warning Currently only Miniaudio is supported.
	 */
	static std::unique_ptr<IAudioEngine> create(Backend backend);

	/**
	 * @brief Destructor.
	 */
	virtual ~IAudioEngine() = default;

	/**
	 * @brief Initializes the audio engine.
	 */
	virtual void initialize() = 0;

	/**
	 * @brief Deinitializes the audio engine.
	 */
	virtual void deinitialize() = 0;

	virtual void update() = 0;

	// Asset creation

	/**
	 * @brief Creates an audio asset from the file.
	 * 
	 * @param [in] path Path to the audio file.
	 * @return			A pointer to the created asset.
	 */
	virtual AudioAssetPtr loadSound(const eastl::string& path) = 0;

	/**
	 * @brief Creates an audio asset from the streamed audio file.
	 * 
	 * @param [in] path Path to the audio file to stream.
	 * @return			A pointer to the created asset.
	 * 
	 * @note	When a large audio file (e.g. a music track) is to be loaded, it is more efficent to stream it then to boldly load the entire file.
	 *			When an audio file is streamed, it is loaded in pieces of 2 seconds length. If the audio is short, use standard loadSound function.
	 */
	virtual AudioAssetPtr loadStream(const eastl::string& path) = 0;

	/**
	 * @brief
	 */
	virtual AudioAssetContainerPtr createContainer(const eastl::string& name);

	// Asset access
	virtual AudioAssetList audioAssets() = 0;
	virtual AudioAssetContainerList containerAssets();
	virtual AudioAssetList assets();

private:
	AudioAssetContainerList m_containers;
};

using AudioEnginePtr = std::unique_ptr<IAudioEngine>;

NAU_AUDIO_END
