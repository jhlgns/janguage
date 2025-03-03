#include "parse.h"
#include <catch2/catch_test_macros.hpp>

namespace assertions
{
    template<typename T>
    using Assertion = std::function<void(T)>;

    struct Nop
    {
        void operator()(auto) { }
    };

    struct Null
    {
        void operator()(AstNode *node) { REQUIRE(node == nullptr); }
    };

    struct Token
    {
        explicit Token(std::string_view text)
            : text{text}
        {
        }

        explicit Token(TokenType type)
            : type{type}
        {
        }

        std::optional<std::string_view> text;
        std::optional<TokenType> type;

        void operator()(const ::Token &token)
        {
            if (this->type.has_value())
            {
                REQUIRE(token.type == this->type.value());
            }

            if (this->text.has_value())
            {
                REQUIRE(token.text() == this->text.value());
            }
        }

        void operator()(TokenType type) { REQUIRE(type == this->type.value()); }
    };

    struct BinaryOperator
    {
        Assertion<TokenType> type;
        Assertion<AstNode *> lhs;
        Assertion<AstNode *> rhs;

        void operator()(AstNode *node)
        {
            auto bin_op = ast_cast<AstBinaryOperator>(node);
            REQUIRE(bin_op != nullptr);

            this->type(bin_op->type);
            this->lhs(bin_op->lhs);
            this->rhs(bin_op->rhs);
        }
    };

    struct Block
    {
        std::vector<Assertion<AstNode *>> statements;

        void operator()(AstNode *node)
        {
            auto block = ast_cast<AstBlock>(node);
            REQUIRE(block != nullptr);

            REQUIRE(block->statements.size() == this->statements.size());

            for (auto i = 0; i < block->statements.size(); ++i)
            {
                this->statements[i](block->statements[i]);
            }
        }
    };

    struct Declaration
    {
        std::string_view identifier;
        Assertion<AstNode *> type;
        Assertion<AstNode *> init_expression;

        void operator()(AstNode *node)
        {
            auto decl = ast_cast<AstDeclaration>(node);
            REQUIRE(decl != nullptr);

            REQUIRE(this->identifier == decl->identifier.text());
            this->type(decl->type);
            this->init_expression(decl->init_expression);
        }
    };

    struct Identifier
    {
        explicit Identifier(std::string_view identifier)
            : identifier{identifier}
        {
        }

        std::string_view identifier;

        void operator()(AstNode *node)
        {
            auto ident = ast_cast<AstIdentifier>(node);
            REQUIRE(ident != nullptr);

            REQUIRE(ident->identifier.text() == this->identifier);
        }
    };

    struct If
    {
        Assertion<AstNode *> condition;
        Assertion<AstBlock *> then_block;
        Assertion<AstBlock *> else_block;

        void operator()(AstNode *node)
        {
            auto yf = ast_cast<AstIf>(node);
            REQUIRE(yf != nullptr);

            this->condition(yf->condition);
            this->then_block(&yf->then_block);
            this->else_block(&yf->else_block);
        }
    };

    struct Literal
    {
        std::optional<Token> token;
        std::optional<LiteralType> type;
        std::optional<int64_t> int_value;

        void operator()(AstNode *node)
        {
            auto literal = ast_cast<AstLiteral>(node);
            REQUIRE(literal != nullptr);

            if (this->token.has_value())
            {
                this->token.value()(literal->token);
            }

            if (this->type.has_value())
            {
                REQUIRE(this->type.value() == literal->type);
            }

            if (this->int_value.has_value())
            {
                REQUIRE(this->int_value.value() == literal->int_value);
            }
        }
    };

    struct ProcedureSignature
    {
        std::vector<Assertion<AstDeclaration *>> arguments;

        void operator()(AstNode *node)
        {
            auto signature = ast_cast<AstProcedureSignature>(node);
            REQUIRE(signature != nullptr);

            REQUIRE(signature->arguments.size() == this->arguments.size());

            for (auto i = 0; i < signature->arguments.size(); ++i)
            {
                this->arguments[i](&signature->arguments[i]);
            }
        }
    };

    struct Procedure
    {
        Assertion<AstProcedureSignature *> signature;
        Assertion<AstBlock *> body;

        void operator()(AstNode *node)
        {
            auto proc = ast_cast<AstProcedure>(node);
            REQUIRE(proc != nullptr);

            this->signature(&proc->signature);
            this->body(&proc->body);
        }
    };

