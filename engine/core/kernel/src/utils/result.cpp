// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/utils/result.h"


namespace nau
{
    Result<void>::Result(Result<>&& other)
        : m_error(std::move(other.m_error))
    {}

    Result<void>::operator bool () const
    {
        return !isError();
    }

    bool Result<void>::isError() const
    {
        return static_cast<bool>(m_error);
    }

    Error::Ptr Result<void>::getError() const
    {
        NAU_ASSERT(isError(), "Result<void> has no error");

        return m_error;
    }

    bool Result<>::isSuccess(Error::Ptr* error) const
    {
        if (m_error && error)
        {
            *error = m_error;
            return false;
        }

        return true;
    }

    void Result<void>::ignore() const noexcept
    {
        NAU_ASSERT(!m_error, "Ignoring Result<> that holds an error:{}", m_error->getMessage());
    }

} // namespace nau
