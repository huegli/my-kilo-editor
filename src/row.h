#pragma once

#include <string>

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

std::size_t editorRowCxToRx(const erow &row, const std::size_t cx);
std::size_t editorRowRxToCx(const erow &row, const std::size_t rx);
void editorUpdateRow(erow &row);
void editorInsertRow(const int at, const std::string &s);
void editorDelRow(const int at);
void editorRowInsertChar(erow &row, const int at, const char c);
void editorRowAppendString(erow &row, const std::string &s);
void editorRowDelChar(erow &row, const int at);