    struct ProcedureCall
    {
        Assertion<AstNode *> procedure;
        std::vector<Assertion<AstNode *>> arguments;

        void operator()(AstNode *node)
        {
            auto call = ast_cast<AstProcedureCall>(node);
            REQUIRE(call != nullptr);

            this->procedure(call->proc);

            REQUIRE(call->arguments.size() == this->arguments.size());

            for (auto i = 0; i < call->arguments.size(); ++i)
            {
                this->arguments[i](call->arguments[i]);
            }
        }
    };

    struct Return
    {
        Assertion<AstNode *> expression;

        void operator()(AstNode *node)
        {
            auto retyrn = ast_cast<AstReturn>(node);
            REQUIRE(retyrn != nullptr);

            this->expression(retyrn->expression);
        }
    };

    struct SimpleType
    {
        explicit SimpleType(std::string_view identifier)
            : identifier{identifier}
        {
        }

        std::string_view identifier;

        void operator()(AstNode *node)
        {
            auto type = ast_cast<AstSimpleType>(node);
            REQUIRE(type != nullptr);

            REQUIRE(this->identifier == type->identifier.text());
        }
    };
}  // namespace assertions

AstProcedure *parse_program_and_get_main(std::string_view source)
{
    AstProgram program{};
    REQUIRE(parse_program(source, &program));

    auto main_decl = program.block.find_declaration("main");
    REQUIRE(main_decl != nullptr);

    auto main = ast_cast<AstProcedure>(main_decl->init_expression);
    REQUIRE(main != nullptr);

    return main;
}

void test_integer_literal(std::string_view literal_text, int64_t expected_value)
{
    namespace as = assertions;

    AstProgram program;

    auto source = std::format("main := proc() {{ a := {} }}", literal_text);
    REQUIRE(parse_program(source, &program));

    auto main_decl = program.block.find_declaration("main");
    REQUIRE(main_decl != nullptr);

    as::Procedure{
        .signature = as::Nop{},

        .body = as::Block{
            .statements = {
                as::Declaration{
                    .identifier      = "a",
                    .type            = as::Null{},
                    .init_expression = as::Literal{.int_value = expected_value},
                },
            }}}(main_decl->init_expression);
}

TEST_CASE("Integer literals", "[parse]")
{
#define CASE(literal) test_integer_literal(#literal, literal);
    CASE(0);
    CASE(1);
    CASE(7);
    CASE(788);
    CASE(789237489234);
    CASE(9223372036854775807);  // int64_t max value

    CASE(0x0);
    CASE(0x00003);
    CASE(0xaaaaaaaaaa9900);
    CASE(0x0000000000000000);
    CASE(0x0000000000000001);
    CASE(0x7fffffffffffffff);
    CASE(0x78910fabc78eed81);
    CASE(0xAFFED00F);
    CASE(0xDec0dedFece5);
#undef CASE
}

TEST_CASE("Basic binary operators", "[parse]")
{
    namespace as = assertions;

    std::string_view source{R"(
main := proc() {
    a + b
    a - b
    a * b
    a / b
    a % b
    a >= b
}
)"};

    AstProgram program{};
    REQUIRE(parse_program(source, &program));

    auto main_decl = program.block.find_declaration("main");
    REQUIRE(main_decl != nullptr);

    auto main = ast_cast<AstProcedure>(main_decl->init_expression);
    REQUIRE(main != nullptr);

    as::Procedure{
        .signature = as::Nop{},

        .body = as::Block{
            .statements =
                {
                    as::BinaryOperator{
                        .type = as::Token{Tt::plus},
                        .lhs  = as::Identifier{"a"},
                        .rhs  = as::Identifier{"b"},
                    },
                    as::BinaryOperator{
                        .type = as::Token{Tt::minus},
                        .lhs  = as::Identifier{"a"},
                        .rhs  = as::Identifier{"b"},
                    },
                    as::BinaryOperator{
                        .type = as::Token{Tt::asterisk},
                        .lhs  = as::Identifier{"a"},
                        .rhs  = as::Identifier{"b"},
                    },
                    as::BinaryOperator{
                        .type = as::Token{Tt::slash},
                        .lhs  = as::Identifier{"a"},
                        .rhs  = as::Identifier{"b"},
                    },
                    as::BinaryOperator{
                        .type = as::Token{Tt::mod},
                        .lhs  = as::Identifier{"a"},
                        .rhs  = as::Identifier{"b"},
                    },
                    as::BinaryOperator{
                        .type = as::Token{Tt::greater_than_or_equal},
                        .lhs  = as::Identifier{"a"},
                        .rhs  = as::Identifier{"b"},
                    },
                },
        }}(main);
}

