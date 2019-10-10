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
    class MarkdownFormatter {
        void Parse(const pegtl::parse_tree::node& node, RichText& richText, RichPara& para, RichPara::CharFormat& charFmt);
        
    public:
        RichText FormatMarkdown(String str, RichPara::CharFormat& charFmt);
    };
}

#endif
