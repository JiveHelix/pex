#pragma once


template<size_t size, class...> struct PrintTypes; // intentionally undefined


template<typename TestType, typename = void>
struct TypeTester
{
    using Type = TestType;
};

// #define TYPE_TESTER_LIMIT 2048

#ifdef TYPE_TESTER_LIMIT

template<typename TestType>
struct TypeTester
<
    TestType,
    std::enable_if_t<(sizeof(TestType) > TYPE_TESTER_LIMIT)>
>
{
    using Type = PrintTypes<sizeof(TestType), TestType>;
};

#endif

