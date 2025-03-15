#include "catch2/catch_test_macros.hpp"
#include "test_utils.h"
#include "typecheck.h"

using Types = BuiltinTypes;

void test_type(AstNode *ast, const Node *expected_type)
{
    BlockNode block;

    auto node = make_node(&block, ast);
    REQUIRE(node != nullptr);

    if (expected_type == nullptr)
    {
        REQUIRE(typecheck(node) == false);
    }
    else
    {
        REQUIRE(typecheck(node));
        REQUIRE(types_equal(node->type, expected_type));
    }
}

TEST_CASE("Literals", "[typecheck]")
{
    test_type(make_int_literal(1), &Types::i64);
    test_type(make_int_literal(1, 'u'), &Types::u64);
    test_type(make_float_literal(1.0f), &Types::f32);
    test_type(make_double_literal(1.0), &Types::f64);
    test_type(make_bool_literal(false), &Types::boolean);
}

TEST_CASE("Binary operators", "[typecheck]")
{
    SECTION("Arithmetic")
    {
        test_type(make_binary_operator(Tt::plus, make_int_literal(1), make_int_literal(2)), &Types::i64);
        test_type(make_binary_operator(Tt::minus, make_int_literal(1), make_int_literal(2)), &Types::i64);
        test_type(make_binary_operator(Tt::asterisk, make_int_literal(1), make_int_literal(2)), &Types::i64);
        test_type(make_binary_operator(Tt::slash, make_int_literal(1), make_int_literal(2)), &Types::i64);
        test_type(make_binary_operator(Tt::mod, make_int_literal(1), make_int_literal(2)), &Types::i64);

        test_type(make_binary_operator(Tt::plus, make_float_literal(1), make_float_literal(2)), &Types::f32);
        test_type(make_binary_operator(Tt::minus, make_float_literal(1), make_float_literal(2)), &Types::f32);
        test_type(make_binary_operator(Tt::asterisk, make_float_literal(1), make_float_literal(2)), &Types::f32);
        test_type(make_binary_operator(Tt::slash, make_float_literal(1), make_float_literal(2)), &Types::f32);
        test_type(make_binary_operator(Tt::mod, make_float_literal(1), make_float_literal(2)), &Types::f32);

        test_type(make_binary_operator(Tt::plus, make_double_literal(1), make_double_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::minus, make_double_literal(1), make_double_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::asterisk, make_double_literal(1), make_double_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::slash, make_double_literal(1), make_double_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::mod, make_double_literal(1), make_double_literal(2)), &Types::f64);
    };

    SECTION("Type coercion")
    {
        // TODO: Integer literals are defaulted to i64, so the floating point result is always going to
        // be f64 - think of some way to test coercion of differnt type sizes (i32 + f32 -> f32...)

        test_type(make_binary_operator(Tt::plus, make_float_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::plus, make_int_literal(1), make_float_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::plus, make_double_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::plus, make_int_literal(1), make_double_literal(2)), &Types::f64);

        test_type(make_binary_operator(Tt::minus, make_float_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::minus, make_int_literal(1), make_float_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::minus, make_double_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::minus, make_int_literal(1), make_double_literal(2)), &Types::f64);

        test_type(make_binary_operator(Tt::asterisk, make_float_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::asterisk, make_int_literal(1), make_float_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::asterisk, make_double_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::asterisk, make_int_literal(1), make_double_literal(2)), &Types::f64);

        test_type(make_binary_operator(Tt::slash, make_float_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::slash, make_int_literal(1), make_float_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::slash, make_double_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::slash, make_int_literal(1), make_double_literal(2)), &Types::f64);

        test_type(make_binary_operator(Tt::mod, make_float_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::mod, make_int_literal(1), make_float_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::mod, make_double_literal(1), make_int_literal(2)), &Types::f64);
        test_type(make_binary_operator(Tt::mod, make_int_literal(1), make_double_literal(2)), &Types::f64);
    };

    SECTION("Different signedness")
    {
        test_type(make_binary_operator(Tt::plus, make_int_literal(1, 'u'), make_int_literal(2)), nullptr);
        test_type(make_binary_operator(Tt::minus, make_int_literal(1), make_int_literal(2, 'u')), nullptr);
        test_type(make_binary_operator(Tt::asterisk, make_int_literal(1), make_int_literal(2, 'u')), nullptr);
        test_type(make_binary_operator(Tt::slash, make_int_literal(1), make_int_literal(2, 'u')), nullptr);
        test_type(make_binary_operator(Tt::mod, make_int_literal(1, 'u'), make_int_literal(2)), nullptr);
    };

    SECTION("Bitwise")
    {
        test_type(
            make_binary_operator(Tt::left_shift, make_int_literal(1, 'u'), make_int_literal(2, 'u')),
            &Types::u64);
        test_type(
            make_binary_operator(Tt::right_shift, make_int_literal(1, 'u'), make_int_literal(2, 'u')),
            &Types::u64);
        test_type(make_binary_operator(Tt::bit_and, make_int_literal(1, 'u'), make_int_literal(2, 'u')), &Types::u64);
        test_type(make_binary_operator(Tt::bit_xor, make_int_literal(1, 'u'), make_int_literal(2, 'u')), &Types::u64);
        test_type(make_binary_operator(Tt::bit_or, make_int_literal(1, 'u'), make_int_literal(2, 'u')), &Types::u64);

        test_type(make_binary_operator(Tt::left_shift, make_int_literal(1, 'u'), make_int_literal(2)), nullptr);
        test_type(make_binary_operator(Tt::right_shift, make_int_literal(1), make_int_literal(2, 'u')), nullptr);
        test_type(make_binary_operator(Tt::bit_and, make_int_literal(1, 'u'), make_int_literal(2)), nullptr);
        test_type(make_binary_operator(Tt::bit_xor, make_int_literal(1), make_int_literal(2)), nullptr);
        test_type(make_binary_operator(Tt::bit_or, make_int_literal(1), make_int_literal(2)), nullptr);
    };

    SECTION("Boolean short circuit")
    {
        test_type(
            make_binary_operator(Tt::logical_or, make_bool_literal(true), make_int_literal(false)),
            &Types::boolean);
        test_type(
            make_binary_operator(Tt::logical_and, make_bool_literal(false), make_int_literal(true)),
            &Types::boolean);
        test_type(make_binary_operator(Tt::logical_or, make_int_literal(1), make_int_literal(2)), nullptr);
        test_type(make_binary_operator(Tt::logical_and, make_int_literal(1), make_int_literal(2)), nullptr);
        test_type(make_binary_operator(Tt::logical_and, make_int_literal(1), make_int_literal(2)), nullptr);
        test_type(make_binary_operator(Tt::logical_or, make_int_literal(1), make_int_literal(2)), nullptr);
    };

    SECTION("Comparison")
    {
        test_type(make_binary_operator(Tt::inequal, make_int_literal(1), make_int_literal(2)), &Types::boolean);
        test_type(make_binary_operator(Tt::equal, make_int_literal(1), make_int_literal(2)), &Types::boolean);
        test_type(make_binary_operator(Tt::greater_than, make_int_literal(1), make_int_literal(2)), &Types::boolean);
        test_type(
            make_binary_operator(Tt::greater_than_or_equal, make_int_literal(1), make_int_literal(2)),
            &Types::boolean);
        test_type(make_binary_operator(Tt::less_than, make_int_literal(1), make_int_literal(2)), &Types::boolean);
        test_type(
            make_binary_operator(Tt::less_than_or_equal, make_int_literal(1), make_int_literal(2)),
            &Types::boolean);
    };
}

