#pragma once

#ifdef STORM_REFL_PARSE
#define STORM_REFL_IGNORE __attribute__((annotate("no_refl")))
#define STORM_REFL_ATTR(x) __attribute__((annotate(#x)))
#define STORM_REFL_ATTR_VAL(name, x) __attribute__((annotate(#name " " #x)))
#else
#define STORM_REFL_IGNORE
#define STORM_REFL_ATTR(x)
#define STORM_REFL_ATTR_VAL(name, x)
#endif

#define STORM_REFL static const bool is_reflectable = true;

