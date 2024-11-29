// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/version/engine_version.h"
namespace nau
{
    EngineVersion::EngineVersion(uint16_t major, uint16_t minor, uint16_t patch, const nau::string& commit, const nau::string& branch) :
        m_major(major),
        m_minor(minor),
        m_patch(patch),
        m_commit(commit),
        m_branch(branch)
    {
    }

    bool EngineVersion::matchVersion(const EngineVersion& other) const
    {
        return (m_major == other.m_major) && (m_minor == other.m_minor) && (m_patch == other.m_patch);
    }

    bool EngineVersion::matchVersionAndBuild(const EngineVersion& other) const
    {
        return matchVersion(other) && (m_commit == other.m_commit) && (m_branch == other.m_branch);
    }

    bool EngineVersion::greaterOrEqualVersion(const EngineVersion& other) const
    {
        if (m_major < other.m_major)
        {
            return false;
        }
        if(m_major > other.m_major)
        {
            return true;
        }
        // Major matched

        if(m_minor < other.m_minor)
        {
            return false;
        }
        if(m_minor > other.m_minor)
        {
            return true;
        }
        // Minor matched

        if(m_patch < other.m_patch)
        {
            return false;
        }
        return true;
    }


    nau::string EngineVersion::toString() const
    {
        if (!m_commit.empty() || !m_branch.empty())
        {
            return nau::string::format("{}.{}.{}-{}+{}", m_major, m_minor, m_patch, m_commit, m_branch);
        }
        return nau::string::format("{}.{}.{}", m_major, m_minor, m_patch);
    }

    // TODO - used std::string for numeric conversions, check, if nau::string can be used here ?
    bool EngineVersion::parse(const std::string& engineVersionString, EngineVersion& out)
    {
        const char* startPtr;
        char* endPtr;
        startPtr = engineVersionString.c_str();
        auto major = std::strtol(startPtr, &endPtr, 10);
        if(startPtr == endPtr || *(endPtr++) != '.')
        {
            return false;
        }
        startPtr = endPtr;
        auto minor = std::strtol(startPtr, &endPtr, 10);
        if(startPtr == endPtr || *(endPtr++) != '.')
        {
            return false;
        }
        startPtr = endPtr;
        auto patch = std::strtol(startPtr, &endPtr, 10);
        if(startPtr == endPtr)
        {
            return false;
        }
        if (*endPtr == '-')
        {
            // Parse commit and branch
            std::string str(++endPtr);
            std::size_t found = str.find('+');
            if (found == std::string::npos)
            {
                return false;
            }
            out.m_commit = str.substr(0, found);
            out.m_branch = str.substr(found + 1);
        }
        out.m_major = major;
        out.m_minor = minor;
        out.m_patch = patch;
        return true;
    }

    static EngineVersion g_engineVersionStatic(NAU_VERSION_MAJOR, NAU_VERSION_MINOR, NAU_VERSION_PATCH, NAU_GIT_COMMIT, NAU_GIT_BRANCH);

    const EngineVersion& EngineVersion::current()
    {
        return g_engineVersionStatic;
    }
}  // namespace nau
