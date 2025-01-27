#include "Interpreter.h"

namespace Coda {
	namespace Runtime {
		ValuePtr Interpreter::evaluateUnaryExpression(const Frontend::Node& op, Environment& env) {
			IF_ERROR_RETURN_VALUE_PTR;
			std::string unaryOperator = op.value;
			ValuePtr value = interpret(*op.left.get(), env);
			IF_ERROR_RETURN_VALUE_PTR;
			if (unaryOperator == "-") {
				value->value = ((value->value[0] == '-') ? value->value.substr(1) : "-" + value->value);
				value->value.erase(value->value.find_last_not_of('0') + 1, std::string::npos);
				value->value.erase(value->value.find_last_not_of('.') + 1, std::string::npos);
				return value;
			}
			else if (unaryOperator == "+") {
				return std::make_shared<Value>(value);
			}
			else if (unaryOperator == "!") {
				Value booleanValue = Value(Type::BOOL, "false", op.startPosition, op.endPosition);
				if (!Value::isTruthy(*(std::shared_ptr<IValue>)value))
					booleanValue.value = "true";
				return std::make_shared<Value>(booleanValue);
			}
			else if (unaryOperator == "++") {
				double num = std::stod(value->value);
				num++;
				value->value = std::to_string(num);
				value->value.erase(value->value.find_last_not_of('0') + 1, std::string::npos);
				value->value.erase(value->value.find_last_not_of('.') + 1, std::string::npos);

				return value;
			}
			else if (unaryOperator == "--") {
				double num = std::stod(value->value);
				num--;
				value->value = std::to_string(num);
				value->value.erase(value->value.find_last_not_of('0') + 1, std::string::npos);
				value->value.erase(value->value.find_last_not_of('.') + 1, std::string::npos);
				return value;
			}
			else if (unaryOperator == "typeof") {
				return std::make_shared<Value>(Type::STRING, Value::getTypeAsString(value->type), value->startPosition, value->endPosition);
			}
			else if (unaryOperator == "sizeof") {
				int length = 0;
				if (value->type == Type::LIST || value->type == Type::OBJECT) {
					length = value->properties.size();
				}
				else {
					length = value->value.size();
				}
				return std::make_shared<Value>(Type::INT, std::to_string(length), value->startPosition, value->endPosition);
			}
			else if (unaryOperator == "delete") {
				env.remove(value->value);
				return std::make_shared<Value>(Type::UNDEFINED);
			}
			else {
				Error::Runtime::raise("Unrecognized unary operator '" + unaryOperator + "'");
				return nullptr;
			}
		}

		ValuePtr Interpreter::evaluateIdentifier(const Frontend::Node& astNode, Environment& env)
		{
			IF_ERROR_RETURN_VALUE_PTR;
			return IVALUE_TO_VALUE(env.lookupSymbol(astNode.value));
		}

		ValuePtr Interpreter::performAssignmentOperation(ValuePtr left, const ValuePtr& interpreted, const std::function<double(double, double)>& operation)
		{
			IF_ERROR_RETURN_VALUE_PTR;
			if (isNumericType(left->type) && isNumericType(interpreted->type)) {
				double num = std::stod(left->value);
				double rightNum = std::stod(interpreted->value);
				num = operation(num, rightNum);
				left->value = std::to_string(num);
				left->type = (left->type == Type::DOUBLE) ? Type::DOUBLE : Type::FLOAT;
				left->value.erase(left->value.find_last_not_of('0') + 1, std::string::npos);
				left->value.erase(left->value.find_last_not_of('.') + 1, std::string::npos);
				return left;
			}
			else {
				Error::Runtime::raise("Cannot assign '" + interpreted->value + "' to '" + left->value + "' as both types must be numeric.");
				return nullptr;
			}
		}

		ValuePtr Interpreter::evaluateAssignmentExpression(const Frontend::Node& astNode, Environment& env)
		{
			IF_ERROR_RETURN_VALUE_PTR;

			if (astNode.value == "=") {
				if (astNode.left->type == Frontend::NodeType::IDENTIFIER) {
					if (astNode.right->type == Frontend::NodeType::JUMP_EXPRESSION) {
						ValuePtr assignable = interpret(*astNode.right.get(), env);
						env.declareOrAssignVariable(astNode.left->value, assignable);
						return std::make_shared<Value>(Type::NONE, "");
					}
					else {
						return IVALUE_TO_VALUE(env.declareOrAssignVariable(astNode.left->value, interpret(*astNode.right.get(), env)));
					}
				}
				else if (astNode.left->type == Frontend::NodeType::MEMBER_EXPRESSION) {
					if (astNode.right->type == Frontend::NodeType::JUMP_EXPRESSION) {
						ValuePtr assignable = interpret(*astNode.right.get(), env);
						env.declareOrAssignVariable(astNode.left->value, assignable);
						return std::make_shared<Value>(Type::NONE, "");
					}
					else {
						return IVALUE_TO_VALUE(env.declareOrAssignVariable(*astNode.left.get(), interpret(*astNode.right.get(), env)));
					}
				}
				else {
					Error::Runtime::raise("Invalid Assignment Operation, at ", astNode.endPosition);
					return nullptr;
				}
			}
			else if (astNode.value == "+=") {
				ValuePtr left = IVALUE_TO_VALUE(env.lookupSymbol(astNode.left->value));
				ValuePtr interpreted = interpret(*astNode.right.get(), env);
				return performAssignmentOperation(left, interpreted, [](double a, double b) { return a + b; });
			}
			else if (astNode.value == "-=") {
				ValuePtr left = IVALUE_TO_VALUE(env.lookupSymbol(astNode.left->value));
				ValuePtr interpreted = interpret(*astNode.right.get(), env);
				return performAssignmentOperation(left, interpreted, [](double a, double b) { return a - b; });
			}
			else if (astNode.value == "*=") {
				ValuePtr left = IVALUE_TO_VALUE(env.lookupSymbol(astNode.left->value));
				ValuePtr interpreted = interpret(*astNode.right.get(), env);
				return performAssignmentOperation(left, interpreted, [](double a, double b) { return a * b; });
			}
			else if (astNode.value == "/=") {
				ValuePtr left = IVALUE_TO_VALUE(env.lookupSymbol(astNode.left->value));
				ValuePtr interpreted = interpret(*astNode.right.get(), env);
				return performAssignmentOperation(left, interpreted, [](double a, double b) { return a / b; });
			}
			else {
				return nullptr;
			}
		}

		ValuePtr Interpreter::evaluateVariableDeclaration(const Frontend::Node& astNode, Environment& env, bool isConstant)
		{
			IF_ERROR_RETURN_VALUE_PTR;

			ValuePtr rhs = interpret(*astNode.right.get(), env);
			if (astNode.properties.size() > 0 && astNode.properties.at("copy") != nullptr) {
				rhs = std::make_shared<Value>(rhs->copy());
			}

			return IVALUE_TO_VALUE(env.declareOrAssignVariable(astNode.left->value, rhs, isConstant));
		}
	} // namespace Frontend
} // namespace Coda