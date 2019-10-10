# MarkdownFormatter
Simple markdown formatter for Rich control on U++

# Dependencies
Requires Ultimate++: https://www.ultimatepp.org/
Requires PEGTL: https://github.com/taocpp/PEGTL

# Usage:
```C++
#include <CtrlLib/CtrlLib.h>
#include <MarkdownFormatter/MarkdownFormatter.h>

using namespace Upp;

GUI_APP_MAIN {
    TopWindow wnd;
    
    RichTextView richTextView;
    richTextView.SetZoom(Zoom(1, 1));
    
    MarkdownFormatter mdf;
    
    RichPara::CharFormat fmt;
    fmt.paper = White();
    fmt.ink   = Black();
    (Font&)fmt = Arial(15);
    
    auto txt = mdf.FormatMarkdown(
        "Normal\n"
        "*Italics*\n"
        "**Bold**\n"
        "~~Strikethrough~~\n"
        "***BoldItalics***\n"
        "http://link\n"
        "```This is a sample code block\nLine #2```\n"
        "Done", fmt);
    
    richTextView.Pick(pick(txt));
    
    wnd.Add(richTextView.SizePos());
    wnd.Title("Markdown test").Sizeable().Run();
}


```
