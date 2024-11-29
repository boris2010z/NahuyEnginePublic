// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/scene/scene_object.h"

#include "./scene_management/scene_manager_impl.h"
#include "nau/messaging/messaging.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/internal/component_factory.h"
#include "nau/service/service_provider.h"
#include "nau/scene/scene_factory.h"

namespace nau::scene
{
    SceneObject::SceneObject(scene::ObjectUniquePtr<SceneComponent>&& rootComponent) :
        m_rootComponent(rootComponent.giveUp())
    {
        setUid(Uid::generate());

        NAU_FATAL(m_rootComponent);

        m_rootComponent->m_parentObject = this;
        m_components.push_back(*m_rootComponent);
    }

    SceneObject::~SceneObject()
    {
        if (!m_children.empty()) [[likely]]
        {
            // m_children are organized as intrusive list which must be cleared prior objects are actually will be deleted.
            // Objects owning are transferred into temporary 'children' collections which will be cleared after intrusive list.
            StackVector<ObjectUniquePtr<SceneObject>> children;
            children.reserve(m_children.size());
            for (SceneObject& child : m_children)
            {
                auto& objectPtr = children.emplace_back(&child);

                // need to clear parent to prevent reseting it from SceneObject::onBeforeDeleteObject
                objectPtr->m_parent = nullptr;
            }

            // clearing children intrusive list (prior temporary children collection)
            m_children.clear();

            // actually delete children objects: will also trigger child objects deletion through ObjectUniquePtr destructor
            children.clear();
        }

        m_rootComponent = nullptr;
        {
            // m_components are organized as intrusive list which must be cleared components are actually will be deleted.
            // Components owning are transferred into temporary 'components' collections which will be cleared after intrusive list.
            StackVector<ObjectUniquePtr<Component>> components;
            components.reserve(m_components.size());
            for (scene_internal::ComponentListNode& componentNode : m_components)
            {
                auto& componentPtr = components.emplace_back(&static_cast<Component&>(componentNode));

                // need to clear parent to prevent reseting it from Component::onBeforeDeleteObject
                componentPtr->m_parentObject = nullptr;
            }

            // clearing intrusive list (prior temporary components collection)
            m_components.clear();

            // actually delete component objects: .clear() will also trigger components deletion through ObjectUniquePtr destructor
            components.clear();
        }
    }

    void SceneObject::onBeforeDeleteObject()
    {
        if (m_parent)
        {
            resetParentInternal(nullptr, SetParentOpts::DontKeepWorldTransform);
        }
    }

    void SceneObject::destroy()
    {
        const bool isSceneRoot = m_scene && (this == &m_scene->getRoot());
        NAU_ASSERT(!isSceneRoot, "Can not explicitly destroy scene root");
        if (isSceneRoot)
        {
            return;
        }

        getServiceProvider().get<SceneManagerImpl>().destroySceneObject(*this);
    }

    void SceneObject::clearSceneReferencesRecursive()
    {
        clearAllWeakReferences();

        for (auto& componentNode : m_components)
        {
            static_cast<Component&>(componentNode).clearAllWeakReferences();
        }

        for (SceneObject& child : m_children)
        {
            child.clearSceneReferencesRecursive();
        }
    }

    eastl::string_view SceneObject::getName() const
    {
        return m_name;
    }

    void SceneObject::setName(eastl::string_view name)
    {
        m_name = name;
    }

    IScene* SceneObject::getScene() const
    {
        return m_scene;
    }

    ActivationState SceneObject::getActivationState() const
    {
        return getServiceProvider().get<SceneManagerImpl>().getSceneObjectActivationState(*this);
    }

    SceneObject::Ptr SceneObject::clone()
    {
        SceneAsset::Ptr prefabAsset = scene::wrapSceneObjectAsAsset(*this);
        return getServiceProvider().get<scene::ISceneFactory>().createSceneObjectFromAsset(*prefabAsset);
    }

    const SceneComponent& SceneObject::getRootComponentInternal() const
    {
        NAU_FATAL(!m_components.empty());
        NAU_FATAL(m_rootComponent && m_rootComponent == &m_components.front());

        return *m_rootComponent;
    }

    SceneComponent& SceneObject::getRootComponentInternal()
    {
        NAU_FATAL(!m_components.empty());
        NAU_FATAL(m_rootComponent && m_rootComponent == &m_components.front());

        return *m_rootComponent;
    }

    async::Task<> SceneObject::activate()
    {
        return getServiceProvider().get<SceneManagerImpl>().activateSceneObject(*this);
    }

