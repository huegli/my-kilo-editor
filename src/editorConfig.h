#pragma once

#include "row.h"

#include <string>
#include <vector>

struct editorConfig
{
  std::size_t cx, cy;
  std::size_t rx;
  std::size_t rowoff;
  std::size_t coloff;
  int screenrows;
  int screencols;
  std::size_t numrows;
  std::vector<row::erow> row{};
  int dirty;
  std::string filename{};
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
};

extern struct editorConfig E;


