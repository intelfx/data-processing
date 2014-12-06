add_library (expression
             lexer.cpp parser.cpp
             node.cpp node-dump.cpp node-priority.cpp node-compare.cpp
             visitor-print.cpp visitor-calculate.cpp node-clone.cpp visitor-simplify.cpp visitor-optimize.cpp visitor-differentiate.cpp visitor-latex.cpp
             util-tree.cpp)

add_executable (differentiator
                main-differentiator.cpp)
target_link_libraries (differentiator expression)

add_executable (error_calculator
                main-error_calculator.cpp)
target_link_libraries (error_calculator expression)

install(TARGETS differentiator error_calculator
        RUNTIME DESTINATION bin)

