#include "parse.h"

#include "string_util.h"

#include <charconv>
#include <format>
#include <iostream>

// TODO: The parsing functions should return an invalid parser on critical errors so that all parsing is cancelled

struct Parser
{
    // TODO: operator new(size_t, Parser &) and then allocate all AST nodes with the memory pool
    explicit Parser(const Lexer &lexer)
        : lexer{lexer}
    {
    }

    Lexer lexer;
    const char *error_context{};

    void arm(const char *context) { this->error_context = context; }

    [[nodiscard]] Parser quiet()
    {
        auto result          = *this;
        result.error_context = nullptr;
        return result;
    }

    void error(const Parser &start, std::string_view error) const
    {
        if (this->error_context == nullptr)
        {
            return;
        }

        auto current_token = start.peek_token();

        auto line = extract_line(start.lexer.source, current_token.pos.at);

        auto message = std::format(
            "Parser error at {}:{}\n{}\n{}",
            start.lexer.cursor.line,  // TODO: This should not be start, right? Remove start?
            start.lexer.cursor.line_offset,
            line,
            error);

        std::cout << message << std::endl;
    }

    Token peek_token() const { return this->lexer.peek_token(); }

    Token next_token()
    {
        auto token = this->lexer.next_token();
        while (token.type == Tt::single_line_comment || token.type == Tt::multi_line_comment)
        {
            token = this->lexer.next_token();
        }

        return token;
    }

    Parser parse_token(TokenType type, Token *out_token) const
    {
        auto start = *this;
        auto p     = *this;

        auto token = p.next_token();
        if (token.type != type)
        {
            this->error(start, std::format("Expected {}, received {}", to_string(type), token.to_string()));
            return start;
        }

        if (out_token != nullptr)
        {
            *out_token = token;
        }

        return p;
    }

    Parser parse_token(TokenType type) const { return this->parse_token(type, nullptr); }

    Parser parse_keyword(std::string_view keyword) const
    {
        auto start = *this;
        auto p     = *this;

        auto token = p.next_token();
        if (token.type != Tt::keyword)
        {
            this->error(start, std::format("Expected keyword {}, received {}", keyword, token.to_string()));
            return start;
        }

        auto len = keyword.size();
        if (token.length < len)
        {
            len = token.length;
        }
        if (strncmp(token.pos.at, keyword.data(), len) != 0)
        {
            p.error(start, std::format("Expected keyword {}, received {}", keyword, token.to_string()));
            return start;
        }

        return p;
    }

    bool advance(const Parser &parsed)
    {
        if (parsed.lexer.cursor.at <= this->lexer.cursor.at)
        {
            return false;
        }

        this->lexer = parsed.lexer;

        return true;
    }

    bool operator>>=(const Parser &other) { return this->advance(other); }
};

Parser parse_expr(Parser p, AstNode *&out_expr);
Parser parse_decl(Parser p, AstDeclaration &out_decl);
Parser parse_block(Parser p, AstBlock &out_block, bool allow_raw_statement);
Parser parse_type(Parser p, AstNode *&out_type);
Parser parse_compiler_error_block(Parser p, AstBlock &out_block);

