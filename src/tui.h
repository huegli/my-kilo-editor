#pragma once

#include "edit.h"
#include "terminal.h"
#include <string>

namespace tui {

void Save(edit::editorConfig &, const Term::Terminal &term);
void Find(edit::editorConfig &, const Term::Terminal &term);

void DrawRows(edit::editorConfig &, std::string &);
void DrawStatusBar(edit::editorConfig &, std::string &);
void DrawMessageBar(edit::editorConfig &, std::string &);
void RefreshScreen(edit::editorConfig &, std::string &);
void SetStatusMessage(edit::editorConfig &);
void SetStatusMessage(edit::editorConfig &, const char *msg);

char *Prompt(edit::editorConfig &,
  const Term::Terminal &term,
  const char *prompt1,
  const char *prompt2,
  void (*callback)(edit::editorConfig &, char *, int));
void MoveCursor(edit::editorConfig &, int key);
bool ProcessKeypress(edit::editorConfig &, const Term::Terminal &term);
void init(edit::editorConfig &, const Term::Terminal &term);

}// end namespace tui
