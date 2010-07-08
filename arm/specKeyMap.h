#include "types.h"

struct CMatrixRecord
{
    word keyCode;
    byte specKeyCode0;
    byte specKeyCode1;
};

extern CMatrixRecord const keyMatrixMain[];

extern CMatrixRecord const keyMatrixKempston[];
extern CMatrixRecord const keyMatrixSinclair1[];
extern CMatrixRecord const keyMatrixSinclair2[];
extern CMatrixRecord const keyMatrixCursor[];
extern CMatrixRecord const keyMatrixQaopm[];

enum { SPEC_KEY_NONE = 0xff,
        SPEC_KEY_CAPS_SHIFT = 0, SPEC_KEY_Z, SPEC_KEY_X, SPEC_KEY_C, SPEC_KEY_V,
        SPEC_KEY_A, SPEC_KEY_S, SPEC_KEY_D, SPEC_KEY_F, SPEC_KEY_G,
        SPEC_KEY_Q, SPEC_KEY_W, SPEC_KEY_E, SPEC_KEY_R, SPEC_KEY_T,
        SPEC_KEY_1, SPEC_KEY_2, SPEC_KEY_3, SPEC_KEY_4, SPEC_KEY_5,
        SPEC_KEY_0, SPEC_KEY_9, SPEC_KEY_8, SPEC_KEY_7, SPEC_KEY_6,
        SPEC_KEY_P, SPEC_KEY_O, SPEC_KEY_I, SPEC_KEY_U, SPEC_KEY_Y,
        SPEC_KEY_ENTER, SPEC_KEY_L, SPEC_KEY_K, SPEC_KEY_J, SPEC_KEY_H,
        SPEC_KEY_SPACE, SPEC_KEY_SYMBOL_SHIFT, SPEC_KEY_M, SPEC_KEY_N, SPEC_KEY_B,
        SPEC_KEY_RIGHT, SPEC_KEY_LEFT, SPEC_KEY_DOWN, SPEC_KEY_UP, SPEC_KEY_FIRE,
        SPEC_KEYS_NUMBER };