Parser parse_statement(Parser p, AstNode *&out_statement)
{
    auto start = p;

    AstDeclaration decl{};
    if (p >>= parse_decl(p.quiet(), decl))
    {
        out_statement = new AstDeclaration{std::move(decl)};
        return p;
    }

    if (p >>= p.quiet().parse_keyword("if"))
    {
        p.arm("parsing if statement");

        AstIfStatement yf{};

        if (!(p >>= parse_expr(p, yf.condition)))
        {
            return start;
        }

        AstBlock then_block{};
        if (!(p >>= parse_block(p, then_block, true)))
        {
            return start;
        }

        yf.then_block = new AstBlock{std::move(then_block)};

        if (p >>= p.quiet().parse_keyword("else"))
        {
            // TODO: Re-arm?
            AstBlock else_block{};
            if (!(p >>= parse_block(p, else_block, true)))
            {
                return start;
            }

            yf.else_block = new AstBlock{std::move(else_block)};
        }

        out_statement = new AstIfStatement{std::move(yf)};
        return p;
    }

    if (p >>= p.quiet().parse_keyword("while"))
    {
        p.arm("parsing while loop");

        AstWhileLoop whyle{};

        if (!(p >>= parse_expr(p, whyle.condition)))
        {
            return start;
        }

        AstBlock block{};
        if (!(p >>= parse_block(p, block, true)))
        {
            return start;
        }

        whyle.block = new AstBlock{std::move(block)};

        out_statement = new AstWhileLoop{std::move(whyle)};
        return p;
    }

    if (p >>= p.quiet().parse_keyword("for"))
    {
        p.arm("parsing for loop");

        AstForLoop foa{};

        if (!(p >>= p.parse_token(Tt::identifier, &foa.identifier)))
        {
            return start;
        }

        if (!(p >>= p.quiet().parse_token(Tt::colon)))
        {
            if (!(p >>= parse_expr(p, foa.range_begin)))
            {
                return start;
            }
        }

        if (!(p >>= p.parse_token(Tt::colon)))
        {
            return start;
        }

        foa.comparison_operator = p.next_token().type;
        int64_t default_step{};

        switch (foa.comparison_operator)
        {
            case Tt::less_than:             break;
            case Tt::less_than_or_equal:    break;
            case Tt::greater_than:          break;
            case Tt::greater_than_or_equal: break;
            default:
            {
                p.error(start, "Invalid range operator");
                return start;
            }
        }

        if (!(p >>= parse_expr(p, foa.range_end)))
        {
            return start;
        }

        if (p >>= p.quiet().parse_token(Tt::colon))
        {
            if (!(p >>= parse_expr(p, foa.step)))
            {
                return start;
            }
        }

        AstBlock block{};
        if (!(p >>= parse_block(p, block, true)))
        {
            return start;
        }

        foa.block = new AstBlock{std::move(block)};

        out_statement = new AstForLoop{std::move(foa)};
        return p;
    }

    if (p >>= p.quiet().parse_keyword("break"))
    {
        out_statement = new AstBreakStatement{};
        return p;
    }

    if (p >>= p.quiet().parse_keyword("continue"))
    {
        out_statement = new AstContinueStatement{};
        return p;
    }

    if (p >>= p.quiet().parse_keyword("return"))
    {
        p.arm("parsing return statement");

        AstReturnStatement retyrn{};
        p >>= parse_expr(p.quiet(), retyrn.expression);  // Empty return for void

        out_statement = new AstReturnStatement{std::move(retyrn)};
        return p;
    }

    if (p >>= p.quiet().parse_token(Tt::colon))
    {
        p.arm("parsing label");

        Token identifier{};
        if (!(p >>= p.parse_token(Tt::identifier, &identifier)))
        {
            return start;
        }

        auto result        = new AstLabel{};
        result->identifier = identifier.text();
        out_statement      = result;
        return p;
    }

    if (p >>= p.quiet().parse_keyword("goto"))
    {
        p.arm("parsing goto statement");

        Token identifier{};
        if (!(p >>= p.parse_token(Tt::identifier, &identifier)))
        {
            return start;
        }

        auto result              = new AstGotoStatement{};
        result->label_identifier = identifier.text();
        out_statement            = result;
        return p;
    }

    AstBlock block{};
    if (p >>= parse_block(p.quiet(), block, false))
    {
        out_statement = new AstBlock{std::move(block)};
        return p;
    }

    if (p >>= parse_compiler_error_block(p.quiet(), block))
    {
        out_statement = new AstBlock{std::move(block)};
        return p;
    }

    AstNode *expr{};
    if (p >>= parse_expr(p.quiet(), expr))
    {
        out_statement = expr;
        return p;
    }

    p.error(start, "Failed to parse statement");

    return start;
}

Parser parse_block(Parser p, AstBlock &out_block, bool allow_raw_statement)
{
    auto start = p;

    if (!(p >>= p.quiet().parse_token(Tt::brace_open)))
    {
        if (allow_raw_statement)
        {
            AstNode *statement{};
            if (p >>= parse_statement(p, statement))
            {
                out_block.statements.push_back(statement);
                return p;
            }
        }

        return start;
    }

    p.arm("parsing block");

    while (true)
    {
        if (p >>= p.quiet().parse_token(Tt::brace_close))
        {
            return p;
        }

        AstNode *statement{};
        if (!(p >>= parse_statement(p, statement)))
        {
            return start;
        }

        out_block.statements.push_back(statement);
    }

    return p;
}

