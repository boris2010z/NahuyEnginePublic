// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


// clang-format off

#if __has_include(<EASTL/allocator.h>)

#include <EASTL/allocator.h>

#if !defined(EASTL_USER_DEFINED_ALLOCATOR)

#if !EASTL_DLL

void* operator new[](size_t size, [[maybe_unused]] const char* pName, [[maybe_unused]] int flags, [[maybe_unused]] unsigned debugFlags, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
    {
        return ::malloc(size);
    };

void* operator new[](size_t size, size_t alignment, [[maybe_unused]] size_t alignmentOffset, [[maybe_unused]] const char* pName, [[maybe_unused]] int flags, [[maybe_unused]] unsigned debugFlags, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
    {
        return ::_aligned_malloc(size, alignment);
        // return ::malloc(size);
    };

#endif  // !EASTL_DLL

#else

namespace eastl
{

    allocator::allocator(const char* EASTL_NAME(pName))
    {
#if EASTL_NAME_ENABLED
        mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
    }

    allocator::allocator(const allocator& EASTL_NAME(alloc))
    {
#if EASTL_NAME_ENABLED
        mpName = alloc.mpName;
#endif
    }

    allocator::allocator(const allocator&, const char* EASTL_NAME(pName))
    {
#if EASTL_NAME_ENABLED
        mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
    }

    allocator& allocator::operator=(const allocator& EASTL_NAME(alloc))
    {
#if EASTL_NAME_ENABLED
        mpName = alloc.mpName;
#endif
        return *this;
    }

    const char* allocator::get_name() const
    {
#if EASTL_NAME_ENABLED
        return mpName;
#else
        return EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
    }

    void allocator::set_name(const char* EASTL_NAME(pName))
    {
#if EASTL_NAME_ENABLED
        mpName = pName;
#endif
    }

    void* allocator::allocate(size_t n, int flags)
    {
        return ::malloc(n);
    }

    void* allocator::realloc(void* p, size_t n, int flags)
    {
        return ::realloc(p, n);
    }

    void* allocator::allocate(size_t n, size_t alignment, size_t offset, int flags)
    {
        return ::malloc(n);
    }

    void allocator::deallocate(void* p, size_t)
    {
        ::free(p);
    }

    bool operator==(const allocator& a, const allocator& b)
    {
        return true;
    }

    bool operator!=(const allocator& a, const allocator& b)
    {
        return false;
    }

  /// gDefaultAllocator
  /// Default global allocator instance. 
  NAU_KERNEL_EXPORT allocator  gDefaultAllocator;
  NAU_KERNEL_EXPORT allocator* gpDefaultAllocator = &gDefaultAllocator;

  NAU_KERNEL_EXPORT allocator* GetDefaultAllocator()
  {
    return gpDefaultAllocator;
  }

  NAU_KERNEL_EXPORT allocator* SetDefaultAllocator(allocator* pAllocator)
  {
    allocator* const pPrevAllocator = gpDefaultAllocator;
    gpDefaultAllocator = pAllocator;
    return pPrevAllocator;
  }

}  // namespace eastl
#endif
#endif

namespace EA
{
    namespace StdC
    {
        NAU_KERNEL_EXPORT int Vsnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments)
        {
            return vsnprintf(pDestination, n, pFormat, arguments);
        };
    }
}





// clang-format on