TEST_CASE("Binary operator precedence", "[parse]")
{
    namespace as = assertions;

    std::string_view source{R"(
main := proc() {
    a + b * c
    a * b + c
    1 * 2 < 3 & 4 | 5
    // 1 | 2 & 3 < 4 * 5
}
)"};

    as::Procedure{
        .signature = as::Nop{},

        .body =
            as::Block{
                .statements =
                    {
                        // a + b * c
                        as::BinaryOperator{
                            .type = as::Token{Tt::plus},
                            .lhs  = as::Identifier{"a"},
                            .rhs =
                                as::BinaryOperator{
                                    .type = as::Token{Tt::asterisk},
                                    .lhs  = as::Identifier{"b"},
                                    .rhs  = as::Identifier{"c"},
                                },
                        },

                        // a * b + c
                        as::BinaryOperator{
                            .type = as::Token{Tt::plus},
                            .lhs =
                                as::BinaryOperator{
                                    .type = as::Token{Tt::asterisk},
                                    .lhs  = as::Identifier{"a"},
                                    .rhs  = as::Identifier{"b"},
                                },
                            .rhs = as::Identifier{"c"},
                        },

                        // 1 * 2 < 3 & 4 | 5
                        as::BinaryOperator{
                            .type = as::Token{Tt::bit_or},
                            .lhs =
                                as::BinaryOperator{
                                    .type = as::Token{Tt::bit_and},
                                    .lhs =
                                        as::BinaryOperator{
                                            .type = as::Token{Tt::less_than},
                                            .lhs =
                                                as::BinaryOperator{
                                                    .type = as::Token{Tt::asterisk},
                                                    .lhs  = as::Literal{.int_value = 1},
                                                    .rhs  = as::Literal{.int_value = 2},
                                                },
                                            .rhs = as::Literal{.int_value = 3},
                                        },
                                    .rhs = as::Literal{.int_value = 4},
                                },
                            .rhs = as::Literal{.int_value = 5},
                        },

                    // 1 | 2 & 3 < 4 * 5
                    // as::BinaryOperator{
                    //     .type = as::Token{Tt::plus},
                    //     .lhs = ,
                    //     .rhs = ,
                    // },
                    },
            }}(parse_program_and_get_main(source));
}

TEST_CASE("Declaration", "[parse]")
{
    namespace as = assertions;

    std::string_view source{R"(
main := proc() {
    a: i64
    b: i64 = 2
    c := 3
    d := proc() {}
}
)"};

    as::Procedure{
        .signature = as::Nop{},

        .body =
            as::Block{
                .statements =
                    {
                        as::Declaration{
                            .identifier      = "a",
                            .type            = as::SimpleType{"i64"},
                            .init_expression = as::Null{},
                        },
                        as::Declaration{
                            .identifier      = "b",
                            .type            = as::SimpleType{"i64"},
                            .init_expression = as::Literal{.int_value = 2},
                        },
                        as::Declaration{
                            .identifier      = "c",
                            .type            = as::Null{},
                            .init_expression = as::Literal{.int_value = 3},
                        },
                        as::Declaration{
                            .identifier      = "d",
                            .type            = as::Null{},
                            .init_expression = as::Procedure{.signature = as::Nop{}, .body = as::Nop{}},
                        },
                    }},
    }(parse_program_and_get_main(source));
}

TEST_CASE("Program", "[parse]")
{
    std::string_view source{R"(
/*
Multiline
Comment

/* nested /* comments */
*/

*/

// Single line comment

f := proc(a: int, b: int) {
    if a == 1 {
        return a * b + 2 >> 2
    } else {
        return a / 400 * f(b | 2, a &0xff)
    }
}

main := proc() {
    x := f(3, 4)
}

)"};

    AstProgram program;
    REQUIRE(parse_program(source, &program));

    // TODO
    // std::cout << dump_node(0, &program) << std::endl;
}
