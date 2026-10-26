#pragma once

#include <skn.cpp>
#include <skn/rect.cpp>

struct Font {
    static constexpr OldArray<char, 256> calcWidths() {
        OldArray<char, 256> r = {};
        r[' '] = 2, r['!'] = 1, r['"'] = 3, r['#'] = 6, r['$'] = 5, r['%'] = 8, r['&'] = 7,
        r['\''] = 1, r['('] = 2, r[')'] = 2, r['*'] = 5, r['+'] = 5, r[','] = 2, r['-'] = 4,
        r['.'] = 1, r['/'] = 4, r['0'] = 5, r['1'] = 3, r['2'] = 5, r['3'] = 4, r['4'] = 4,
        r['5'] = 4, r['6'] = 5, r['7'] = 4, r['8'] = 5, r['9'] = 5, r[':'] = 1, r[';'] = 2,
        r['<'] = 2, r['='] = 4, r['>'] = 4, r['?'] = 5, r['@'] = 8, r['A'] = 5, r['B'] = 4,
        r['C'] = 4, r['D'] = 5, r['E'] = 4, r['F'] = 4, r['G'] = 4, r['H'] = 4, r['I'] = 3,
        r['J'] = 4, r['K'] = 4, r['L'] = 4, r['M'] = 7, r['N'] = 5, r['O'] = 5, r['P'] = 5,
        r['Q'] = 5, r['R'] = 5, r['S'] = 5, r['T'] = 5, r['U'] = 4, r['V'] = 5, r['W'] = 9,
        r['X'] = 5, r['Y'] = 5, r['Z'] = 5, r['['] = 3, r['\\'] = 4, r[']'] = 3, r['^'] = 5,
        r['_'] = 4, r['`'] = 2, r['a'] = 4, r['b'] = 4, r['c'] = 4, r['d'] = 4, r['e'] = 4,
        r['f'] = 4, r['g'] = 4, r['h'] = 4, r['i'] = 1, r['j'] = 4, r['k'] = 4, r['l'] = 1,
        r['m'] = 5, r['n'] = 4, r['o'] = 4, r['p'] = 4, r['q'] = 4, r['r'] = 4, r['s'] = 4,
        r['t'] = 4, r['u'] = 4, r['v'] = 5, r['w'] = 5, r['x'] = 5, r['y'] = 5, r['z'] = 4,
        r['{'] = 4, r['|'] = 1, r['}'] = 4, r['~'] = 7;
        return r;
    }
    OldArray<char, 256> widths;
    f32 size = 10.0F;
    skn::Rect texture;

    constexpr void init(skn::Rect texture) {
        this->texture = texture;
        widths = calcWidths();
    }
};
