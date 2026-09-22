#pragma once

#include "abraflexitui/TV.h"

namespace abraflexitui {

enum class EditorMode { Json, Xml };

// TMemo whose visible text is colored as JSON or XML.
// TEditor::formatLine is not virtual, so draw() paints the lines itself.
class SyntaxMemo : public TMemo {
public:
    SyntaxMemo(const TRect &bounds, TScrollBar *hScrollBar, TScrollBar *vScrollBar, TIndicator *indicator,
               ushort bufSize) noexcept;

    void setMode(EditorMode mode);
    void draw() override;

private:
    EditorMode mode_ = EditorMode::Json;
};

} // namespace abraflexitui