    SceneObject& SceneObject::attachChildInternal(SceneObject::Ptr&& childObjectPtr, bool activateNow)
    {
        NAU_FATAL(childObjectPtr);
        NAU_FATAL(!childObjectPtr->m_parent, "Invalid object's owning state");

        // Transferring child's ownership from childObjectPtr to this object.
        SceneObject* const child = childObjectPtr.giveUp();
        child->resetParentInternal(this, SetParentOpts::DontKeepWorldTransform);

        if (getActivationState() == ActivationState::Active && activateNow)
        {
            auto& manager = getServiceProvider().get<SceneManagerImpl>();
            if (auto activationTask = manager.activateSceneObject(*child); !activationTask.isReady())
            {
                NAU_LOG_WARNING("Child attached, but its activation will be completed in async fashion. Consider using attachChildAsync");
                child->getRootComponent().trackAsyncOperation(std::move(activationTask));
            }
#ifdef NAU_ASSERT_ENABLED
            else
            {
                NAU_ASSERT(child->getActivationState() == ActivationState::Active);
            }
#endif
        }

        return *child;
    }

    SceneObject& SceneObject::attachChild(scene::ObjectUniquePtr<SceneObject>&& childObjectPtr)
    {
        return attachChildInternal(std::move(childObjectPtr), true);
    }

    async::Task<ObjectWeakRef<SceneObject>> SceneObject::attachChildAsync(SceneObject::Ptr&& childObjectPtr)
    {
        ObjectWeakRef childRef = attachChildInternal(std::move(childObjectPtr), false);

        if (getActivationState() == ActivationState::Active)
        {
            auto& manager = getServiceProvider().get<SceneManagerImpl>();
            co_await manager.activateSceneObject(*childRef);
        }

        co_return childRef;
    }

    void SceneObject::removeChild(ObjectWeakRef<SceneObject> childRef)
    {
        if (!childRef)
        {
            return;
        }

        SceneObject& child = *childRef;
        if (!m_children.contains(child))
        {
            NAU_LOG_WARNING("Attempt to remove child object ({}) that does not belong to the current object ({})", child.getName(), getName());
            return;
        }

        child.destroy();
    }

    void SceneObject::resetParentInternal(SceneObject* newParent, SetParentOptsFlag options)
    {
        if (m_parent == newParent)
        {
            return;
        }

        NAU_FATAL(m_rootComponent);

        eastl::optional<math::Transform> oldWorldTransform;
        if (!options.has(SetParentOpts::DontKeepWorldTransform))
        {
            oldWorldTransform = m_rootComponent->getWorldTransform();
        }

        scope_on_leave
        {
            if (oldWorldTransform)
            {
                m_rootComponent->setWorldTransform(*oldWorldTransform);
            }
            else
            {
                m_rootComponent->notifyTransformChanged();
            }
        };

        if (auto oldParent = std::exchange(m_parent, newParent); oldParent)
        {
            NAU_FATAL(oldParent->m_children.contains(*this));
            oldParent->m_children.remove(*this);

            NAU_FATAL(oldParent->m_rootComponent);
            oldParent->m_rootComponent->removeTransformChild(*m_rootComponent);
        }

        if (m_parent)
        {
            m_parent->m_children.push_back(*this);

            NAU_FATAL(m_parent->m_rootComponent);
            m_parent->m_rootComponent->appendTransformChild(*m_rootComponent);
            setScene(m_parent->m_scene);
        }
        else
        {
            setScene(nullptr);
        }
    }

    void SceneObject::setParent(SceneObject& newParent, SetParentOptsFlag options)
    {
#ifdef NAU_ASSERT_ENABLED
        const ActivationState thisActivationState = getActivationState();
        const ActivationState parentActivationState = getActivationState();

        NAU_ASSERT(thisActivationState == ActivationState::Active || thisActivationState == ActivationState::Inactive);
        NAU_ASSERT(parentActivationState == thisActivationState);
#endif

        resetParentInternal(&newParent, options);
    }

    SceneObject* SceneObject::getParentObject() const
    {
        return m_parent;
    }

    bool SceneObject::walkChildObjectsRecursive(WalkObjectsCallback callback, void* callbackData)
    {
        NAU_ASSERT(callback);
        if (!callback)
        {
            return false;
        }

        for (SceneObject& child : m_children)
        {
            if (!child.walkChildObjectsRecursive(callback, callbackData))
            {
                return false;
            }

            if (const bool doContinue = callback(child, callbackData); !doContinue)
            {
                return false;
            }
        }

        return true;
    }

