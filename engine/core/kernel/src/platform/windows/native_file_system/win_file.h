// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <atomic>
#include <mutex>

#include "nau/io/file_system.h"
#include "nau/io/stream.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::io
{
    class WinFile final : public  IFile, public IMemoryMappableObject, public INativeFile, public io_detail::IFileInternal
    {
        NAU_CLASS_(WinFile, IFile, IMemoryMappableObject, INativeFile, io_detail::IFileInternal)
    public:
        WinFile(const WinFile&) = delete;
        WinFile(const wchar_t* path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes = FILE_ATTRIBUTE_NORMAL);

        virtual ~WinFile();

        bool supports(FileFeature) const final;

        bool isOpened() const final;

        IStreamBase::Ptr createStream(std::optional<AccessModeFlag>) final;

        AccessModeFlag getAccessMode() const override;

        size_t getSize() const override;

        FsPath getPath() const override;

        void* memMap(size_t offset = 0, size_t count = 0) override;

        void memUnmap(const void*) override;

        void setVfsPath(io::FsPath path) override;

        std::string getNativePath() const override;

    private:
        FsPath m_vfsPath;
        const AccessModeFlag m_accessMode;
        HANDLE m_fileHandle = INVALID_HANDLE_VALUE;
        HANDLE m_fileMappingHandle = nullptr;
        unsigned m_fileMappingCounter = 0;
        void* m_mappedPtr = nullptr;
        std::mutex m_mutex;
    };

    class WinFileStreamBase
    {
    protected:
        WinFileStreamBase(HANDLE fileHandle);
        ~WinFileStreamBase();

        inline HANDLE getFileHandle() const
        {
            NAU_ASSERT(isOpened());
            return m_fileHandle;
        }

        inline bool isOpened() const
        {
            return m_fileHandle != INVALID_HANDLE_VALUE;
        }

        size_t getPositionInternal() const;

        size_t setPositionInternal(OffsetOrigin, int64_t);

    private:
        const HANDLE m_fileHandle;
    };


    class WinFileStreamReader final : public WinFileStreamBase, public virtual IStreamReader
    {
        NAU_CLASS_(nau::io::WinFileStreamReader,  IStreamReader)
    public:
        using WinFileStreamBase::isOpened;

        WinFileStreamReader(HANDLE fileHandle);
        WinFileStreamReader(const wchar_t* path, AccessModeFlag accessMode, OpenFileMode openMode);

        size_t getPosition() const override;

        size_t setPosition(OffsetOrigin, int64_t) override;

        Result<size_t> read(std::byte*, size_t count) override;
    };

    class WinFileStreamWriter final : public WinFileStreamBase,
                                      public virtual IStreamWriter
    {
        NAU_CLASS_(nau::io::WinFileStreamWriter, IStreamWriter)
    public:
        using WinFileStreamBase::isOpened;

        WinFileStreamWriter(HANDLE fileHandle);
        WinFileStreamWriter(const wchar_t* path, AccessModeFlag accessMode, OpenFileMode openMode);

        size_t getPosition() const override;

        size_t setPosition(OffsetOrigin, int64_t) override;

        Result<size_t> write(const std::byte*, size_t count) override;

        void flush() override;
    };

/*
    class WinFileReader : public virtual WinFileBase, public IStreamReader
    {
        NAU_CLASS_(nau::io::WinFileReader, WinFileBase, IStreamReader)
    
    public:
        WinFileReader(const char* path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes);

        bool supports(FileFeature) const final;

        size_t getOffset() const final;

        size_t setOffset(OffsetOrigin, size_t) final;

        Result<size_t> read(std::byte*, size_t count) final;
    };
*/
/*
    class WinFileWriter : public virtual WinFileReader, public IStreamWriter
    {
        NAU_CLASS_(nau::io::WinFileWriter, WinFileReader, IStreamWriter)

    public:

        WinFileWriter (const char* path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes);

        Result<> write(const std::byte*, size_t count) override;

        void flush() override;
    };
    */
    //class WinFileReaderWriter : public virtual WinFileReader, public virtual WinFileWriter
    //{
    //    NAU_CLASS_(nau::io::WinFileReaderWriter, WinFileReader, WinFileWriter)

    //public:

    //    WinFileReaderWriter (const char* path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes);

    //};

    // class WinAsyncFileReader : public WinFileBase, public IAsync
    // {
    //     NAU_CLASS_(nau::io::WinAsyncFileReader, WinFileBase
    // };

    // class WinAsyncFileWriter : public WinAsyncFileReader
    // {

    // };

    // class WinFileReader : public IStreamReader
    // {
    // };

    //IFile::Ptr createNativeFileInternal(const char* path, AccessModeFlag accessMode, OpenFileMode openMode);

}  // namespace nau::io
