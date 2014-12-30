add_library (expression
             lexer.cpp parser.cpp
             node.cpp node-dump.cpp node-priority.cpp node-compare.cpp
             visitor-print.cpp visitor-calculate.cpp node-clone.cpp visitor-simplify.cpp visitor-differentiate.cpp visitor-latex.cpp
             util-tree.cpp)

add_executable (calculator
                main.cpp)
target_link_libraries (calculator expression)

add_executable (taylor
                main-taylor.cpp)
target_link_libraries (taylor expression)

add_custom_command (OUTPUT README.html
                    COMMAND /usr/bin/pandoc ARGS --from markdown+definition_lists+compact_definition_lists --to html5 --standalone --smart "${CMAKE_CURRENT_SOURCE_DIR}/README.md" -o "README.html"
                    MAIN_DEPENDENCY README.md)
add_custom_target(build-documentation ALL
                  DEPENDS README.html)

install(TARGETS calculator
        RUNTIME DESTINATION bin)

