#pragma once
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <tuple>
#include "../../Error/Error.h"
#include "../RuntimeValue/Value.h"
#include "../../Frontend/Node/Node.h"


namespace Coda {
	namespace Runtime {
		/*
			An Environment is used to store the variables and their values.
			A new environment is created for every function call.
			Environment is responsible for variable lookup and assignment.
		*/

		class Environment {
			// A function is a callable object.
		public:
			typedef std::function<ValuePtr(ValuePtr value, Environment scope)> Function;

			// <name, declaration environment, body (AST)>
			typedef std::tuple<std::string, Coda::Runtime::Environment, Coda::Frontend::Node> UserDefinedFunction;

		public:
			// Creates a new environment.
			Environment();

			// Creates a new environment also setting its parent. 
			Environment(Environment* parentEnvironment);

			// Generate a new environment with the root environment as its parent.
			static Environment root();

			/*
				Declares a new variable or assigns a value to an existing variable.
				Throws an error if the variable is constant.
				@param name - The name of the variable.
				@param value - The value of the variable.
				@param isConstant - Whether the variable is constant or not.
				@return - The value of the variable.
			*/
			ValuePtr declareOrAssignVariable(const std::string& name, const ValuePtr& value, bool isConstant = false);

			/*
				Overload of declareOrAssignVariable(const std::string&, const Value&, bool).
			*/
			ValuePtr declareOrAssignVariable(const Frontend::Node& name, const ValuePtr& value, bool isConstant = false);

			/*
				Declare native function.
				@param name - The name of the function.
				@param function - The function.
				@return - The value of the function.
			*/
			ValuePtr declareNativeFunction(const std::string& name, Function function);

			/*
				Declare a user defined function.
				@param name - The name of the function.
				@param astNode - The AST of the function which will be evaluated when the function is called.
				@return - The value of the function.
			*/
			ValuePtr declareUserDefinedFunction(const std::string& name, Frontend::Node astNode);

			/*
				call a UserDefinedFunction.
				@param name - The name of the function.
				@param args - The arguments of the function.
				@param env - The environment in which the function will be called.
				@return - The last evaluated value of the function.
			*/
			ValuePtr callFunction(const std::string& name, const ValuePtr& args, Environment& env);

			/*
				looks for the given symbol in the current environment and its parents.
				returns the value of the symbol if found.
				throws an error if the symbol is not found.
				@param varname - The name of the symbol.
				@return - The value of the symbol.
			*/
			ValuePtr lookupSymbol(std::string varname);

			/*
				Looks for the user defined function with the given name.
				@param name - The name of the function.
				@return - The function.
			*/
			UserDefinedFunction* getFunction(const std::string& name);

			/*
				Add a user defined function to the environment.
				@param name - The name of the function.
				@param function - The function.
			*/
			ValuePtr addFunction(const std::string& name, const Frontend::Node& astNode, Environment& env);

			/*
				Looks for variables, functions and constants with the given name and removes them.
				@param name - The name of the variable, function or constant.
			*/
			void remove(const std::string& name);


		private:
			Environment* parent;
			std::map<std::string, ValuePtr> symbols;
			std::map<std::string, Function> functions;
			std::set<std::string> constants;
			std::vector<UserDefinedFunction> userDefinedFunctions;

		private:
			Environment* resolve(std::string name);
		};
	} // namespace Runtime
} // namespace Coda