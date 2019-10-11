#ifndef _MarkdownFormatter_MarkdownFormatter_h_
#define _MarkdownFormatter_MarkdownFormatter_h_

#include <Core/Core.h>
#include <RichText/RichText.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/uri.hpp>

namespace pegtl = tao::pegtl;

namespace MarkdownParser {
    using namespace pegtl;
    using namespace pegtl::uri;
    
    struct EndOfLine : eol {};
    struct EndOfPara : eof {};
    
    struct Para;
    struct Asterisk1   : one<'*'>   {};
    struct Asterisk2   : two<'*'>   {};
    struct Asterisk3   : three<'*'> {};
    struct Tilde2      : two<'~'>   {};
    struct Tick3       : three<'`'> {};
    struct Whitespace  : sor<one<' '>, one<'\t'>> {};
    
    struct Italics;
    struct Bold :
        seq<
            Asterisk2,
            plus<
                seq<not_at<Asterisk2>, Para>
            >,
            Asterisk2
        > {};
        
    struct Italics :
        seq<
            Asterisk1,
            not_at<Whitespace>,
            plus<
                sor<
                    seq<not_at<Asterisk1>, Para>,
                    Bold
                >
            >,
            not_at<Whitespace>,
            Asterisk1
        > {};
        
    struct Strike :
        seq<
            Tilde2,
            plus<
                seq<not_at<Tilde2>, Para>
            >,
            Tilde2
        > {};
        
    struct SpecialChar :
        sor<
            one<'~'>, one<'*'>, one<'_'>, one<'`'>
        > {};
        
    struct NormalChar :
        seq<
            not_at<sor<SpecialChar, eol>>,
            any
        > {};

    struct Str :
        seq<
            NormalChar,
            star< NormalChar >
        > {};
        
    struct Proto :
        seq<
            string<'h','t','t','p'>,
            opt<one<'s'>
            >,
            one<':'>,
            two<'/'>
        > {};
        
    struct Url :
        seq<
            at<Proto>,
            URI
        > {};
        
    struct Mention :
        seq<
            one<'<'>,
            one<'@'>,
            seq<plus<digit>>,
            one<'>'>
        > {};
    
    struct CodeEol : eol {};
    struct EndOfCode : Tick3 {};
    
    struct Code :
        seq<
            Tick3,
            opt<plus<eol>>,
            plus<
                seq<not_at<Tick3>, sor<Str, SpecialChar, CodeEol>>
            >,
            opt<plus<eol>>,
            EndOfCode
        > {};
    
    struct Para :
        sor<
            Code,
            Url,
            Mention,
            Str,
            Bold,
            Italics,
            Strike,
            EndOfLine,
            SpecialChar
        >{};
    
    struct Grammar : until<EndOfPara, Para> {};
    
    // empty default
    template<typename Rule>
    struct Action{};
    
    template< typename Rule >
    using Selector = parse_tree::selector <
        Rule,
        parse_tree::apply_store_content::to <
            Str, Code, Bold, Italics, Mention, Strike, EndOfLine,
            EndOfPara, SpecialChar, Url, CodeEol, EndOfCode>
    >;
    
    std::unique_ptr<pegtl::parse_tree::node> Parse(std::string content);
}

namespace Upp {
    using Node = const pegtl::parse_tree::node;

    template
        <typename StrPolicy,
         typename SpecialCharPolicy,
         typename ItalicsPolicy,
         typename MentionPolicy,
         typename BoldPolicy,
         typename StrikePolicy,
         typename CodePolicy,
         typename EolPolicy,
         typename UrlPolicy>
    class BasicMarkdownFormatter :
        private StrPolicy,     private SpecialCharPolicy, private ItalicsPolicy,
        private MentionPolicy, private BoldPolicy,        private StrikePolicy,
        private CodePolicy,    private EolPolicy,         private UrlPolicy {
        using StrPolicy::FormatStr;
        using SpecialCharPolicy::FormatSpecialChar;
        using ItalicsPolicy::FormatItalics;
        using MentionPolicy::FormatMention;
        using BoldPolicy::FormatBold;
        using StrikePolicy::FormatStrike;
        using CodePolicy::FormatCode;
        using EolPolicy::FormatEol;
        using UrlPolicy::FormatUrl;
        
    public:
        void Parse(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
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
                    FormatStr(node, richText, para, charFmt);
                }
                else if(node.is<MarkdownParser::SpecialChar>()) {
                    FormatSpecialChar(node, richText, para, charFmt);
                }
                else if(node.is<MarkdownParser::Italics>()) {
                    FormatItalics(node, richText, para, charFmt);
                    charFmt.Italic(true);
                    ParseChildren();
                    charFmt.Italic(false);
                }
                else if(node.is<MarkdownParser::Mention>()) {
                    FormatMention(node, richText, para, charFmt);
                }
                else if(node.is<MarkdownParser::Bold>()) {
                    FormatBold(node, richText, para, charFmt);
                    charFmt.Bold(true);
                    ParseChildren();
                    charFmt.Bold(false);
                }
                else if(node.is<MarkdownParser::Strike>()) {
                    FormatStrike(node, richText, para, charFmt);
                    charFmt.Strikeout(true);
                    ParseChildren();
                    charFmt.Strikeout(false);
                }
                else if(node.is<MarkdownParser::Code>()) {
                    SimulateEndline();
                    FormatCode(node, richText, para, charFmt);
                }
                else if(node.is<MarkdownParser::EndOfLine>()) {
                    SimulateEndline();
                }
                else if(node.is<MarkdownParser::Url>()) {
                    FormatUrl(node, richText, para, charFmt);
                }
            }
        }
        
        RichText Format(String str, RichPara::CharFormat& charFmt) {
            RichText richText;
            RichPara para;
            pegtl::string_input in(str.ToStd(), "content");
            auto root = pegtl::parse_tree::parse<MarkdownParser::Grammar, MarkdownParser::Selector>(in);
            Parse(*root, richText, para, charFmt);
            richText.Cat(para);
            
            return richText;
        }
    };
    
    class StrDefault {
    protected:
        void FormatStr(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            std::string str = node.string();
            String s(str);
            para.Cat(s, charFmt);
        }
    };
    
    class SpecialCharDefault {
    protected:
        void FormatSpecialChar(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            std::string str = node.string();
            String s(str);
            para.Cat(s, charFmt);
        }
    };
    
    class ItalicsDefault {
    protected:
        void FormatItalics(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            // nothing here
        }
    };
    
    class MentionDefault {
    protected:
        void FormatMention(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            auto fmt = charFmt;
            fmt.ink  = Color(93, 126, 218);
            fmt.Bold();
            String s(node.string());
            para.Cat(s, fmt);
        }
    };
    
    class BoldDefault {
    protected:
        void FormatBold(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            // nothing here
        }
    };
    
    class StrikeDefault {
    protected:
        void FormatStrike(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            // nothing here
        }
    };
    
    class CodeDefault {
    protected:
        void FormatCode(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {        
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
    };

    class EolDefault {
    protected:
        void FormatEol(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            // nothing here
        }
    };
    
    class UrlDefault {
    protected:
        void FormatUrl(Node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt) {
            auto fmt = charFmt;
                 fmt.Underline(true);
                 fmt.ink = Blue();
                 fmt.link = node.string();
                 
            para.Cat(fmt.link, fmt);
        }
    };
    
    typedef BasicMarkdownFormatter< StrDefault, SpecialCharDefault, ItalicsDefault,
        MentionDefault, BoldDefault, StrikeDefault, CodeDefault, EolDefault, UrlDefault>
        MarkdownFormatter;
}
#endif
