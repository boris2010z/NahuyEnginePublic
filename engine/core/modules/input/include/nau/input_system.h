// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <stdint.h>

#include <EASTL/shared_ptr.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/io/fs_path.h"
#include "nau/math/math.h"
#include "nau/rtti/type_info.h"
#include "nau/utils/functor.h"

#pragma once
namespace nau
{
    /**
     * @brief Provides an interface for platform-dependent access to the input source
     */
    struct InputSource
    {
        /**
         * @brief Platform-dependent handle
         */
        uintptr_t m_handle;
        /**
         * @brief Source name
         */
        eastl::string m_name;
    };

    /**
    @brief Manages input sources
    */
    class IInputSourceManager
    {
        NAU_TYPEID(nau::IInputSourceManager)

    public:
        /**
         * @brief Set callback which provide sources to input system
         *
         * @param [in] Callback with ref to sources vector
         */
        virtual void setGetSources(nau::Functor<void(eastl::vector<eastl::shared_ptr<InputSource>>& sources)>) = 0;
    };

    /**
    @brief Provides an interface for input serialization to Datablock.
    */
    class IInputSerializable
    {
    public:
        /**
         * @brief Serializes the object into the DataBlock.
         * 
         * @param [out] Output datablock.
         */
        virtual void serialize(DataBlock* blk) const = 0;

        /**
         * @brief Deserializes the object from the DataBlock.
         * 
         * @param [in] blk  A pointer to the DataBlock to load the data from.
         * @return          `true` on success, `false` otherwise.
         */
        virtual bool deserialize(const DataBlock* blk) = 0;
    };

    /**
     * @brief Provides an interface for platform-dependent access to the input device.
     */
    class IInputDevice
    {
    public:

        /**
         * @brief Determines possible types of input devices.
         */
        enum Type
        {
            Unsupported,    /** < Indicates that the input device has not been recognized and is not supported. */
            Keyboard,       
            Mouse,          
            Joystick,       /** < Indicates that the input device has been recognized as a joystick or a gamepad. */
            Touch,          /** < Indicates that the input device has been recognized as a touchpad. */
            Other           /** < Indicates that the input device is supported and yet has not been recognized as any other supported type of input device. */
        };

        /**
         * @brief Determines possible states of a key.
         */
        enum KeyState
        {
            Released,
            Pressed
        };

        /**
         * @brief Retrieves the name of the input device.
         * 
         * @return Name of the input device.
         */
        virtual eastl::string getName() const = 0;

        /**
         * @brief Retrieves the type of the input device.
         * 
         * @return Type of the input device.
         */
        virtual Type getType() const = 0;

        /**
         * @brief Retrieves the number of keys that the input device supports.
         * 
         * @return Number of supported keys.
         */
        virtual unsigned getKeysNum() const = 0;

        /**
         * @brief Retrieves the number of axes the input device supports.
         * 
         * @return Number of supported axes.
         */
        virtual unsigned getAxisNum() const = 0;

        /**
         * @brief Retrieves axis name by its index.
         * 
         * @param [in] keyId    Index of the key.
         * @return              Name of the key.
         */
        virtual eastl::string getKeyName(unsigned keyId) const = 0;

        /**
         * @brief Retrieves the axis name by its index.
         *
         * @param [in] axisId   Index of the axis.
         * @return              Name of the axis.
         */
        virtual eastl::string getAxisName(unsigned axisId) const = 0;

        /**
         * @brief Retrieves the key index by its name.
         * 
         * @param [in] keyName  Name of the key.
         * @return              Index of the key.
         */
        virtual unsigned getKeyByName(const eastl::string_view keyName) const = 0;

        /**
         * @brief Retrieves the axis index by its name.
         *
         * @param [in] axisName Name of the axis.
         * @return              Index of the axis.
         */
        virtual unsigned getAxisByName(const eastl::string_view axisName) const = 0;

        /**
         * @brief Retrieves the state of the key.
         *
         * @param [in] keyId    Index of the key.
         * @return              State of the key.
         */
        virtual KeyState getKeyState(unsigned keyId) const = 0;

        /**
         * @brief Retrieves the state of the axis.
         *
         * @param [in] axisId   Index of the axis.
         * @return              State of the axis.
         */
        virtual float getAxisState(unsigned axisId) const = 0;
    };

    /**
     * @brief Provides a proxy interface for platform-independent access to the input device.
     */
    class IInputController
    {
    public:

        /**
         * @brief Retrieves the name of the actual input device.
         *
         * @return Name of the input device.
         */
        virtual eastl::string getName() const = 0;

        /**
         * @brief Retrieves a pointer to the actual input device.
         *
         * @return A pointer to the actual input device.
         */
        virtual IInputDevice* getDevice() = 0;