TEST_CASE("Identifiers - declared in parent block", "[typecheck]")
{
    BlockNode parent_block{};

    auto decl              = new DeclarationNode{};
    decl->containing_block = &parent_block;
    decl->identifier       = "x";
    decl->specified_type   = const_cast<SimpleTypeNode *>(&Types::u8);  // TODO
    decl->init_expression  = new NopNode{};
    parent_block.statements.push_back(decl);

    REQUIRE(typecheck(decl));

    BlockNode child_block{};
    child_block.containing_block = &parent_block;

    {
        IdentifierNode identifier{};
        identifier.containing_block = &child_block;
        identifier.identifier       = "x";

        REQUIRE(typecheck(&identifier));
        REQUIRE(types_equal(identifier.type, &Types::u8));
    }

    {
        IdentifierNode identifier{};
        identifier.containing_block = &child_block;
        identifier.identifier       = "y";

        REQUIRE(typecheck(&identifier) == false);
    }
}

TEST_CASE("Identifiers - uninitialized, with explicit type", "[typecheck]")
{
    BlockNode block{};

    auto decl              = new DeclarationNode{};
    decl->containing_block = &block;
    decl->identifier       = "x";
    decl->specified_type   = const_cast<SimpleTypeNode *>(&Types::u8);  // TODO
    decl->init_expression  = new NopNode{};
    block.statements.push_back(decl);

    REQUIRE(typecheck(decl));

    {
        IdentifierNode identifier{};
        identifier.containing_block = &block;
        identifier.identifier       = "x";

        REQUIRE(typecheck(&identifier));
        REQUIRE(types_equal(identifier.type, &Types::u8));
    }

    {
        IdentifierNode identifier{};
        identifier.containing_block = &block;
        identifier.identifier       = "y";

        REQUIRE(typecheck(&identifier) == false);
    }
}

TEST_CASE("Identifiers - initialized, without explicit type", "[typecheck]")
{
    BlockNode block{};

    LiteralNode init_expression{};
    init_expression.value.emplace<bool>(true);

    auto decl              = new DeclarationNode{};
    decl->containing_block = &block;
    decl->identifier       = "x";
    decl->specified_type   = new NopNode{};
    decl->init_expression  = &init_expression;
    block.statements.push_back(decl);

    REQUIRE(typecheck(decl));

    {
        IdentifierNode identifier{};
        identifier.containing_block = &block;
        identifier.identifier       = "x";

        REQUIRE(typecheck(&identifier));
        REQUIRE(types_equal(identifier.type, &Types::boolean));
    }

    {
        IdentifierNode identifier{};
        identifier.containing_block = &block;
        identifier.identifier       = "y";

        REQUIRE(typecheck(&identifier) == false);
    }
}

TEST_CASE("Declarations - initialized, with different explicit type", "[typecheck]")
{
    BlockNode block{};

    LiteralNode init_expression{};
    init_expression.value.emplace<bool>(true);

    auto decl              = new DeclarationNode{};
    decl->containing_block = &block;
    decl->identifier       = "x";
    decl->specified_type   = const_cast<SimpleTypeNode *>(&Types::i8);  // TODO
    decl->init_expression  = &init_expression;
    block.statements.push_back(decl);

    REQUIRE(typecheck(decl) == false);
}
