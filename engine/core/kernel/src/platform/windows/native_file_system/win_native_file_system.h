// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include "nau/io/file_system.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::io
{
    class WinNativeFileSystem final : public IMutableFileSystem,
                                      public INativeFileSystem
    {
        NAU_CLASS_(nau::io::WinNativeFileSystem, IMutableFileSystem, INativeFileSystem)

    public:
        WinNativeFileSystem(std::string rootPath, bool isReadonly);

        bool isReadOnly() const override;

        bool exists(const FsPath&, std::optional<FsEntryKind> kind) override;

        size_t getLastWriteTime(const FsPath&) override;

        IFile::Ptr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) override;

        OpenDirResult openDirIterator(const FsPath& path) override;

        void closeDirIterator(void*) override;

        FsEntry incrementDirIterator(void*) override;

        Result<> createDirectory(const FsPath&) override;

        Result<> remove(const FsPath&, bool recursive = false) override;

        std::wstring resolveToNativePath(const FsPath& path) override;

    private:

        std::filesystem::path resolveToNativePathNoCheck(const FsPath& path);

        const std::string m_basePath;
        bool m_isReadOnly;
    };
}