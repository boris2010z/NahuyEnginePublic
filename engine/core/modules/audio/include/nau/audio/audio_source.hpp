// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once 

#include "audio_common.hpp"

#include <functional>
#include <memory>
#include <chrono>

#include <EASTL/vector.h>


NAU_AUDIO_BEGIN

// ** SoundCompletionCallback

using SoundCompletionCallback = std::function<void()>;


// ** IAudioSource

/**
 * @brief Provides interface for managing audio playback.
 */
class IAudioSource
{
public:
	// Playback

	/**
	 * @brief Resumes the audio playback.
	 */
	virtual void play() = 0;

	/**
	 * @brief Stops the audio playback.
	 * 
	 * @note Currently @ref stop and @ref pause methods are similar. Calling @ref stop does not result in playback rewinding.
	 *		 To rewind the playback use @ref rewind function.
	 */
	virtual void stop() = 0;

	/**
	 * @brief Pauses the audio playback.
	 * 
	 * @note Currently @ref stop and @ref pause methods are similar. 
	 */
	virtual void pause() = 0;

	/**
	 * @brief Changes the current audio source playback position.
	 * 
	 * @param [in] ms Value in milliseconds to change the current playback position to.
	 */
	virtual void seek(std::chrono::milliseconds ms) = 0;

	/**
	 * @brief Resets the audio source playback to the beginning.
	 */
	virtual void rewind();
	
	// State

	/**
	 * @brief Retrieves the audio source playback position.
	 * 
	 * @return Audio source playback current in milliseconds.
	 */
	virtual std::chrono::milliseconds position() const = 0;

	/**
	 * @brief Retrieves the audio source duration.
	 * 
	 * @return Audio source duration in milliseconds.
	 */
	virtual std::chrono::milliseconds duration() const = 0;

	/**
	 * @brief Checks whether the audio source playback has reached its end.
	 * 
	 * @return `true` if the playback has reached its end, `false` otherwise.
	 */
	virtual bool isAtEnd() const = 0;

	/**
	 * @brief Checks whether the audio source is currently being played.
	 * 
	 * @return `true` if the audio source is currently being pplayed, `false` otherwise.
	 */
	virtual bool isPlaying() const = 0;

	// Callbacks

	/**
	 * @brief Sets a callback to be dispatched when the audio source playback reaches its end.
	 * 
	 * @param [in] callback Callback to use.
	 */
	virtual void setEndCallback(SoundCompletionCallback callback) = 0;

	/**
	 * @brief Sets an audio source to be played after this source playback has reached its end.
	 * 
	 * @param [in] next A pointer to the audio source to be played next.
	 */
	virtual void playNext(std::shared_ptr<IAudioSource> next);
};

using AudioSourcePtr = std::shared_ptr<IAudioSource>;
using AudioSourceList = eastl::vector<AudioSourcePtr>;

NAU_AUDIO_END