Parser parse_compiler_error_block(Parser p, AstBlock &out_block)
{
    auto start = p;

    Token token{};
    if (!(p >>= p.quiet().parse_token(Tt::identifier, &token)) || token.text() != "__error")
    {
        return start;
    }

    p.arm("parsing compiler error block");

    if (!(p >>= p.quiet().parse_token(Tt::parenthesis_open)))
    {
        return start;
    }

    Token mode_token{};
    if (!(p >>= p.quiet().parse_token(Tt::string_literal, &mode_token)))
    {
        return start;
    }

    assert(mode_token.text().size() >= 2);
    auto mode = mode_token.text().substr(1, mode_token.text().size() - 2);

    if (mode == "declaration")
    {
        out_block.expected_compiler_error_kind = AstBlock::CompilerErrorKind::declaration;
    }
    else if (mode == "typecheck")
    {
        out_block.expected_compiler_error_kind = AstBlock::CompilerErrorKind::typecheck;
    }
    else
    {
        p.error(start, std::format("Expected the mode to be either 'declaration' or 'typecheck' , got '{}'", mode));
        return start;
    }

    if (!(p >>= p.quiet().parse_token(Tt::parenthesis_close)))
    {
        return start;
    }

    if (!(p >>= parse_block(p, out_block, false)))
    {
        return start;
    }

    return p;
}

Parser parse_proc_signature(Parser p, AstProcedureSignature &out_signature)
{
    auto start = p;

    if (!(p >>= p.quiet().parse_keyword("proc")))
    {
        return start;
    }

    p.arm("parsing procedure signature");

    if (!(p >>= p.parse_token(Tt::parenthesis_open)))
    {
        return start;
    }

    auto require_close = false;
    while (true)
    {
        if (require_close)
        {
            if (!(p >>= p.parse_token(Tt::parenthesis_close)))
            {
                return start;
            }

            break;
        }
        else if (p >>= p.quiet().parse_token(Tt::parenthesis_close))
        {
            break;
        }

        if (p >>= p.quiet().parse_token(Tt::triple_dot))
        {
            out_signature.is_vararg = true;
            require_close           = true;
            continue;
        }

        AstDeclaration arg{};
        if (!(p >>= parse_decl(p, arg)))
        {
            return start;
        }

        arg.is_procedure_argument = true;
        out_signature.arguments.push_back(new AstDeclaration{std::move(arg)});

        if (!(p >>= p.quiet().parse_token(Tt::comma)))
        {
            require_close = true;
        }
    }

    // NOTE: Could be optional?
    if (!(p >>= parse_type(p, out_signature.return_type)))
    {
        p.error(start, "Failed to parse return type");
        return start;
    }

    return p;
}

Parser parse_proc(Parser p, AstProcedure &out_proc)
{
    auto start = p;

    AstProcedureSignature signature{};
    if (!(p >>= parse_proc_signature(p.quiet(), signature)))
    {
        return start;
    }

    out_proc.signature = new AstProcedureSignature{std::move(signature)};

    p.arm("parsing procedure");

    if (p >>= p.quiet().parse_keyword("external"))
    {
        out_proc.is_external = true;
        return p;
    }

    AstBlock body{};
    if (!(p >>= parse_block(p, body, true)))  // TODO: allow_raw_statement?...
    {
        return start;
    }

    out_proc.body = new AstBlock{std::move(body)};

    return p;
}

