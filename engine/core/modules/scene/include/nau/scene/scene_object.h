// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <EASTL/intrusive_list.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

#include <concepts>

#include "nau/async/task_base.h"
#include "nau/memory/eastl_aliases.h"
#include "nau/messaging/async_message_stream.h"
#include "nau/scene/components/scene_component.h"
#include "nau/scene/nau_object.h"
#include "nau/scene/transform_control.h"
#include "nau/utils/typed_flag.h"

namespace nau::scene
{
    struct IScene;

    /**
     * @brief Represents various options applied when object's parent is changed.
     */
    enum class SetParentOpts
    {
        /**
         * @brief  Indicates that the object will be moved so that its local transform is unmodified.
         *         Otherwise, it will remain its position in the world space and its local coordinates will be adjusted.
         */
        DontKeepWorldTransform = NauFlag(1)
    };

    NAU_DEFINE_TYPED_FLAG(SetParentOpts)

    /**
        @brief An object that can be placed at a scene
     */
    class NAU_CORESCENE_EXPORT SceneObject : public NauObject,
                                             public TransformControl,
                                             public eastl::intrusive_list_node

    {
        NAU_OBJECT(nau::scene::SceneObject, NauObject, TransformControl)

    public:
        using Ptr = ObjectUniquePtr<SceneObject>;
        using WeakRef = ObjectWeakRef<SceneObject>;

        using WalkObjectsCallback = bool (*)(SceneObject& object, void* callbackData);
        using WalkComponentsCallback = bool (*)(Component& component, void* callbackData);
        using WalkConstComponentsCallback = bool (*)(const Component& component, void* callbackData);

        /**
         * @brief Default constructor (deleted).
         */
        SceneObject() = delete;

        /**
         * @brief Initialization constructor.
         *
         * @param [in] rootComponent    Object root component. See m_rootComponent.
         */
        SceneObject(ObjectUniquePtr<SceneComponent>&& rootComponent);

        /**
         * @brief Copy constructor (deleted).
         */
        SceneObject(const SceneObject&) = delete;

        /**
         * @brief Destructor.
         */
        ~SceneObject();

        /**
         * @brief Destroys object's children, components and then the object itself.
         *
         * If the object is not attached to the scene (i.e. it is inactive), calling this will remove all weak references
         * to the object as well ass to its children and components. Given that the object is owned by a ObjectUniquePtr
         * instance, the smart pointer is responsible for its destruction. Otherwise the object is destructed immediately.\n
         * If the object is attached to the scene all weak references to self, object's children and components are removed as well.
         * However, no sooner all async operations over the object's components are finished than the actual destruction occurs.
         */
        void destroy() final;

        /**
         * @brief Retrieves the object name.
         *
         * @return Name of the scene object.
         */
        eastl::string_view getName() const;

        /**
         * @brief Assigns name to the scene object.
         *
         * @param [in] name name to assign.
         */
        void setName(eastl::string_view name);

        /**
         * @brief Retrives the scene which the object is attached to.
         *
         * @return Scene which the object is attached to.
         */
        IScene* getScene() const;

        /**
         * @brief Retrieves the object activation state.
         *
         * @return Activation state of the object.
         */
        ActivationState getActivationState() const;

        SceneObject::Ptr clone();

        /**
         *   @brief Retrieves the parent of the object.
         *
         *   @return Parent object or `NULL` if the object does not have a parent.
         */
        SceneObject* getParentObject() const;

        /**
         * @brief Provides mutable access to the root component of the object.
         *
         * @tparam ComponentType Type of the root component. It has to be derived from SceneComponent.
         *
         * @return Object root component. See m_rootComponent.
         */
        template <std::derived_from<Component> ComponentType = SceneComponent>
        ComponentType& getRootComponent();

        /**
         * @brief Provides const access to the root component of the object.
         *
         * @tparam ComponentType Type of the root component. It has to be derived from SceneComponent.
         *
         * @return Object root component. See m_rootComponent.
         */
        template <std::derived_from<Component> ComponentType = SceneComponent>
        const ComponentType& getRootComponent() const;

        /**
         * @brief Attaches an object as a child.
         *
         * @param [in] childObject  A pointer to the object to attach.
         * @return                  Child object.
         *
         * @note This is a syncronous operation. For async analog see attachChildAsync.
         *
         * As a result of the call ownership of the attached object is transferred to the parent.
         */
        SceneObject& attachChild(SceneObject::Ptr&& childObject);

        /**
         * @brief Asynchronously attaches an object as a child.
         *
         * @param [in] childObject  A pointer to the object to attach.
         * @return                  Task object providing operation status.
         *
         * @note For a synchronous analog see attachChild.
         *
         * As a result of the operation ownership of the attached object is transferred to the parent.
         * Also, given that the parent is active, the child get activated as well.
         */
        async::Task<ObjectWeakRef<SceneObject>> attachChildAsync(SceneObject::Ptr&& childObject);

        /**
         * @brief Destroys the specified child object.
         *
         * @param [in] childRef Object to detach.
         *
         * The child object will actually be deleted only after all its component have been deactivated
         * and all async operations over it have been finished (see removeComponent for more details).
         */
        void removeChild(ObjectWeakRef<SceneObject> childRef);

        /**
         * @brief Assigns a parent to the scene object.
         *
         * @param [in] newParent    Parent object to assing.
         * @param [in] options      Options to apply during the operation.
         *
         * @note    You can assign a parent to the object only provided that their activation states are identical.
         * @note    By default the system will save the world transform and adjjust the local transform relatively to the new parent.
                    If this is not desired, pass DontKeepWorldTransform flag for options.
         *
         * The method does not trigger any events related to the activation and deactivation of the component
         * nor it initializes any resources. The change of parent only happens within the scene hierarchy.
         */
        void setParent(SceneObject& newParent, SetParentOptsFlag options = {});

        /**
         * @brief Iterates through the child objects and applies a callback.
         *
         * @param [in] callback         Callable object to apply to each iterated child object.
         * @param [in] callbackData     Callback input data.
         * @param [in] walkRecursive    If `false` is passed, the function iterates only through first-order descendants (i.e. children, not "grandchildren", "great-grandchildren" etc.). Otherwise, the function iterates through all children.
         *
         * @note **callback** has to return boolean which indicates whether iteration should proceed,
         * i.e. if applying it to a child object returned `false`, the iteration stops.
         */
        void walkChildObjects(WalkObjectsCallback callback, void* callbackData, bool walkRecursive);

        /**
         * @brief Retrieves object's children.
         *
         * @param [in] recursive    If `false` is passed, the function retrieves only first-order descendants (i.e. children, not "grandchildren", "great-grandchildren" etc.). Otherwise, the function retrieves all children.
         * @return                  A collection of object's children.
         */
        Vector<SceneObject*> getChildObjects(bool recursive);

        /**
         * @brief Retrieves object's first order descendants (i.e. children, not "grandchildren", "great-grandchildren" etc.).
         *
         * @return A collection of object's children.
         *
         * To retrieve higher-order descendants (grandchildren", "great-grandchildren" etc.) use getAllChildObjects.
         */
        Vector<SceneObject*> getDirectChildObjects();

        /**
         * @brief Retrieves all object's children.
         *
         * @return A collection of object's children.
         *
         * To retrieve only first-order (direct) descendants use getDirectChildObjects.
         */
        Vector<SceneObject*> getAllChildObjects();

        /**
         * @brief Attaches the component to the object.
         *
         * @param [in] type Component type information.
         * @return          Attached component.
         *
         * This is synchronous operation. See also: addComponentAsync(const rtti::TypeInfo&)
         */
        Component& addComponent(const rtti::TypeInfo& type, Functor<void(Component&)> initializer = nullptr);

        /**
         * @brief Attaches the component to the object.
         *
         * @tparam ComponentType Type of the component to attach. It has to be derived from Component.
         *
         * @return          Attached component.
         *
         * This is synchronous operation. See also: addComponentAsync().
         */
        template <std::derived_from<Component> ComponentType>
        ComponentType& addComponent(Functor<void(ComponentType&)> initializer = nullptr);

        /**
         * @brief Attaches the component to the object.
         *
         * @param [in] type Component type information.
         * @return          Attached component.
         *
         * This is asynchronous operation. See also: getComponent(const rtti::TypeInfo&).
         */
        async::Task<ObjectWeakRef<Component>> addComponentAsync(const rtti::TypeInfo& type, Functor<void(Component&)> initializer = nullptr);

        /**
         * @brief Attaches the component to the object.
         *
         * @tparam ComponentType Type of the component to attach. It has to be derived from Component.
         *
         * @return          Attached component.
         *
         * This is asynchronous operation. See also: addComponent().
         */
        template <std::derived_from<Component> ComponentType>
        async::Task<ObjectWeakRef<ComponentType>> addComponentAsync(Functor<void(ComponentType&)> initializer = nullptr);

        /**
         * @brief Removes the specified components from the scene object.
         *
         * @param [in] componentRef Component to remove.
         *
         * Internally component removal is asynchronous operation with multiple phases.
         *  1.
         *      - A component can actually be removed from scene only from outside of the current update call.
         *  2.
         *      - At the post-update stage IComponentActivation::deactivateComponent or IComponentAsyncActivation::deactivateComponentAsync
         *      will be called. Also IComponentsActivator/IComponentsAsyncActivator starts processing component's deactivation.
         *      - The deactivation phase runs without blocking the update for some time. During the deactivation phase component is still a part of scene, however it is not updated anymore.
         *  3.
         *      - After the deactivation has been finished the component is detached from scene (i.e. removed from the parent object).
         *      - The system deletes the component instance only after all async operations associated with it has been completed.
         */
        void removeComponent(ObjectWeakRef<Component> componentRef);

        /**
         * @brief Iterates through the components (including its root component) of the object and its children and applies a callback.
         *
         * @param [in] callback         Callable object to apply to each iterated component.
         * @param [in] callbackData     Callback input data.
         * @param [in] walkRecursive    If `false` is passed, the function iterates only through the components that are attached to the object. Otherwise, the function iterates through the components of each child recursively.
         * @param [in] componentType    Type of the components to apply **callback** to.
         *
         * @note **callback** has to return boolean which indicates whether iteration should proceed,
         * i.e. if applying it to a component returned `false`, the iteration stops.
         */
        void walkComponents(WalkComponentsCallback callback, void* callbackData, bool walkRecursive, const rtti::TypeInfo* componentType = nullptr);

        /**
         * @brief Iterates through the components (including its root component) of the object and its children and applies a callback.
         *
         * @param [in] callback         Callable object to apply to each iterated component.
         * @param [in] callbackData     Callback input data.
         * @param [in] walkRecursive    If `false` is passed, the function iterates only through the components that are attached to the object. Otherwise, the function iterates through the components of each child recursively.
         * @param [in] componentType    Type of the components to apply **callback** to. If `NULL` is passed, all components are visited.
         *
         * @note **callback** has to return boolean which indicates whether iteration should proceed,
         * i.e. if applying it to a component returned `false`, the iteration stops.
         */
        void walkComponents(WalkConstComponentsCallback callback, void* callbackData, bool walkRecursive, const rtti::TypeInfo* componentType = nullptr) const;

        /**
         * @brief Retrieves object components.
         *
         * @param [in] recursive        If `false` is passed, the function retrieves only the components that are attached to the object. Otherwise, the function retrieves the components of all children recursively.
         * @param [in] componentType    Type of the components to retrieve. If `NULL` is passed, all components are retrieved.
         * @return                      A collection of object components.
         */
        Vector<Component*> getComponents(bool recursive, const rtti::TypeInfo* componentType = nullptr);

        /**
         * @brief Retrieves object components.
         *
         * @param [in] recursive        If `false` is passed, the function retrieves only the components that are attached to the object. Otherwise, the function retrieves the components of all children recursively.
         * @param [in] componentType    Type of the components to retrieve. If `NULL` is passed, all components are retrieved.
         * @return                      A collection of object components.
         */
        Vector<const Component*> getComponents(bool recursive, const rtti::TypeInfo* componentType = nullptr) const;

        /**
         * @brief Retrieves object components.
         *
         * @tparam ComponentType Type of components to retrieve. It has to be a subclass of Component.
         *
         * @return                      A collection of object components.
         *
         * @note The function only returns components that are attached to the calling object.
         * If you are interested in children's components as well, use getAllComponents.
         */
        template <std::derived_from<Component> ComponentType = Component>
        Vector<Component*> getDirectComponents();

        /**
         * @brief Retrieves object components.
         *
         * @tparam ComponentType Type of components to retrieve. It has to be a subclass of Component.
         *
         * @return                      A collection of object components.
         *
         * @note The function only returns components that are attached to the calling object.
         * If you are interested in children's components as well, use getAllComponents.
         */
        template <std::derived_from<Component> ComponentType = Component>
        Vector<const Component*> getDirectComponents() const;

        /**
         * @brief Retrieves object components.
         *
         * @tparam ComponentType Type of components to retrieve. It has to be a subclass of Component.
         *
         * @return                      A collection of object components.
         *
         * @note The function returns components attached to the calling object as well as to its children.
         * If you only are interested in the former, use getDirectComponents.
         */
        template <std::derived_from<Component> ComponentType = Component>
        Vector<Component*> getAllComponents();

        /**
         * @brief Retrieves object components.
         *
         * @tparam ComponentType Type of components to retrieve. It has to be a subclass of Component.
         *
         * @return                      A collection of object components.
         *
         * @note The function returns components attached to the calling object as well as to its children.
         * If you only are interested in the former, use getDirectComponents.
         */
        template <std::derived_from<Component> ComponentType = Component>
        Vector<const Component*> getAllComponents() const;

        /**
         * @brief Searches for a component of the specified type among the components of the calling object and its children.
         *
         * @param [in] type         Type of the component to retrieve.
         * @param [in] recursive    If `false` is passed, the component is looked for only among the components attached to the calling object. Otherwise, the seach is conducted recursively among its children as well.
         * @return                  A pointer to the retrieved component.
         */
        Component* findFirstComponent(const rtti::TypeInfo& type, bool recursive = false);

        /**
         * @brief Searches for a component of the specified type among the components of the calling object and its children.
         *
         * @tparam ComponentType    Type of the component to retrieve. It has to be a subclass of Component.
         *
         * @param [in] recursive    If `false` is passed, the component is looked for only among the components attached to the calling object. Otherwise, the seach is conducted recursively among its children as well.
         * @return                  A pointer to the retrieved component.
         */
        template <std::derived_from<Component> ComponentType>
        ComponentType* findFirstComponent(bool recursive = false);

        /**
         * @brief Retrieves object world transformation.
         *
         * @return Object transformation in world coordinates.
         */
        const math::Transform& getWorldTransform() const final;

        /**
         * @brief Sets object transform in world coordinates.
         *
         * @param [in] transform Transform to assign.
         */
        void setWorldTransform(const math::Transform& transform) final;

        /**
         * @brief Retrieves object local transformation.
         *
         * @return Object transformation with respect to its parent.
         */
        const math::Transform& getTransform() const final;

        /**
         * @brief Sets object transform relative to its parent.
         *
         * @param [in] transform Transform to assign.
         */
        void setTransform(const math::Transform& transform) final;

        /**
         * @brief Sets object rotation relative to its parent.
         *
         * @param [in] transform Transform to assign.
         */
        void setRotation(math::quat rotation) final;

        /**
         * @brief Sets object translation relative to its parent.
         *
         * @param [in] transform Transform to assign.
         */
        void setTranslation(math::vec3 position) final;

        /**
         * @brief Sets object scale relative to its parent.
         *
         * @param [in] transform Transform to assign.
         */
        void setScale(math::vec3 scale) final;

        /**
         * @brief Retrieves object local rotation.
         *
         * @return Object rotation with respect to its parent.
         */
        math::quat getRotation() const final;

        /**
         * @brief Retrieves object local translation.
         *
         * @return Object translation with respect to its parent.
         */
        math::vec3 getTranslation() const final;

        /**
         * @brief Retrieves object local scale.
         *
         * @return Object scale with respect to its parent.
         */
        math::vec3 getScale() const final;

        AsyncMessageSource& getMessageSource();

    protected:
        const SceneComponent& getRootComponentInternal() const;
        SceneComponent& getRootComponentInternal();

    private:
        void onBeforeDeleteObject() override;
        void setScene(IScene*);
        bool walkChildObjectsRecursive(WalkObjectsCallback callback, void* callbackData);
        bool walkComponentsRecursive(WalkComponentsCallback callback, void* callbackData, const rtti::TypeInfo* componentType);
        async::Task<> activate();
        void clearSceneReferencesRecursive();

        void resetParentInternal(SceneObject* newParent, SetParentOptsFlag options);
        void removeComponentFromList(Component& component);

        SceneObject& attachChildInternal(SceneObject::Ptr&& childObject, bool activateNow);
        Component& addComponentInternal(const rtti::TypeInfo& componentType, Functor<void(Component&)>& initializer, bool activateNow);

        /**
         * @brief A component that is used as 'base'. Local transform of all other components are relative to the root component.
         */
        SceneComponent* m_rootComponent = nullptr;
        SceneObject* m_parent = nullptr;
        IScene* m_scene = nullptr;
        eastl::string m_name;
        eastl::intrusive_list<scene_internal::ComponentListNode> m_components;
        eastl::intrusive_list<SceneObject> m_children;
        nau::Ptr<AsyncMessageSource> m_messageSource;

        ActivationState m_activationState = ActivationState::Inactive;

        friend class SceneImpl;
        friend class SceneManagerImpl;
        friend class Component;
    };

