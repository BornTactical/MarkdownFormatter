#include "MarkdownFormatter.h"

namespace Upp {
	void MarkdownFormatter::Parse(const pegtl::parse_tree::node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
		auto ParseChildren = ([&] {
			if (!node.children.empty()) {
				for (auto& up : node.children) {
					Parse(*up, richText, para, charFmt);
				}
			}
		});
		
		auto SimulateEndline = ([&] {
			if(!para.IsEmpty()) {
				richText.Cat(para);
				para = RichPara();
			}
		});
		
		if(node.is_root()) {
			ParseChildren();
		}
		
		if(node.has_content()) {
			if(node.is<MarkdownParser::Str>()) {
				std::string str = node.string();
				String s(str);
				para.Cat(s, charFmt);
			}
			else if(node.is<MarkdownParser::SpecialChar>()) {
				std::string str = node.string();
				String s(str);
				para.Cat(s, charFmt);
			}
			else if(node.is<MarkdownParser::Italics>()) {
				charFmt.Italic(true);
				ParseChildren();
				charFmt.Italic(false);
			}
			else if(node.is<MarkdownParser::Mention>()) {
				auto fmt = charFmt;
				fmt.ink  = Color(93, 126, 218);
				fmt.Bold();
				String s(node.string());
				para.Cat(s, fmt);
			}
			else if(node.is<MarkdownParser::Bold>()) {
				charFmt.Bold(true);
				ParseChildren();
				charFmt.Bold(false);
			}
			else if(node.is<MarkdownParser::Strike>()) {
				charFmt.Strikeout(true);
				ParseChildren();
				charFmt.Strikeout(false);
			}
			else if(node.is<MarkdownParser::Code>()) {
				SimulateEndline();
				auto fmt = charFmt;
				
				(Font&)charFmt = Monospace(13);
				RichPara code;
				RichTable table;
				table.AddColumn(1);
				
				auto tableFormat = table.GetFormat();
				     tableFormat.gridcolor = Black();
					 tableFormat.frame = 0;
					 tableFormat.framecolor = Color(0,0,0);
					 tableFormat.grid = 0;
					 tableFormat.lm = 60;
					 tableFormat.rm = 60;
					 table.SetFormat(tableFormat);
				
				RichText cellText;
				
				for (auto& up : node.children) {
					if(up->is<MarkdownParser::CodeEol>()) {
						RichPara::Format format = code.format;
						code = RichPara();
						code.format = format;
					}
					else if(up->is<MarkdownParser::Str>()) {
						std::string str = up->string();
						String s(str);
						code.Cat(s, charFmt);
						cellText.Cat(code);
					}
					else if(up->is<MarkdownParser::EndOfCode>()) {
						table.SetPick(0, 0, pick(cellText));
						RichCell::Format cellFmt = table.GetFormat(0, 0);
						cellFmt.bordercolor = Color(40, 40, 40);
						cellFmt.border = Rect{2,2,2,2};
						cellFmt.margin = Rect{10,10,10,10};
						table.SetFormat(0, 0, cellFmt);
						
						richText.CatPick(pick(table));
					}
				}
				
				charFmt = fmt;
			}
			else if(node.is<MarkdownParser::EndOfLine>()) {
				SimulateEndline();
			}
			else if(node.is<MarkdownParser::Url>()) {
				auto fmt = charFmt;
				fmt.Underline(true);
				fmt.ink = Blue();
				fmt.link = node.string();
				para.Cat(fmt.link, fmt);
			}
		}
	}
	
	RichText MarkdownFormatter::FormatMarkdown(String str, RichPara::CharFormat& charFmt) {
		RichText richText;
		RichPara para;
		pegtl::string_input in(str.ToStd(), "content");
		auto root = pegtl::parse_tree::parse<MarkdownParser::Grammar, MarkdownParser::Selector>(in);
		Parse(*root, richText, para, charFmt);
		richText.Cat(para);
		
		return richText;
	}
}