        /**
         * @brief Called on frame update.
         * 
         * @param [in] dt Delta time.
         */
        virtual void update(float dt) = 0;
    };

    template <typename T>
    concept InputSignalSupportedType = eastl::is_same_v<T, float> || eastl::is_same_v<T, int> || eastl::is_same_v<T, unsigned int> || eastl::is_same_v<T, char> || eastl::is_same_v<T, eastl::string>;

    /**
     * @brief Encapsulates input signal generic properties.
     */
    class InputSignalProperties
    {
        NAU_CLASS_FIELDS(
            CLASS_FIELD(properties))

    public:

        /**
         * @brief Assigns value to the signal property.
         * 
         * @tparam T Type of the signal property value. It has to be one of the following: `float`, `int`, `unsigned int`, `char`, `eastl::string`.
         * 
         * @param [in] key      Key under which the property was registered. If the property has not yet been added, use add method.
         * @param [in] value    Value to assign.
         */
        template <typename T>
        void set(const eastl::string& key, const T& value)
        {
            if constexpr (InputSignalSupportedType<T>)
            {
                if (m_properties.count(key) != 0)
                {
                    m_properties[key] = makeValueCopy(value);
                    m_changed = true;
                }
                else
                {
                    NAU_ASSERT(false, "InputSignalProperties: does not exist ({})", key.c_str());
                }
            }
            else
            {
                NAU_ASSERT(false, "InputSignalProperties: unsupported type");
            }
        }

        /**
         * @brief Retrieves the signal property value.
         * 
         * @param [in] key  Key of the property to retrieve the value of.
         * @return          A Result<T> object storing the requested value.
         */
        template <typename T>
        Result<T> get(const eastl::string& key) const
        {
            const auto value = m_properties.at(key);

            if constexpr (InputSignalSupportedType<T>)
            {
                return *runtimeValueCast<T>(value);
            }
            else
            {
                NAU_ASSERT(false, "InputSignalProperties: unsupported type");
            }
        }

        /**
         * @brief Checks whether signal properties have been modified since last update of the signal state.
         * 
         * @param [in] setTo    Optional value to assign to `m_changed`.
         * 
         * @return              `true` if the properties have been changed, `false` otherwise. The return value is adequate at the moment of the function call.
         */
        bool isChanged(bool setTo = false)
        {
            if (m_changed)
            {
                m_changed = setTo;
                return true;
            }
            return false;
        }

    private:
        friend class InputSignalImpl;

        /**
         * @brief Registers the signal property.
         * 
         * @tparam T Type of the signal property value. It has to be one of the following: `float`, `int`, `unsigned int`, `char`, `eastl::string`.
         * 
         * @param [in] key      Key to register the property under.
         * @param [in] value    Initial value of the property.
         */
        template <typename T>
        void add(const eastl::string& key, const T& value)
        {
            if constexpr (InputSignalSupportedType<T>)
            {
                m_properties[key] = makeValueCopy(value);
            }
            else
            {
                NAU_ASSERT(false, "InputSignalProperties: unsupported type");
            }
        }

        /**
         * @brief Stores signal properties and provides access to their values by their names.
         */
        eastl::unordered_map<eastl::string, RuntimeValue::Ptr> m_properties;

        /**
         * @brief Indicates whether signal properties have been change and the signal state require update.
         */
        bool m_changed = false;
    };

    /**
     * @brief Provides input signal interface allowing to access signal state, values and nested inout signals.
     * 
     * Supported signal values are: float, float2, float3 and float4.
     * 
     * Curently supported signal types are:
     * - 'pressed' signals get promoted to 'high' state on key press;
     * - 'released' signals get promoted to 'high' state on key release;
     * - 'move' signals get promoted to 'high' state on axis value change; absolute axis value can be retrieved;
     * - 'move_relative' signals get promoted to 'high' state on axis value change; axis offset from its value in the previous frame can be retrieved;
     * - 'or' signals get promoted to 'high' state as soon as any of the nested signals get promoted;
     * - 'and' signals get promoted to 'high' state as soon as each nested signal gets promoted;
     * - 'not' signals get promoted to 'high' state as soon as the nested state gets demoted to 'low' state;
     * - 'key_axis' signals get promoted to 'high' state on key press or release; the key state is converted to an axis value and can be retrieved;
     * - 'delay' signals get promoted to 'high' as soon as the key has been pressed for a time period;
     * - 'multiple' signals get promoted as soon as the key has been pressed a number of times;
     * - 'scale' signal scales its nested signals values by a factor defined through its properties;
     * - 'dead_zone' signal nulls its nested signals values in case they are inside a square area defined through its properties;
     * - 'clamp' signal clamps its nested signals values between two boundaries that are defined through its properties.
     */
    class IInputSignal : public IInputSerializable
    {
    public:

        /**
         * @brief Determines possible signal states.
         */
        enum State
        {
            Low,
            High
        };

