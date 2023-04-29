#pragma once

#include "edit.h"
#include <string>

namespace row {

std::size_t CxToRx(const edit::erow &, const std::size_t);
std::size_t RxToCx(const edit::erow &, const std::size_t);
void Update(edit::erow &);
void InsertChar(edit::erow &, const int, const char);
void AppendString(edit::erow &, const std::string &);
void DelChar(edit::erow &, const int);

}// end namespace row