    void SceneObject::walkChildObjects(WalkObjectsCallback callback, void* callbackData, bool walkRecursive)
    {
        NAU_ASSERT(callback);
        if (!callback)
        {
            return;
        }

        if (!walkRecursive)
        {
            for (const SceneObject& child : m_children)
            {
                SceneObject& mutableChild = const_cast<SceneObject&>(child);
                if (const bool doContinue = callback(mutableChild, callbackData); !doContinue)
                {
                    return;
                }
            }
        }
        else
        {
            walkChildObjectsRecursive(callback, callbackData);
        }
    }

    Vector<SceneObject*> SceneObject::getChildObjects(bool recursive)
    {
        Vector<SceneObject*> children;
        // actually don't known how many children there are when recursive == true (hope for intelligence of allocator)
        children.reserve(m_children.size());

        if (!recursive)
        {
            for (SceneObject& child : m_children)
            {
                children.push_back(&child);
            }
        }
        else
        {
            walkChildObjectsRecursive([](SceneObject& childObject, void* ptr)
            {
                reinterpret_cast<decltype(children)*>(ptr)->push_back(&childObject);
                return true;
            }, &children);
        }

        return children;
    }

    Component& SceneObject::addComponentInternal(const rtti::TypeInfo& componentType, Functor<void(Component&)>& initializer, bool activateNow)
    {
        scene::ObjectUniquePtr<Component> componentPtr = getServiceProvider().get<IComponentFactory>().createComponent(componentType);
        NAU_FATAL(componentPtr, "Fail to create Component with specified type: ({})", componentType.getTypeName());

        Component* const childComponent = componentPtr.giveUp();
        childComponent->m_parentObject = this;
        m_components.push_back(*childComponent);

        if (SceneComponent* const sceneComponent = childComponent->as<SceneComponent*>())
        {
            NAU_FATAL(m_rootComponent);
            m_rootComponent->appendTransformChild(*sceneComponent);
        }

        if (initializer)
        {
            initializer(*childComponent);
        }

        if (IComponentEvents* const componentEvents = childComponent->as<IComponentEvents*>())
        {
            componentEvents->onComponentCreated();
        }

        if (const auto state = getActivationState(); activateNow && (state == ActivationState::Activating || state == ActivationState::Active))
        {
            auto& manager = getServiceProvider().get<SceneManagerImpl>();
            if (auto activationTask = manager.activateComponents({childComponent}, false); !activationTask.isReady())
            {
                NAU_LOG_WARNING("Component is added, but its activation will be completed in async fashion. Consider using attachChildAsync");
                childComponent->trackAsyncOperation(std::move(activationTask));
            }
#ifdef NAU_ASSERT_ENABLED
            else
            {
                NAU_ASSERT(childComponent->getActivationState() == ActivationState::Active);
            }
#endif
        }

        return *childComponent;
    }

    Component& SceneObject::addComponent(const rtti::TypeInfo& componentType, Functor<void(Component&)> initializer)
    {
        return addComponentInternal(componentType, initializer, true);
    }

    async::Task<ObjectWeakRef<Component>> SceneObject::addComponentAsync(const rtti::TypeInfo& componentType, Functor<void(Component&)> initializer)
    {
        ObjectWeakRef component = addComponentInternal(componentType, initializer, false);
        if (getActivationState() == ActivationState::Active)
        {
            SceneManagerImpl& manager = getServiceProvider().get<SceneManagerImpl>();
            co_await manager.activateComponents({component.get()}, false);
        }

        co_return component;
    }

    void SceneObject::removeComponent(ObjectWeakRef<Component> componentRef)
    {
        if (!componentRef)
        {
            return;
        }

        Component& component = *componentRef;
        if (!m_components.contains(component))
        {
            NAU_LOG_WARNING("Attempt to removing component that does not belongs to the scene object ({})", getName());
            return;
        }

        NAU_ASSERT(&component != m_rootComponent, "You are not able to remove root component");
        if (&component == m_rootComponent)
        {
            return;
        }

        getServiceProvider().get<SceneManagerImpl>().destroyComponent(component);
    }

