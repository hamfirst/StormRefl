#pragma once

#ifdef STORM_REFL_PARSE
#define STORM_REFL_IGNORE __attribute__((annotate("no_refl")))
#define STORM_REFL_ATTR(x) __attribute__((annotate(#x)))
#define STORM_REFL_ATTR_VAL(name, x) __attribute__((annotate(#name " " #x)))
#define STORM_REFL_FUNC __attribute__((annotate("refl_func")))
#else
#define STORM_REFL_IGNORE
#define STORM_REFL_ATTR(x)
#define STORM_REFL_ATTR_VAL(name, x)
#define STORM_REFL_FUNC
#endif

#define STORM_REFL static const bool is_reflectable = true;
#define STORM_REFL_FUNCS static const bool is_functional = true;

