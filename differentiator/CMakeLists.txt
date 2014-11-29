add_library (calculator
             lexer.cpp parser.cpp
             node.cpp node-dump.cpp node-priority.cpp node-compare.cpp
             visitor-print.cpp visitor-calculate.cpp node-clone.cpp visitor-simplify.cpp visitor-optimize.cpp visitor-differentiate.cpp
             util-tree.cpp)

add_executable (differentiator
                main-differentiator.cpp)
target_link_libraries (differentiator calculator)

add_executable (error_calculator
                main-error_calculator.cpp)
target_link_libraries (error_calculator calculator)

install(TARGETS differentiator error_calculator
        RUNTIME DESTINATION bin)

