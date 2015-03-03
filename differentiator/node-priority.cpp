#include "node.h"

namespace Node
{

enum class Priority
{
	AddSub,
	MulDiv,
	Power,
	Value,
	Variable = Value,
	Function = Value
};

Priority AdditionSubtraction::priority_static()    { return Priority::AddSub; }
Priority MultiplicationDivision::priority_static() { return Priority::MulDiv; }
Priority Power::priority_static()                  { return Priority::Power; }
Priority Variable::priority_static()               { return Priority::Variable; }
Priority Function::priority_static()               { return Priority::Function; }
Priority Value::priority_static()                  { return Priority::Value; }


bool Base::numeric_output() const            { return false; }
bool Value::numeric_output() const           { return true; }
bool Power::numeric_output() const           { return get_base()->numeric_output(); }

} // namespace Node
