#ifndef ALG_H
#define ALG_H

#define VEC(N, ...) typedef union v ## N { \
        float s[N];                        \
        struct { __VA_ARGS__ };            \
} v ## N

#define COMP_2 float x; \
               float y;
#define COMP_3 COMP_2 float z;
#define COMP_4 COMP_3 float w;

VEC(2, COMP_2);
VEC(3, COMP_3);
VEC(4, COMP_4);
#undef VEC

#endif
