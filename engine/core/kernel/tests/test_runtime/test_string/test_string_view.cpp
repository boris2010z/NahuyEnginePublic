// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/string/string.h"
#include <string>
#include "EASTL/vector.h"

using namespace std::string_literals;  // Necessary for operator "s"


namespace nau::test
{
    TEST(NauStringView, baseConstructors)
    {
        nau::string c_str8u{u8"TEST: русские, 🤝  и ツ♫你好 symbols"};
        nau::string c_str16u{u"TEST: русские, 🤝  и ツ♫你好 symbols"};
        nau::string c_str32u{U"TEST: русские, 🤝  и ツ♫你好 symbols"};

        eastl::vector<nau::string_view> test_strings = {
            nau::string_view(c_str8u),
            nau::string_view(c_str16u),
            nau::string_view(c_str32u),
        };

        EXPECT_EQ(test_strings[0].data(), c_str8u.c_str());
        EXPECT_EQ(test_strings[1].data(), c_str16u.c_str());
        EXPECT_EQ(test_strings[2].data(), c_str32u.c_str());
        EXPECT_EQ(test_strings[0].c_str(), c_str8u.c_str());
        EXPECT_EQ(test_strings[1].c_str(), c_str16u.c_str());
        EXPECT_EQ(test_strings[2].c_str(), c_str32u.c_str());

        EXPECT_EQ(test_strings[0].substr(1, 1).data(), c_str8u.c_str() + 1);
        EXPECT_EQ(test_strings[1].substr(1, 1).data(), c_str16u.c_str() + 1);
        EXPECT_EQ(test_strings[2].substr(1, 1).data(), c_str32u.c_str() + 1);

        EXPECT_EQ(test_strings[0].substr(20, 1).data(), c_str8u.c_str() + 31);
        EXPECT_EQ(test_strings[1].substr(20, 1).data(), c_str16u.c_str() + 31);
        EXPECT_EQ(test_strings[2].substr(20, 1).data(), c_str32u.c_str() + 31);

        for(auto& string_view : test_strings)
        {
            ASSERT_EQ(string_view.length(), 32);
            ASSERT_EQ(string_view.size(), 51);
            for(int i = 0; i < string_view.length(); i++)
            {
                EXPECT_EQ(string_view[i], U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
                EXPECT_EQ(string_view.at(i), U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
            }
            for(int i = 0; i < string_view.size(); i++)
            {
                EXPECT_EQ(string_view.data()[i], u8"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
            }
        }
        for(auto& string_view : test_strings)
        {
            string_view.remove_prefix(1);
            ASSERT_EQ(string_view.length(), 31);
            ASSERT_EQ(string_view.size(), 50);
            for(int i = 0; i < string_view.length(); i++)
            {
                EXPECT_EQ(string_view[i], U"EST: русские, 🤝  и ツ♫你好 symbols"[i]);
                EXPECT_EQ(string_view.at(i), U"EST: русские, 🤝  и ツ♫你好 symbols"[i]);
            }
            for(int i = 0; i < string_view.size(); i++)
            {
                EXPECT_EQ(string_view.data()[i], u8"EST: русские, 🤝  и ツ♫你好 symbols"[i]);
            }
        }
        for(auto& string_view : test_strings)
        {
            string_view.remove_prefix(6);
            ASSERT_EQ(string_view.length(), 25);
            ASSERT_EQ(string_view.size(), std::strlen((char*)u8"усские, 🤝  и ツ♫你好 symbols"));
            for(int i = 0; i < string_view.length(); i++)
            {
                EXPECT_EQ(string_view[i], U"усские, 🤝  и ツ♫你好 symbols"[i]);
                EXPECT_EQ(string_view.at(i), U"усские, 🤝  и ツ♫你好 symbols"[i]);
            }
            for(int i = 0; i < string_view.size(); i++)
            {
                EXPECT_EQ(string_view.data()[i], u8"усские, 🤝  и ツ♫你好 symbols"[i]);
            }
        }
        for(auto& string_view : test_strings)
        {
            string_view.remove_suffix(11);
            ASSERT_EQ(string_view.length(), 14);
            ASSERT_EQ(string_view.size(), std::strlen((char*)u8"усские, 🤝  и ツ"));
            for(int i = 0; i < string_view.length(); i++)
            {
                EXPECT_EQ(string_view[i], U"усские, 🤝  и ツ"[i]);
                EXPECT_EQ(string_view.at(i), U"усские, 🤝  и ツ"[i]);
            }
            for(int i = 0; i < string_view.size(); i++)
            {
                EXPECT_EQ(string_view.data()[i], u8"усские, 🤝  и ツ"[i]);
            }
        }
    }

    TEST(NauStringView, iterators)
    {
        nau::string test_str{u8"TEST: русские, 🤝  и ツ♫你好 symbols"};
        nau::string_view str_view = test_str;

        int i = 0;
        for(auto it = str_view.begin(); it < str_view.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());
        i = 0;
        for(auto it = str_view.cbegin(); it < str_view.cend(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());
        i = str_view.length();
        for(auto it = str_view.rbegin(); it < str_view.rend(); i--, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i - 1]);
        }
        EXPECT_EQ(i, 0);
        i = str_view.length();
        for(auto it = str_view.crbegin(); it < str_view.crend(); i--, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i - 1]);
        }
        EXPECT_EQ(i, 0);

        nau::string_view str_view2 = test_str;

        i = 0;
        for(auto it = str_view2.begin(); it < str_view2.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());

        str_view2 = test_str;

        i = 0;
        for(auto it = str_view2.begin(); it < str_view2.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());

        nau::string_view str_view3 = std::move(str_view);
        ASSERT_TRUE(str_view.empty());

        i = 0;
        for(auto it = str_view3.begin(); it < str_view3.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());

        nau::string_view sub_str_view1 = str_view3.substr(0, 0);
        ASSERT_TRUE(sub_str_view1.empty());
        sub_str_view1 = str_view3.substr(1, 0);
        ASSERT_TRUE(sub_str_view1.empty());
        sub_str_view1 = str_view3.substr(1, 1);
        sub_str_view1 = sub_str_view1.substr(0, 0);
        ASSERT_TRUE(sub_str_view1.empty());
        sub_str_view1 = str_view3.substr(0, 10);

        str_view = test_str;
        str_view = str_view.substr(7, 15);

        i = 0;
        for(auto it = str_view.begin(); it < str_view.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i]);
        }
        EXPECT_EQ(i, 15);
        i = 0;
        for(auto it = str_view.cbegin(); it < str_view.cend(); i++, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i]);
        }
        i = str_view.length();

        [[maybe_unused]]
        auto t1 = str_view.rbegin();

        [[maybe_unused]]
        auto t2 = str_view.rend();
        for(auto it = str_view.rbegin(); it < str_view.rend(); i--, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i - 1]);
        }
        EXPECT_EQ(i, 0);
        i = str_view.length();
        for(auto it = str_view.crbegin(); it < str_view.crend(); i--, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i - 1]);
        }
        EXPECT_EQ(i, 0);

        str_view3 = std::move(str_view);
        ASSERT_TRUE(str_view.empty());

        i = 0;
        for(auto it = str_view3.begin(); it < str_view3.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i]);
        }
        EXPECT_EQ(i, 15);

        nau::string copy_test = u8"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

        str_view3.copy(copy_test, 15, 0);
        i = 0;
        for(auto it = copy_test.begin(); it < copy_test.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i]);
        }
        EXPECT_EQ(i, 15);
    }
}  // namespace nau::test