    void SceneObject::removeComponentFromList(Component& component)
    {
        NAU_FATAL(component.m_parentObject == this);
        NAU_FATAL(m_components.contains(component));
        m_components.remove(component);

        // In general root component must not be removed by user.
        // But when object is deleting it state changed to ActivationState::Deactivating and all components (include root)
        // are detached from the object and deleted separately (in correct order = expecting that the root should be removed last)
        if (m_rootComponent && (static_cast<Component*>(m_rootComponent) == &component))
        {
            NAU_ASSERT(m_activationState == ActivationState::Deactivating);

            resetParentInternal(nullptr, SetParentOpts::DontKeepWorldTransform);
            m_rootComponent = nullptr;
        }

        NAU_FATAL(m_rootComponent || m_activationState == ActivationState::Deactivating);

        if (m_rootComponent)
        {
            if (SceneComponent* const sceneComponent = component.as<SceneComponent*>(); sceneComponent && sceneComponent != m_rootComponent)
            {
                m_rootComponent->removeTransformChild(*sceneComponent);
            }
        }

        component.m_parentObject = nullptr;
    }

    void SceneObject::walkComponents(WalkComponentsCallback callback, void* callbackData, bool walkRecursive, const rtti::TypeInfo* componentType)
    {
        if (!walkRecursive)
        {
            for (auto& listNode : m_components)
            {
                Component& component = static_cast<Component&>(listNode);
                if (componentType && !component.is(*componentType))
                {
                    continue;
                }

                if (const bool doContinue = callback(component, callbackData); !doContinue)
                {
                    return;
                }
            }
        }
        else
        {
            walkComponentsRecursive(callback, callbackData, componentType);
        }
    }

    void SceneObject::walkComponents(WalkConstComponentsCallback callback, void* callbackData, bool walkRecursive, const rtti::TypeInfo* componentType) const
    {
        if (!walkRecursive)
        {
            for (const auto& listNode : m_components)
            {
                const Component& component = static_cast<const Component&>(listNode);
                if (componentType && !component.is(*componentType))
                {
                    continue;
                }

                const bool doContinue = callback(component, callbackData);
                if (!doContinue)
                {
                    return;
                }
            }
        }
        else
        {
            auto mutableThis = const_cast<SceneObject*>(this);
            mutableThis->walkComponentsRecursive(reinterpret_cast<WalkComponentsCallback>(callback), callbackData, componentType);
        }
    }

    bool SceneObject::walkComponentsRecursive(WalkComponentsCallback callback, void* callbackData, const rtti::TypeInfo* componentType)
    {
        for (SceneObject& child : m_children)
        {
            if (!child.walkComponentsRecursive(callback, callbackData, componentType))
            {
                return false;
            }
        }

        walkComponents(callback, callbackData, false, componentType);

        return true;
    }

    Vector<Component*> SceneObject::getComponents(bool recursive, const rtti::TypeInfo* componentType)
    {
        size_t totalComponents = 0;
        if (recursive)
        {
            walkComponents([](const Component&, void* ptr)
            {
                ++(*reinterpret_cast<size_t*>(ptr));
                return true;
            }, &totalComponents, recursive, componentType);
        }
        else
        {
            totalComponents = m_components.size();
        }

        Vector<Component*> components;
        components.reserve(totalComponents);

        walkComponents([](Component& component, void* ptr)
        {
            reinterpret_cast<decltype(components)*>(ptr)->push_back(&component);
            return true;
        }, &components, recursive, componentType);

        return components;
    }

    Vector<const Component*> SceneObject::getComponents(bool recursive, const rtti::TypeInfo* componentType) const
    {
        size_t totalComponents = 0;
        if (recursive)
        {
            walkComponents([](const Component&, void* ptr)
            {
                ++(*reinterpret_cast<size_t*>(ptr));
                return true;
            }, &totalComponents, true, componentType);
        }
        else
        {
            totalComponents = m_components.size();
        }

        Vector<const Component*> components;
        components.reserve(totalComponents);

        walkComponents([](const Component& component, void* ptr)
        {
            reinterpret_cast<decltype(components)*>(ptr)->push_back(&component);
            return true;
        }, &components, recursive, componentType);

        return components;
    }

    Component* SceneObject::findFirstComponent(const rtti::TypeInfo& type, bool recursive)
    {
        Component* resultComponent = nullptr;

        walkComponents([](Component& component, void* ptr)
        {
            *reinterpret_cast<Component**>(ptr) = &component;
            return false;
        }, &resultComponent, recursive, &type);

        NAU_FATAL(!resultComponent || resultComponent->is(type));
        return resultComponent;
    }

    AsyncMessageSource& SceneObject::getMessageSource()
    {
        if (!m_messageSource)
        {
            m_messageSource = AsyncMessageSource::create();
        }

        return *m_messageSource;
    }

    void SceneObject::setScene(IScene* scene)
    {
        NAU_ASSERT(!scene || !m_scene, "Scene already set");

        m_scene = scene;
        for (auto& child : m_children)
        {
            child.setScene(scene);
        }
    }

}  // namespace nau::scene