Parser parse_primary_expr(Parser p, AstNode *&out_primary_expr)
{
    auto start = p;

    Token token{};
    if (p >>= p.quiet().parse_token(Tt::number_literal, &token))
    {
        assert(token.length > 0);

        p.arm("parsing number literal");

        AstLiteral literal{};
        literal.token = token;

        auto begin  = token.pos.at;
        auto end    = token.pos.at + token.length;
        auto base   = 10;
        auto suffix = '\0';
        std::from_chars_result result{};

        auto is_hex = token.pos.at[0] == '0' && token.pos.at[1] == 'x';
        if (is_hex)
        {
            begin += 2;
            base = 16;
        }

        if (end[-1] == 'u' || (is_hex == false && end[-1] == 'f'))
        {
            suffix = end[-1];
            --end;
        }

        auto has_point = token.text().find('.') != std::string_view::npos;
        if (suffix == 'f')
        {
            // TODO: This does not support scientific float notation.
            // std::from_chars for floating point numbers is not implemented
            // in the current version of clang++ that is installable on Debian.
            char *parse_end;
            auto value = strtof(token.pos.at, &parse_end);

            if (std::isnan(value))
            {
                p.error(start, "Failed to parse float literal");
                return start;
            }

            if (parse_end != end)
            {
                p.error(start, "Failed to parse float literal");
                return start;
            }

            literal.value.emplace<float>(value);
        }
        else if (has_point)
        {
            // TODO: This does not support scientific float notation.
            // std::from_chars for floating point numbers is not implemented
            // in the current version of clang++ that is installable on Debian.
            char *parse_end;
            auto value = strtod(token.pos.at, &parse_end);

            if (std::isnan(value))
            {
                p.error(start, "Failed to parse double literal");
                return start;
            }

            if (parse_end != end)
            {
                p.error(start, "Failed to parse double literal");
                return start;
            }

            literal.value.emplace<double>(value);
        }
        else
        {
            uint64_t value;
            result = std::from_chars(begin, end, value, base);

            if (result.ec != std::errc{})
            {
                p.error(start, "Failed to parse integer literal");
                return start;
            }

            if (result.ptr != end)
            {
                p.error(start, "Failed to parse integer literal");
                return start;
            }

            literal.value.emplace<uint64_t>(value);
        }

        literal.suffix = suffix;

        out_primary_expr = new AstLiteral{std::move(literal)};
        return p;
    }

    if (p >>= p.quiet().parse_token(Tt::string_literal, &token))
    {
        assert(token.length >= 2);

        p.arm("parsing string literal");

        AstLiteral literal{};
        literal.token = token;

        // The raw string literal is always going to be at most as long as the encoded string literal
        std::vector<char> escaped_string;
        escaped_string.reserve(token.text().size());

        for (auto at = token.pos.at + 1; at < token.pos.at + token.length - 1; ++at)
        {
            if (*at == '\\')
            {
                auto next = at[1];

                switch (next)
                {
                    case 'a':  escaped_string.push_back('\a'); break;
                    case 'b':  escaped_string.push_back('\b'); break;
                    case 'e':  escaped_string.push_back('\e'); break;
                    case 'f':  escaped_string.push_back('\f'); break;
                    case 'n':  escaped_string.push_back('\n'); break;
                    case 'r':  escaped_string.push_back('\r'); break;
                    case 't':  escaped_string.push_back('\t'); break;
                    case 'v':  escaped_string.push_back('\v'); break;
                    case '0':  escaped_string.push_back('\0'); break;
                    case '"':  escaped_string.push_back('"'); break;
                    case '\\': escaped_string.push_back('\\'); break;
                    default:   p.error(start, "Invalid escape sequence"); return start;
                }

                ++at;
            }
            else
            {
                escaped_string.push_back(*at);
            }
        }

        escaped_string.push_back('\0');

        literal.value.emplace<std::string>(escaped_string.data());

        out_primary_expr = new AstLiteral{std::move(literal)};
        return p;
    }

    if (p >>= p.quiet().parse_token(Tt::identifier, &token))
    {
        auto ident        = new AstIdentifier{};
        ident->identifier = token;
        out_primary_expr  = ident;
        return p;
    }

    if (p >>= p.quiet().parse_keyword("true"))
    {
        auto literal   = new AstLiteral{};
        literal->token = token;
        literal->value.emplace<bool>(true);

        out_primary_expr = literal;
        return p;
    }
    else if (p >>= p.quiet().parse_keyword("false"))
    {
        auto literal   = new AstLiteral{};
        literal->token = token;
        literal->value.emplace<bool>(false);

        out_primary_expr = literal;
        return p;
    }

    if (p >>= p.quiet().parse_token(Tt::parenthesis_open))
    {
        p.arm("parsing parenthesis expression");

        AstNode *expr{};
        if (!(p >>= parse_expr(p, expr)))
        {
            return start;
        }

        if (!(p >>= p.parse_token(Tt::parenthesis_close)))
        {
            return start;
        }

        out_primary_expr = expr;
        return p;
    }

    AstProcedure proc{};
    if (p >>= parse_proc(p.quiet(), proc))
    {
        out_primary_expr = new AstProcedure{std::move(proc)};
        return p;
    }

    p.error(start, "Failed to parse primary expression");

    return start;
}

