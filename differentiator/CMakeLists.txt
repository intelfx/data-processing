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

install(TARGETS calculator
        RUNTIME DESTINATION bin)

