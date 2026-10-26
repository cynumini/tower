#include "font.hpp"

Font createDefaultFont(FRect texture) {
    return {.widths = DefaultFontWidths, .texture = texture};
}
