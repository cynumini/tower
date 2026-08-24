#pragma once

namespace unagi {
enum class Result { ongoing, success, failure };

Result init();
Result update();
void draw();
void quit();
} // namespace unagi