    template <std::derived_from<Component> ComponentType>
    ComponentType& SceneObject::getRootComponent()
    {
        SceneComponent& root = getRootComponentInternal();

        if constexpr (std::is_same_v<ComponentType, SceneComponent>)
        {
            return root;
        }
        else
        {
            return root.as<ComponentType&>();
        }
    }

    template <std::derived_from<Component> ComponentType>
    const ComponentType& SceneObject::getRootComponent() const
    {
        const SceneComponent& root = getRootComponentInternal();

        if constexpr (std::is_same_v<ComponentType, SceneComponent>)
        {
            return root;
        }
        else
        {
            return root.as<const ComponentType&>();
        }
    }

    template <std::derived_from<Component> ComponentType>
    ComponentType& SceneObject::addComponent(Functor<void(ComponentType&)> initializer)
    {
        if (initializer)
        {
            Component& component = addComponent(rtti::getTypeInfo<ComponentType>(), [&initializer](Component& comp) mutable
            {
                initializer(comp.as<ComponentType&>());
            });
            return component.as<ComponentType&>();
        }

        Component& component = addComponent(rtti::getTypeInfo<ComponentType>());
        return component.as<ComponentType&>();
    }

    template <std::derived_from<Component> ComponentType>
    async::Task<ObjectWeakRef<ComponentType>> SceneObject::addComponentAsync(Functor<void(ComponentType&)> initializer)
    {
        if (initializer)
        {
            auto componentRef = co_await addComponentAsync(rtti::getTypeInfo<ComponentType>(), [&initializer](Component& comp)
            {
                initializer(comp.as<ComponentType&>());
            });
            co_return ObjectWeakRef<ComponentType>{componentRef};
        }

        auto componentRef = co_await addComponentAsync(rtti::getTypeInfo<ComponentType>());
        co_return ObjectWeakRef<ComponentType>{componentRef};
    }

