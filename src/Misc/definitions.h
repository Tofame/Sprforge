#pragma once

#if defined(__clang__)
#define FUNC_NAME __PRETTY_FUNCTION__
#elif defined(__GNUC__) || defined(__GNUG__)
#define FUNC_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNC_NAME __FUNCSIG__
#else
    #define FUNC_NAME __func__
#endif