Parser parse_expression_suffix(Parser p, AstNode *lhs, AstNode **node)
{
    auto start = p;

    if (p >>= p.quiet().parse_token(Tt::parenthesis_open))
    {
        p.arm("parsing procedure call");

        AstProcedureCall call{};
        call.procedure = lhs;

        auto require_close = false;
        while (true)
        {
            if (require_close)
            {
                if (!(p >>= p.parse_token(Tt::parenthesis_close)))
                {
                    return start;
                }

                break;
            }
            else if (p >>= p.quiet().parse_token(Tt::parenthesis_close))
            {
                break;
            }

            AstNode *argument{};
            if (!(p >>= parse_expr(p, argument)))
            {
                return start;
            }

            call.arguments.push_back(argument);

            if (!(p >>= p.quiet().parse_token(Tt::comma)))
            {
                require_close = true;
            }
        }

        *node = new AstProcedureCall{std::move(call)};
        return p;
    }

    p.error(start, "Failed to parse suffix expression");

    return start;
}

Parser parse_binary_expr(Parser p, AstNode *&out_node, int prev_prec)
{
    auto start = p;

    AstNode *lhs;
    if (!(p >>= parse_primary_expr(p, lhs)))
    {
        return start;
    }
    p >>= parse_expression_suffix(p.quiet(), lhs, &lhs);

    while (true)
    {
        auto op   = p.peek_token();
        auto prec = binary_operator_precedence(op.type);

        if (prec == 0 || prec <= prev_prec)
        {
            out_node = lhs;
            return p;
        }

        p.next_token();
        p.arm("parsing binary operator right hand side");

        AstNode *rhs;
        if (!(p >>= parse_binary_expr(p, rhs, prec)))
        {
            return start;
        }

        auto bin_op           = new AstBinaryOperator{};
        bin_op->operator_type = op.type;
        bin_op->lhs           = lhs;
        bin_op->rhs           = rhs;

        lhs = bin_op;
    }

    out_node = lhs;

    return p;
}

Parser parse_expr(Parser p, AstNode *&node)
{
    return parse_binary_expr(p, node, -1);
}

Parser parse_type(Parser p, AstNode *&out_type)
{
    auto start = p;

    Token identifier{};
    if (p >>= p.quiet().parse_token(Tt::identifier, &identifier))
    {
        auto type        = new AstTypeIdentifier{};
        type->identifier = identifier;

        out_type = type;

        return p;
    }

    if (p >>= p.quiet().parse_token(Tt::asterisk))
    {
        AstPointerType type{};
        if (!(p >>= parse_type(p, type.target_type)))
        {
            return start;
        }

        out_type = new AstPointerType{std::move(type)};

        return p;
    }

    if (p >>= p.quiet().parse_token(Tt::bracket_open))
    {
        AstArrayType type{};

        // TODO: Could be empty or '..'
        if (!(p >>= parse_expr(p, type.length_expression)))
        {
            return start;
        }

        if (!(p >>= p.parse_token(Tt::bracket_close)))
        {
            return start;
        }

        if (!(p >>= parse_type(p, type.element_type)))
        {
            return start;
        }

        out_type = new AstArrayType{std::move(type)};

        return p;
    }

    AstProcedureSignature signature{};
    if (p >>= parse_proc_signature(p.quiet(), signature))
    {
        out_type = new AstProcedureSignature{std::move(signature)};
        return p;
    }

    p.error(start, "Failed to parse type");

    return start;
}

Parser parse_decl(Parser p, AstDeclaration &out_decl)
{
    auto start = p;

    if (!(p >>= p.parse_token(Tt::identifier, &out_decl.identifier)))
    {
        return start;
    }

    if (!(p >>= p.parse_token(Tt::colon)))
    {
        return start;
    }

    p.arm("parsing declaration");

    auto has_type = p >>= parse_type(p.quiet(), out_decl.type);

    if (p >>= p.quiet().parse_token(Tt::assign))
    {
        if (!(p >>= parse_expr(p, out_decl.init_expression)))
        {
            return start;
        }
    }
    else if (has_type == false)
    {
        p.error(start, "Declaration without init expression needs a type");
    }

    return p;
}

Parser parse_module(Parser p, AstModule &out_module)
{
    auto start = p;

    p.arm("parsing module");

    out_module.block = new AstBlock{};

    while (true)
    {
        AstDeclaration decl{};
        if (!(p >>= parse_decl(p, decl)))
        {
            return start;
        }

        out_module.block->statements.push_back(new AstDeclaration{decl});

        if (p.peek_token().type == Tt::eof)
        {
            return p;
        }
    }

    return p;
}

AstModule *parse_module(std::string_view source)
{
    Lexer lexer{source};
    Parser p{lexer};

    AstModule module{};
    if (!(p >>= parse_module(p, module)))
    {
        std::cout << "Compilation failed" << std::endl;

        return nullptr;
    }

    return new AstModule{std::move(module)};
}