    template <std::derived_from<Component> ComponentType>
    Vector<Component*> SceneObject::getDirectComponents()
    {
        if constexpr (std::is_same_v<ComponentType, Component>)
        {
            return getComponents(false, nullptr);
        }
        else
        {
            return getComponents(false, &rtti::getTypeInfo<ComponentType>());
        }
    }

    template <std::derived_from<Component> ComponentType>
    Vector<const Component*> SceneObject::getDirectComponents() const
    {
        if constexpr (std::is_same_v<ComponentType, Component>)
        {
            return getComponents(false, nullptr);
        }
        else
        {
            return getComponents(false, &rtti::getTypeInfo<ComponentType>());
        }
    }

    template <std::derived_from<Component> ComponentType>
    Vector<Component*> SceneObject::getAllComponents()
    {
        if constexpr (std::is_same_v<ComponentType, Component>)
        {
            return getComponents(true, nullptr);
        }
        else
        {
            return getComponents(true, &rtti::getTypeInfo<ComponentType>());
        }
    }

    template <std::derived_from<Component> ComponentType>
    Vector<const Component*> SceneObject::getAllComponents() const
    {
        if constexpr (std::is_same_v<ComponentType, Component>)
        {
            return getComponents(true, nullptr);
        }
        else
        {
            return getComponents(true, &rtti::getTypeInfo<ComponentType>());
        }
    }

    template <std::derived_from<Component> ComponentType>
    ComponentType* SceneObject::findFirstComponent(bool recursive)
    {
        const rtti::TypeInfo& type = rtti::getTypeInfo<ComponentType>();
        Component* const component = findFirstComponent(type, recursive);

        return component ? component->as<ComponentType*>() : nullptr;
    }

}  // namespace nau::scene
