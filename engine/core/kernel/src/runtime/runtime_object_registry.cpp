// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/runtime/internal/runtime_object_registry.h"

#include "nau/memory/singleton_memop.h"
#include "nau/threading/lock_guard.h"
#include "nau/utils/scope_guard.h"

namespace nau
{

    class RuntimeObjectRegistryImpl final : public RuntimeObjectRegistry
    {
        NAU_DECLARE_SINGLETON_MEMOP(RuntimeObjectRegistryImpl)
    public:
        RuntimeObjectRegistryImpl() = default;

        ~RuntimeObjectRegistryImpl();

        [[nodiscard]]
        ObjectId addObject(nau::Ptr<>);

        [[nodiscard]]
        ObjectId addObject(IRttiObject&);

        void removeObject(ObjectId);

        bool isAutoRemovable(ObjectId);

    private:
        class ObjectEntry
        {
        public:
            ObjectEntry() :
                m_objectId(0),
                m_ptr(nullptr),
                m_isWeak(false)

            {
            }

            ObjectEntry(ObjectId objectId, IRttiObject& object) :
                m_objectId(objectId),
                m_ptr(&object),
                m_isWeak(false)
            {
            }

            ObjectEntry(ObjectId objectId, nau::Ptr<>& object) :
                m_objectId(objectId),
                m_ptr(getWeakRef(object)),
                m_isWeak(true)
            {
            }

            ObjectEntry(const ObjectEntry&) = delete;

            ObjectEntry(ObjectEntry&& other) :
                m_objectId(std::exchange(other.m_objectId, 0)),
                m_ptr(std::exchange(other.m_ptr, nullptr)),
                m_isWeak(std::exchange(other.m_isWeak, false))
            {
            }

            ~ObjectEntry()
            {
                reset();
            }

            ObjectEntry& operator=(const ObjectEntry&) = delete;

            ObjectEntry& operator=(ObjectEntry&& other)
            {
                reset();

                m_objectId = std::exchange(other.m_objectId, 0);
                m_ptr = std::exchange(other.m_ptr, nullptr);
                m_isWeak = std::exchange(other.m_isWeak, false);

                return *this;
            }

            explicit operator bool() const
            {
                return m_ptr != nullptr;
            }

            ObjectId getObjectId() const
            {
                return m_objectId;
            }

            bool isWeakRef() const
            {
                return m_isWeak;
            }

            bool isExpired() const
            {
                return !m_ptr || (m_isWeak && reinterpret_cast<const IWeakRef*>(m_ptr)->isDead());
            }

            IRttiObject* lock() const
            {
                if(!m_ptr)
                {
                    return nullptr;
                }

                if(m_isWeak)
                {
                    return reinterpret_cast<IWeakRef*>(m_ptr)->acquire();
                }

                IRttiObject* const instance = reinterpret_cast<IRttiObject*>(m_ptr);
                if(IRefCounted* const refCounted = instance->as<IRefCounted*>())
                {  // always add reference if object is ref counted (even it was registered throug IRttiObject& constructor)
                   // this is makes logic below where objects are used more simple (just always releaseRef())
                    refCounted->addRef();
                }

                return instance;
            }

            bool isMyNonWeakRefObject(const IRttiObject* object) const
            {
                return !m_isWeak && m_ptr == object;
            }

        private:
            static void* getWeakRef(nau::Ptr<>& object)
            {
                NAU_ASSERT(object);
                return object->getWeakRef();
            }

            void reset()
            {
                if(m_ptr && m_isWeak)
                {
                    reinterpret_cast<IWeakRef*>(m_ptr)->releaseRef();
                }

                m_ptr = nullptr;
                m_isWeak = false;
            }

            ObjectId m_objectId;
            void* m_ptr;
            bool m_isWeak;
        };

        void removeExpiredEntries();

        void visitObjects(void (*callback)(eastl::span<IRttiObject*>, void*), const rtti::TypeInfo*, void*) override;

        std::recursive_mutex m_mutex;
        ObjectId m_objectId = 0;
        eastl::vector<ObjectEntry> m_objects;
    };

    RuntimeObjectRegistryImpl::~RuntimeObjectRegistryImpl()
    {
        removeExpiredEntries();
        NAU_ASSERT(m_objects.empty(), "Still alive ({}) objects", m_objects.size());
    }

    void RuntimeObjectRegistryImpl::visitObjects(VisitObjectsCallback callback, const rtti::TypeInfo* type, void* callbackData)
    {
        lock_(m_mutex);

        if(m_objects.empty())
        {
            return;
        }

        eastl::vector<IRttiObject*> instances;

        scope_on_leave
        {
            for(IRttiObject* const obj : instances)
            {
                if(IRefCounted* const refCounted = obj->as<IRefCounted*>())
                {
                    refCounted->releaseRef();
                }
            }
        };

        // collect required objects, remove expired entries
        for(size_t i = 0; i < m_objects.size();)
        {
            IRttiObject* const instance = m_objects[i].lock();
            if(!instance)
            {
                const size_t lastIndex = m_objects.size() - 1;
                if(i != lastIndex)
                {
                    m_objects[i] = std::move(m_objects[lastIndex]);
                }

                m_objects.resize(lastIndex);
                continue;
            }

            if(!type || instance->is(*type))
            {
                instances.push_back(std::move(instance));
            }
            else if(IRefCounted* const refCounted = instance->as<IRefCounted*>())
            {  // object will not be used and must be released immediately
                refCounted->releaseRef();
            }

            ++i;
        }

        if(!instances.empty())
        {
            callback({instances.begin(), instances.end()}, callbackData);
        }
    }

