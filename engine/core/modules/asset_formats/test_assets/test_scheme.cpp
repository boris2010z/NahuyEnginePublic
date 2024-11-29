// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/assets/asset_content_provider.h"
#include "nau/assets/asset_manager.h"
#include "nau/assets/asset_path_resolver.h"
#include "nau/assets/texture_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/test/helpers/app_guard.h"

namespace nau::test
{
    namespace
    {
        /**
         */
        class MyAssetPathResolver : public IAssetPathResolver
        {
            NAU_TYPEID(nau::test::MyAssetPathResolver)
            NAU_CLASS_BASE(IAssetPathResolver)

            static constexpr eastl::string_view SchemeToFile = "path2file";
            static constexpr eastl::string_view SchemeToContent = "path2content";
            static constexpr eastl::string_view SchemeToInvalid = "path2invalid";

            eastl::tuple<AssetPath, AssetContentInfo> resolvePath(const AssetPath& assetPath) override
            {
                if (assetPath.hasScheme(SchemeToFile))
                {
                    return {AssetPath(assetPath).setScheme("file"), {}};
                }
                else if (assetPath.hasScheme(SchemeToContent))
                {
                    if (assetPath.getContainerPath() == "png")
                    {
                        return {AssetPath{"t_content:tex_png"}, {.kind = "png"}};
                    }
                }
                else if (assetPath.hasScheme(SchemeToInvalid))
                {
                    return {AssetPath{"unknown:/some_file"}, {}};
                }

                return {};
            }

            eastl::vector<eastl::string_view> getSupportedSchemes() const override
            {
                return {
                    SchemeToFile,
                    SchemeToContent,
                    SchemeToInvalid};
            }
        };

        /**
         */
        class MyContentProvider : public IAssetContentProvider
        {
            NAU_TYPEID(nau::test::MyContentProvider)
            NAU_CLASS_BASE(IAssetContentProvider)

            static constexpr eastl::string_view Scheme{"t_content"};

            Result<AssetContent> openStreamOrContainer(const AssetPath& assetPath) override
            {
                using namespace nau::io;

                NAU_ASSERT(assetPath.hasScheme(Scheme));
                if (!assetPath.hasScheme(Scheme))
                {
                    return NauMakeError("Invalid scheme");
                }

                io::IStreamBase::Ptr stream;
                AssetContentInfo info;

                auto& vfs = getServiceProvider().get<IFileSystem>();

                if (assetPath.getContainerPath() == "tex_png")
                {
                    info.kind = "png";
                    stream = vfs.openFile("/content/white_8x8.png", io::AccessMode::Read, io::OpenFileMode::OpenExisting)->createStream();
                }

                return {std::move(stream), std::move(info)};
            }

            eastl::vector<eastl::string_view> getSupportedSchemes() const override
            {
                return {
                    Scheme};
            }
        };
    }  // namespace

    /**
     */
    class TestAssetScheme : public testing::Test
    {
        void SetUp() final
        {
            m_app.start();
        }

        void TearDown() final
        {
            m_app.stop();
        }

    protected:
        /**
         */
        class MyTestApp final : public AppGuard
        {
            void setupTestServices() override
            {
                registerServices<MyContentProvider, MyAssetPathResolver>();
            }
        };

        static constexpr size_t PngWidth = 8;
        static constexpr size_t PngHeight = 8;

        static IAssetManager& getAssetManager()
        {
            return getServiceProvider().get<IAssetManager>();
        }

        static nau::Ptr<ITextureAssetAccessor> assetToTexture(IAssetDescriptor::Ptr asset)
        {
            using namespace nau::async;

            auto result = async::run([asset]() -> Task<Ptr<>>
            {
                return asset->getRawAsset();
            }, Executor::getDefault());

            return *async::waitResult(std::move(result));
        }

        MyTestApp m_app;
    };

