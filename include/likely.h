#pragma once
static inline bool(likely)(bool x) { return __builtin_expect((x), true); }
static inline bool(unlikely)(bool x) { return __builtin_expect((x), false); }