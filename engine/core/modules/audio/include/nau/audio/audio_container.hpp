// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once 

#include "audio_asset.hpp"
#include "audio_source.hpp"

#include "nau/utils/enum/enum_reflection.h"

NAU_AUDIO_BEGIN

// ** AudioAssetContainer

NAU_DEFINE_ENUM_(AudioContainerKind, Sequence, Random, Shuffle);
// TODO: Add mix

/**
 * @brief Manages multiple audio assets.
 */
class NAU_AUDIO_EXPORT AudioAssetContainer : public IAudioAsset
{
public:

	/**
	 * @brief Initialization constructor.
	 * 
	 * @param [in] Container name.
	 */
	AudioAssetContainer(const eastl::string& name);

	/**
	 * @brief Destructor.
	 */
	~AudioAssetContainer();

	// From IAudioAsset

	/**
	 * @brief Creates another instance of the container depending on the container type.
	 * 
	 * New instance contents depend on the container kind.
	 * - If the container type is AudioContainerKind::Sequence, the underlying audio assets will be simply copied.
	 * - If the container type is AudioContainerKind::Random, a single, randomly picked asset will be copied.
	 * - If the container type is AudioContainerKind::Shuffle, the underlying audio assets will be copied but their order will be randomly changed.
	 */
	AudioSourcePtr instantiate() override;

	/**
	 * @brief Retrieves the audio asset container name.
	 * 
	 * @return Name of the audio asset container.
	 */
	eastl::string name() const override;

	/**
	 * @brief Retrieves an iterator to the first managed audio asset.
	 * 
	 * @return An iterator to the first managed audio asset.
	 */
	AudioAssetList::iterator begin();

	/**
	 * @brief Retrieves an iterator to a virtual element right after the last managed audio asset.
	 * 
	 * @return An iterator to a virtual element right after the last managed audio asset.
	 */
	AudioAssetList::iterator end();

	/**
	 * @brief Retrieves the audio asset container kind.
	 * 
	 * @return Container type.
	 * 
	 * See @ref instantiate for more details.
	 */
	AudioContainerKind kind() const;

	/**
	 * @brief Changes the audio asset container type.
	 * 
	 * @param [in] kind Value to assign.
	 */
	void setKind(AudioContainerKind kind);

	/**
	 * @brief Adds the audio asset to the container.
	 * 
	 * @param [in] asset A pointer to the asset to add.
	 */
	void add(AudioAssetPtr asset);

	/**
	 * @brief Removes the asset from the container.
	 */
	void remove(AudioAssetPtr asset);
	
	/**
	 * @brief Checks whether the container is empty.
	 * 
	 * @return `true` if the container is empty, `false` otherwise.
	 */
	bool empty() const;

private:
	class Impl;
	std::unique_ptr<Impl> m_pimpl;
};

using AudioAssetContainerPtr = std::shared_ptr<AudioAssetContainer>;
using AudioAssetContainerList = eastl::vector<AudioAssetContainerPtr>;


// ** AudioContainer

/**
 * @brief Manages a queue of audio source to be played.
 * 
 * Note that calling methods on an empty container is safe, but won't result in anything.
 */
class AudioContainer : public IAudioSource
{
public:
	AudioContainer() = default;

	// From IAudioSource

	/**
	 * @brief Resumes the audio playback.
	 */
	void play() override;

	/**
	 * @brief Stops the audio playback.
	 *
	 * @note Currently @ref stop and @ref pause methods are similar. Calling @ref stop does not result in playback rewinding.
	 *		 To rewind the playback use @ref rewind function.
	 */
	void stop() override;

	/**
	 * @brief Pauses the audio playback.
	 *
	 * @note Currently @ref stop and @ref pause methods are similar.
	 */
	void pause() override;

	/**
	 * @brief Changes the current audio source playback position.
	 *
	 * @param [in] ms Position in milliseconds to change the current playback position to.
	 */
	void seek(std::chrono::milliseconds ms) override;

	/**
	 * @brief Retrieves the total duration of audio sources in the container.
	 *
	 * @return Total audio source duration in milliseconds.
	 */
	std::chrono::milliseconds duration() const override;

	/**
	 * @brief Retrieves the playback position in the currently played audio source.
	 *
	 * @return Playback position in the currently played audio source in milliseconds.
	 */
	std::chrono::milliseconds position() const override;

	/**
	 * @brief Checks whether the playback in the currently played audio source has reached its end.
	 *
	 * @return `true` if the playback has reached its end, `false` otherwise.
	 */
	bool isAtEnd() const override;

	/**
	 * @brief Checks whether an audio source from the container is currently being played.
	 *
	 * @return `true` if an audio source from the container is currently being played, `false` otherwise.
	 */
	bool isPlaying() const override;

	/**
	 * @brief Sets a callback to be dispatched when the playback of the last audio source in the queue reaches its end.
	 *
	 * @param [in] callback Callback to use.
	 */
	void setEndCallback(SoundCompletionCallback callback) override;

	/**
	 * @brief Adds the audio source to the container.
	 * 
	 * @param [in] source A pointer to the audio source to add.
	 */
	void addSource(AudioSourcePtr source);

private:
	// TODO: pimpl
	AudioSourceList  m_sources;
	AudioSourcePtr   m_current;
};

using AudioContainerPtr = std::shared_ptr<AudioContainer>;

NAU_AUDIO_END
