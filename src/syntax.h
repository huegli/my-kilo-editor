#pragma once

#include "edit.h"

namespace syntax {

enum editorHighlight {
  HL_NORMAL = 0,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};


void Update(edit::editorConfig &, edit::erow &);
void SelectHighlight(edit::editorConfig &);

}// end namespace syntax
