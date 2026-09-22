#pragma once

#include "abraflexitui/TV.h"

#include <string>

namespace abraflexitui {

// Pretty-print the editor buffer as JSON or XML and replace its text.
// Returns false and sets error when the text is not valid for that mode
// or does not fit in the editor.
bool formatEditorText(TEditor &editor, bool xml, std::string &error);

} // namespace abraflexitui