    /**
        Test: just opens assets with default known scheme 'file:'
     */
    TEST_F(TestAssetScheme, OpenFileAsset)
    {
        {
            auto asset = getAssetManager().openAsset(AssetPath{"file:/content/white_8x8.png"});
            ASSERT_TRUE(asset);
            nau::Ptr<ITextureAssetAccessor> tex = assetToTexture(asset);
            ASSERT_TRUE(tex);
            ASSERT_EQ(tex->getDescription().width, PngWidth);
            ASSERT_EQ(tex->getDescription().height, PngHeight);
        }
    }

    /**
        Test: correct handle opening asset with the unknown asset scheme:  must returns nullptr
     */
    TEST_F(TestAssetScheme, UnknownScheme1)
    {
        auto asset = getAssetManager().openAsset(AssetPath{"unknown:/content/white_8x8.png"});
        ASSERT_FALSE(asset);
    }

    /**
        Test: correct handle opening asset with the unknown asset scheme: must returns nullptr.
        This test checks more complex case: 'path2invalid' is acceptable, but next it resolved to 'unknown'.
     */
    TEST_F(TestAssetScheme, UnknownScheme2)
    {
        auto asset = getAssetManager().openAsset(AssetPath{"path2invalid:/content/white_8x8.png"});
        ASSERT_FALSE(asset);
    }

    /**
        Test: custom content provider that can handle 't_content' scheme
     */
    TEST_F(TestAssetScheme, CustomContentProvider)
    {
        auto assetPng = getAssetManager().openAsset(AssetPath{"t_content:tex_png"});
        EXPECT_TRUE(assetPng);
        nau::Ptr<ITextureAssetAccessor> texPng = assetToTexture(assetPng);
        EXPECT_TRUE(texPng);
        EXPECT_EQ(texPng->getDescription().width, PngWidth);
    }

    /**
        Test: custom path resolver that can handle 'path2file' scheme
     */
    TEST_F(TestAssetScheme, CustomPathResolver)
    {
        auto assetPng = getAssetManager().openAsset(AssetPath{"path2file:/content/white_8x8.png"});
        EXPECT_TRUE(assetPng);
        nau::Ptr<ITextureAssetAccessor> texPng = assetToTexture(assetPng);
        EXPECT_TRUE(texPng);
        EXPECT_EQ(texPng->getDescription().width, PngWidth);
    }

    /**
        Test: custom path resolver will resolve 'path2content' to 't_content'
        which in turn must be resolved with content provider.
     */
    TEST_F(TestAssetScheme, CustomSchemeChained)
    {
        auto assetPng = getAssetManager().openAsset(AssetPath{"path2content:png"});
        ASSERT_TRUE(assetPng);
        nau::Ptr<ITextureAssetAccessor> texPng = assetToTexture(assetPng);
        ASSERT_TRUE(texPng);
        ASSERT_EQ(texPng->getDescription().width, PngWidth);
    }

    /**
     */
    TEST_F(TestAssetScheme, OpenSameAssetWithDifferentPaths)
    {
        {
            auto asset1 = getAssetManager().openAsset(AssetPath{"path2file:/content/white_8x8.png"});
            auto asset2 = getAssetManager().openAsset(AssetPath{"file:/content/white_8x8.png"});

            ASSERT_TRUE(asset1);
            ASSERT_TRUE(asset2);
            ASSERT_EQ(asset1->getAssetId(), asset2->getAssetId());
        }

        {
            auto asset1 = getAssetManager().openAsset(AssetPath{"path2content:png"});
            auto asset2 = getAssetManager().openAsset(AssetPath{"t_content:tex_png"});

            ASSERT_TRUE(asset1);
            ASSERT_TRUE(asset2);
            ASSERT_EQ(asset1->getAssetId(), asset2->getAssetId());
        }
    }

    /**
     */
    TEST_F(TestAssetScheme, KeepAssetInnerPath)
    {
        AssetPath path{"path2file:/content/white_8x8.png+[mip1]"};

        Result<AssetPath> resolvedPath = path.resolve();
        ASSERT_TRUE(resolvedPath);

        ASSERT_NE(*resolvedPath, path);
        ASSERT_EQ(resolvedPath->getAssetInnerPath(), path.getAssetInnerPath());
    }
}  // namespace nau::test
