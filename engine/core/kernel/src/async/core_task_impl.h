// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <atomic>

#include "nau/async/core/core_task.h"
#include "nau/memory/mem_allocator.h"

namespace nau::async
{

    /**
     */
    class CoreTaskImpl final : public CoreTask
    {
    public:
        CoreTaskImpl(IMemAllocator::Ptr, void* allocatedStorage, size_t size, StateDestructorCallback destructor);

        ~CoreTaskImpl();

        // uintptr_t getTaskId() const override;
        void addRef() override;
        void releaseRef() override;
        bool isReady() const override;

        Error::Ptr getError() const override;
        const void* getData() const override;
        void* getData() override;
        size_t getDataSize() const override;
        void setContinuation(TaskContinuation) override;
        bool hasContinuation() const override;
        bool hasCapturedExecutor() const override;

        void setContinueOnCapturedExecutor(bool continueOnCapturedExecutor) override;
        bool isContinueOnCapturedExecutor() const override;

        void setReadyCallback(ReadyCallback callback, void*, void* = nullptr) override;
        bool tryResolveInternal(ResolverCallback, void*) override;

        CoreTaskImpl* getNext() const;
        void setNext(CoreTaskImpl*);

        std::string getName() const;
        void setName(std::string name);



    private:
        void invokeReadyCallback();
        void tryScheduleContinuation();

        IMemAllocator::Ptr m_allocator;

        // In some cases m_allocatedStorage can differ from (void*)this, because of custom types alignment.
        // For simplification aligned storage allocation, just keeps m_allocatedStorage (which may initially have incorrect alignment).
        void* const m_allocatedStorage;
        const size_t m_dataSize;
        const StateDestructorCallback m_destructor;
        std::atomic<uint32_t> m_refsCount{1};
        mutable std::atomic<uint32_t> m_flags{0};
        Error::Ptr m_error;
        TaskContinuation m_continuation;
        Executor::Invocation m_readyCallback;
        std::atomic<bool> m_isContinueOnCapturedExecutor = true;
        CoreTaskImpl* m_next = nullptr;
        std::string m_name = "";
    };

}  // namespace nau::async