        /**
         * @brief Retrieves the signal name.
         * 
         * @return Name of the signal.
         */
        virtual const eastl::string& getName() const = 0;

        /**
         * @brief Retrieves the signal type.
         * 
         * @return Type of the signal as a string.
         * 
         * For more information on signal types, see IInputSignal.
         */
        virtual const eastl::string& getType() const = 0;

        /**
         * @brief Retrieves a pointer to the input controller, which this signal is attached to.
         * 
         * @return A pointer to the input controller.
         */
        virtual IInputController* getController() = 0;

        /**
         * @brief Attaches the signal to the controller.
         *
         * @pram [in] controller A pointer to the input controller to attach the signal to.
         */
        virtual void setController(IInputController*) = 0;

        /**
         * @brief Retrieves the current state of the signal.
         * 
         * @return Signal current state.
         */
        virtual State getState() const = 0;

        /**
         * @brief Retrieves the previous state of the signal.
         *
         * @return Signal state in the previous frame.
         */
        virtual State getPreviousState() const = 0;

        /**
         * @brief Retrieves the signal float value.
         * 
         * @return Signal value.
         */
        virtual float getValue() const = 0;

        /**
         * @brief Retrieves the signal vector2 value.
         *
         * @return Signal value.
         */
        virtual math::vec2 getVector2() const = 0;

        /**
         * @brief Retrieves the signal vector3 value.
         *
         * @return Signal value.
         */
        virtual math::vec3 getVector3() const = 0;

        /**
         * @brief Retrieves the signal vector4 value.
         *
         * @return Signal value.
         */
        virtual math::vec4 getVector4() const = 0;

        /**
         * @brief Adds a nested signal as input.
         */
        virtual void addInput(IInputSignal* source) = 0;

        /**
         * @brief Retrieves a nested input signal.
         * 
         * @param [in] idx  Index of the nested signal to retrieve.
         * @return          A pointer to the nested signal.
         */
        virtual IInputSignal* getInput(unsigned idx) = 0;

        /**
         * @brief Retrieves the maximal number of nested input signals.
         * 
         * @return  Maximal number of nested input signals.
         * 
         * The output depends on the signal type.
         */
        virtual unsigned maxInputs() const = 0;

        /**
         * @brief Provides access to the signal properties.
         * 
         * @return A reference to the InputSignalProperties object storing properties of the signal.
         */
        virtual InputSignalProperties& Properties() = 0;

        /**
         * @brief Retrieves the signal properties.
         * 
         * @return A const reference to the InputSignalProperties object storing properties of the signal.
         */
        virtual const InputSignalProperties& Properties() const = 0;

        /**
         * @brief Called each frame.
         * 
         * @param [in] dt Delta time.
         * 
         * The method can be used to modify values, nested signals or state of the signal.
         */
        virtual void update(float dt) = 0;
    };

    /**
     * @brief Provides input action interface which can exhibit user-defined behavior depending on the state change of the associated signal with context support.
     */
    class IInputAction : public IInputSerializable
    {
    public:

        /**
         * @brief Determines the action response to the associated signal state change
         * 
         * See IInputSignal for more information.
         */
        enum Type
        {
            /**
             * @brief Actions of this type are triggered as soon as the associated signal transitions from 'low' to 'high' state.
             * 
             * Such actions may be used, for example, to react to a button press/release.
             */
            Trigger,

            /**
             * @brief Actions of this type are triggered each frame the associated signal is in 'high' state.
             * 
             * Such actions may be used, for example, to react to a button hold or a cursor move.
             */
            Continuous
        };

        /**
         * @brief Retrieves the name of the action.
         * 
         * @return Name of the action.
         */
        virtual eastl::string getName() const = 0;

        /**
         * @brief Retrieves the type of the action.
         *
         * @return Type of the action.
         */
        virtual Type getType() const = 0;

        /**
         * @brief Retrives the signal associated with this action.
         * 
         * @return A pointer to the associated signal.
         */
        virtual IInputSignal* getSignal() = 0;

        /**
         * @brief Attaches string tag to the action.
         * 
         * @param [in] tag Tag to attach to the action.
         * 
         * Multiple tags can be attached to a single action.
         */
        virtual void addContextTag(const eastl::string& tag) = 0;

        /**
         * @brief Detaches the tag from the action.
         * 
         * @param [in] tag Tag to detach from the action.
         */
        virtual void removeContextTag(const eastl::string& tag) = 0;

        /**
         * @brief Checks if the tag is attached to the action.
         * 
         * @param [in] tag  Tag to inquire about.
         * @return          `true` if the tag is attached to the action, `false` otherwise.
         */
        virtual bool isContextTag(const eastl::string& tag) const = 0;

