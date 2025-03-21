#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basics.h"
#include "parse.h"
#include "typecheck.h"

int main(int argc, char **argv)
{
    printf("This is the Janguage compiler.\n");

    if (argc != 2)
    {
        fprintf(stderr, "Usage: jang <main source file>\n");
        return 1;
    }

    auto path = argv[1];
    printf("Compiling file: %s\n", path);

    auto f = fopen(path, "r");
    if (f == nullptr)
    {
        fprintf(stderr, "Could not open file %s", path);
        return 1;
    }
    defer
    {
        fclose(f);
    };

    fseek(f, 0, SEEK_END);
    auto file_size = ftell(f);
    rewind(f);
    auto source = static_cast<char *>(malloc(file_size + 1));
    defer
    {
        free(source);
    };
    source[file_size] = 0;

    size_t pos  = 0;
    size_t read = 0;
    for (size_t read = 0; (read = fread(&source[pos], 1, file_size - pos, f)); pos += read)
    {
    }

    Lexer lexer{source};

    AstProgram program;
    if (parse_program(source, program) == false)
    {
        std::cout << "Failed to parse program" << std::endl;
        return 1;
    }

    auto node = make_node(nullptr, &program);

// #if 0
//     printf("Dumping tokens\n");
//     Token token;
//     while (true)
//     {
//         token = next_token(&l);
//         printf("Line %d char %d: %s ", (int)token.line, (int)token.line_offset, to_string(token.type));
//         if (token.len > 0)
//             fwrite(l.start, 1, l.at - l.start, stdout);
//         printf("\n");

//         if (token.type == Tt::EOF)
//         {
//             break;
//         }
//     }
// #else
//     AstProgram program{};
//     if (parse_program(source, program) == false)
//     {
//         return 1;
//     }

//     /* printf("Program:\n%s\n", dump_node(0, &program).data()); */

//     // BytecodeWriter w{};
//     // if (generate_code(&program.block, &w) == false)
//     // {
//     //     return 1;
//     // }
// #endif

    return 0;
}
