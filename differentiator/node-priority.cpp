#include "node.h"

namespace Node
{

int AdditionSubtraction::priority() const    { return 0; }
int MultiplicationDivision::priority() const { return 1; }
int Power::priority() const                  { return 2; }
int Variable::priority() const               { return 3; }
int Function::priority() const               { return 3; }
int Value::priority() const                  { return 3; }

} // namespace Node