    RuntimeObjectRegistry::ObjectId RuntimeObjectRegistryImpl::addObject(nau::Ptr<> ptr)
    {
        lock_(m_mutex);

        NAU_ASSERT(ptr);

        const auto id = ++m_objectId;
        m_objects.emplace_back(id, ptr);

        return id;
    }

    RuntimeObjectRegistry::ObjectId RuntimeObjectRegistryImpl::addObject(IRttiObject& object)
    {
        lock_(m_mutex);

        const auto id = ++m_objectId;
        m_objects.emplace_back(id, std::ref(object));

        return id;
    }

    void RuntimeObjectRegistryImpl::removeExpiredEntries()
    {
        eastl::erase_if(m_objects, [](const ObjectEntry& entry)
                        {
                            return entry.isExpired();
                        });
    }

    void RuntimeObjectRegistryImpl::removeObject(ObjectId id)
    {
        lock_(m_mutex);

        for(size_t i = 0, size = m_objects.size(); i < size; ++i)
        {
            if(m_objects[i].getObjectId() == id)
            {
                const auto lastIndex = size - 1;
                if(i != lastIndex)
                {
                    m_objects[i] = std::move(m_objects[lastIndex]);
                }
                m_objects.resize(lastIndex);
                break;
            }
        }
    }

    bool RuntimeObjectRegistryImpl::isAutoRemovable(ObjectId objectId)
    {
        lock_(m_mutex);

        auto iter = eastl::find_if(m_objects.begin(), m_objects.end(), [objectId](const ObjectEntry& entry)
                                   {
                                       return entry.getObjectId() == objectId;
                                   });

        return iter != m_objects.end() && iter->isWeakRef();
    }

    namespace
    {
        auto& getRuntimeObjectRegistryRef()
        {
            static eastl::unique_ptr<RuntimeObjectRegistryImpl> s_runtimeObjectRegistry;

            return (s_runtimeObjectRegistry);
        }
    }  // namespace

    RuntimeObjectRegistry& RuntimeObjectRegistry::getInstance()
    {
        NAU_FATAL(getRuntimeObjectRegistryRef());

        return *getRuntimeObjectRegistryRef();
    }

    bool RuntimeObjectRegistry::hasInstance()
    {
        return static_cast<bool>(getRuntimeObjectRegistryRef());
    }

    void RuntimeObjectRegistry::setDefaultInstance()
    {
        NAU_ASSERT(!getRuntimeObjectRegistryRef());

        getRuntimeObjectRegistryRef() = eastl::make_unique<RuntimeObjectRegistryImpl>();
    }

    void RuntimeObjectRegistry::releaseInstance()
    {
        getRuntimeObjectRegistryRef().reset();
    }

    RuntimeObjectRegistration::RuntimeObjectRegistration() :
        m_objectId(0)
    {
    }

    RuntimeObjectRegistration::RuntimeObjectRegistration(nau::Ptr<> object) :
        RuntimeObjectRegistration()
    {
        if(RuntimeObjectRegistry::hasInstance())
        {
            m_objectId = getRuntimeObjectRegistryRef()->addObject(std::move(object));
        }
    }

    RuntimeObjectRegistration::RuntimeObjectRegistration(IRttiObject& object) :
        RuntimeObjectRegistration()
    {
        if(RuntimeObjectRegistry::hasInstance())
        {
            m_objectId = getRuntimeObjectRegistryRef()->addObject(object);
        }
    }

    RuntimeObjectRegistration::RuntimeObjectRegistration(RuntimeObjectRegistration&& other) :
        m_objectId(std::exchange(other.m_objectId, 0))
    {
    }

    RuntimeObjectRegistration::~RuntimeObjectRegistration()
    {
        reset();
    }

    RuntimeObjectRegistration& RuntimeObjectRegistration::operator=(RuntimeObjectRegistration&& other)
    {
        reset();
        m_objectId = std::exchange(other.m_objectId, 0);
        return *this;
    }

    RuntimeObjectRegistration& RuntimeObjectRegistration::operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }

    RuntimeObjectRegistration::operator bool() const
    {
        return m_objectId != 0;
    }

    void RuntimeObjectRegistration::setAutoRemove()
    {
        if(m_objectId == 0 || !RuntimeObjectRegistry::hasInstance())
        {
            return;
        }
        const bool isAutoRemovable = getRuntimeObjectRegistryRef()->isAutoRemovable(m_objectId);
        NAU_FATAL(isAutoRemovable, "Object can not be used as autoremovable");

        if(isAutoRemovable)
        {
            m_objectId = 0;
        }
    }

    void RuntimeObjectRegistration::reset()
    {
        if(m_objectId != 0)
        {
            const auto objectId = std::exchange(m_objectId, 0);

            NAU_ASSERT(RuntimeObjectRegistry::hasInstance());
            if(RuntimeObjectRegistry::hasInstance())
            {
                getRuntimeObjectRegistryRef()->removeObject(objectId);
            }
        }
    }
}  // namespace nau
