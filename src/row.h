#pragma once

#include <string>

namespace row {

typedef struct erow
{
  std::size_t idx;
  std::size_t size;
  std::size_t rsize;
  std::string chars{};
  std::string render{};
  unsigned char *hl;
  int hl_open_comment;
} erow;

std::size_t CxToRx(const erow &row, const std::size_t cx);
std::size_t RxToCx(const erow &row, const std::size_t rx);
void Update(erow &row);
void Insert(const int at, const std::string &s);
void Del(const int at);
void InsertChar(erow &row, const int at, const char c);
void AppendString(erow &row, const std::string &s);
void DelChar(erow &row, const int at);

} // end namespace row