        /**
         * @brief Updates the associated signal and exhibits the user-defined behavior if necessary. 
         *
         * The function is run on per-frame basis.
         */
        virtual void update(float dt) = 0;
    };

    /**
     * @brief Provides a general interface for managing input signals, actions and contexts.
     */
    class IInputSystem
    {
        NAU_TYPEID(nau::IInputSystem)

    public:
     
        /**
         * @brief Creates an input action and registers it in the system.
         * 
         * @param [in] name             Name of the action.
         * @param [in] type             Type of the action.
         * @param [in] signal           A pointer to the signal that is to be associated with the new action.
         * @param [in] actionCallback   A functor that is to be called upon action trigger.
         * @return                      A pointer to the created action.
         */
        virtual eastl::shared_ptr<IInputAction> addAction(const eastl::string& name, IInputAction::Type type, IInputSignal* signal, nau::Functor<void(IInputSignal*)> actionCallback) = 0;

        /**
         * @brief Creates an input action and registers it in the system.
         *
         * @param [in] serialized       A string view of the DataBlock to create the action from.
         * @param [in] actionCallback   A functor that is to be called upon action trigger.
         * @return                      A pointer to the created action.
         */
        virtual eastl::shared_ptr<IInputAction> addAction(const eastl::string& serialized, nau::Functor<void(IInputSignal*)> actionCallback) = 0;

        /**
         * @brief Removes the action from the system.
         * 
         * @param [in] action   A reference to the action to remove.
         * @return              `true` on success, `false` otherwise.
         */
        virtual bool removeAction(eastl::shared_ptr<IInputAction>&& action) = 0;

        /**
         * @brief Creates an input action from the .blk file and registers it in the system.
         *
         * @param [in] filePath         Path to the source .blk file.
         * @param [in] actionCallback   A functor that is to be called upon action trigger.
         * @return                      A pointer to the created action.
         */
        virtual eastl::shared_ptr<IInputAction> loadAction(const io::FsPath& filePath, nau::Functor<void(IInputSignal*)> actionCallback) = 0;

        /**
         * @brief Outputs the action (as DataBlock) into the .blk file.
         * 
         * @param [in] action   A pointer to the action to save.
         * @param [in] filePath Path to the .blk file to ouput the action to.
         * @return              `true` on success, `false` otherwise.
         */
        virtual bool saveAction(const eastl::shared_ptr<IInputAction> action, const eastl::string& filePath) = 0;

        /**
         * @brief Retrieves actions that have been registered in the system.
         * 
         * @param [out] actions Output actions vector.
         */
        virtual void getActions(eastl::vector<eastl::shared_ptr<IInputAction>>& actions) = 0;

        /**
         * @brief Creates a signal.
         * 
         * @param [in] signalType   String type of the signal. For list of supported types see IInputSignal.
         * @return                  A pointer to the created signal.
         */
        virtual IInputSignal* createSignal(const eastl::string& signalType) = 0;

        /**
         * @brief Creates a signal.
         *
         * @param [in] signalType       String type of the signal. For list of supported types see IInputSignal.
         * @param [in] controllerName   Name of the controller to associate this signal with.
         * @param [in] signal           Functor that is to be called at signal creation. You may use this functor to set properties, values or nested input signals of the created signal.
         * @return                      A pointer to the created signal.
        */
        virtual IInputSignal* createSignal(const eastl::string& signalType, const eastl::string& controllerName, nau::Functor<void(IInputSignal*)> signal) = 0;

        /**
         * @brief Resets the system active context list to a single active context.
         * 
         * @param [in] context  Name of the context to add.
         * 
         * To add an active context and keep existing active contexts use addContext method.
         */
        virtual void setContext(const eastl::string& context) = 0;

        /**
         * @brief Adds the context to the system active context list.
         * 
         * @param [in] context  Name of the context to add.
         */
        virtual void addContext(const eastl::string& context) = 0;

        /**
         * @brief Removes the context from the system active context list.
         * 
         * @param [in] context  Name of the context to remove.
         * 
         * Actions from the removed contexts are not triggered.
         */
        virtual void removeContext(const eastl::string& context) = 0;

        /**
         * @brief Retrieves all connected input devices.
         * 
         * @return A vector of connected input devices.
         */
        virtual eastl::vector<IInputDevice*> getDevices() = 0;

        /**
         * @brief Retrieves the input controller (i.e. platform-independent input device abstraction) that matches the description.
         * 
         * @param [in] controllerDesc   Definition of the requested controller registered in the system.
         * @return                      A pointer to the matching controller or `NULL` if none has been found.
         */
        virtual IInputController* getController(const eastl::string& controllerDesc) = 0;

        /**
         * @brief Set current input source 
         *
         * @param [in] source  Name of the source
         * 
         * Source valid until next setInputSource call
         */
        virtual void setInputSource(const eastl::string& source) = 0;
    };

}  // namespace nau
