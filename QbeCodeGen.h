#pragma once
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <iostream>

#include "SymbolTable.h"

// QBE IR emitter and small state container for code generation.
class QbeCodeGen {
public:
	std::ostringstream output;
	int tempCount = 0;

	int labelCount = 0;

	std::unordered_map<std::string, std::string> varMap;
	std::unordered_map<std::string, std::vector<std::pair<Type, std::string>>> functionMap;
	std::unordered_map<std::string, std::string> stringLiteralNames;
	int stringCount = 0;

	std::string lastValue;

	// Summary: Allocates a unique temporary SSA variable name.
	// Input: none.
	// Output: fresh temp variable identifier (e.g. %.N).
	std::string newTempVar() {
		return "%." + std::to_string(labelCount++);
	}

	// Summary: Allocates a unique jump label name using a prefix.
	// Input: label prefix string.
	// Output: fresh QBE label identifier.
	std::string newLabel(std::string name)
	{
		return "@" + name + std::to_string(tempCount++);
	}

	// Summary: Writes indentation spaces into the output stream.
	// Input: indentation depth in logical units.
	// Output: no return value; output stream is updated.
	void emitIndent(int indent) {
		for (int i = 0; i < indent; ++i)
			output << "  ";
	}

	// Summary: Interns a string literal for later data section emission.
	// Input: literal string value.
	// Output: updates string literal table if value is new.
	void RegisterStringLiteral(std::string& value)
	{
		if (stringLiteralNames.find(value) == stringLiteralNames.end()) {
			std::string name = "str" + std::to_string(stringLiteralNames.size());
			stringLiteralNames[value] = name;
		}
	}

	// Summary: Serializes data section and accumulated QBE body to a file.
	// Input: output file path.
	// Output: written SSA file on disk.
	void produceFinalFile(std::string fileName)
	{
		std::ofstream outFile(fileName);
		if (!outFile) {
			std::cerr << "Error opening file for writing: " << fileName << std::endl;
			return;
		}
		for (const auto& pair : stringLiteralNames) {
            outFile << "data $" << pair.second << " = { b \"" << pair.first << "\", b 0 }\n";
		}
		outFile << output.str();
		outFile.close();
	}

	void storeWordValue(const std::string& value, const std::string& var) {
		output << "storew " << value << ", " << var << "\n";
	}

	void storeByteValue(const std::string& value, const std::string& var) {
		output << "storeb " << value << ", " << var << "\n";
	}

	void storeLabelValue(const std::string& value, const std::string& var) {
		output << "storel " << value << ", " << var << "\n";
	}

	void loadWordValue(const std::string& var, const std::string& dest) {
		output << dest << " =w loadw " << var << "\n";
	}

	void loadByteValue(const std::string& var, const std::string& dest) {
		output << dest << " =b loadb " << var << "\n";
	}

	void loadLabelValue(const std::string& var, const std::string& dest) {
		output << dest << " =l loadl " << var << "\n";
	}

	void declareWordVar(const std::string& var) {
		output << var << "=alloc4\n";
	}

	void declareLongVar(const std::string& var) {
		output << var << "=alloc8\n";
	}

	void emitLongAssignment(const std::string& var, const std::string& value) {
		output << var << " =l copy " << value << "\n";
	}

	void emitNumAssignment(const std::string& var, int value)
	{
		output << var << " =w copy " << value << "\n";
	}

	void emitBoolAssignment(const std::string& var, bool value)
	{
		output << var << " =w copy " << (value ? 1 : 0) << "\n";
	}

	void emitCharAssignment(const std::string& var, char value) 
	{
		output << var << " =b copy '" << value << "'\n";
	}

	void emitStringAssignment(const std::string& var, const std::string& value) 
	{
		RegisterStringLiteral(const_cast<std::string&>(value));
		emitLongAssignment(((var[0] == '%') ? var : "%" + var), "$" + stringLiteralNames[value]);
	}

	void emitPrintf(const std::string& formatVar, const std::vector<std::string>& argVars) {
		output << "call $printf(l " << formatVar << ", ...";
		for (const auto& arg : argVars) {
			output << ", w " << arg;
		}
		output << ")\n";
	}

	std::string stripVarName(const std::string& var) {
		if (var[0] == '%')
			return var.substr(1);
		return var;	
	}

	void allocatePointer(const std::string& var) 
	{
		if (!varMap.count(var))
		{
			output << "%" << var << " =alloc8\n";
			varMap[var] = "%" + var;
		}
	}

	// Summary: Emits function prologue for parameterless functions.
	// Input: function name and return type.
	// Output: function header in QBE output and function metadata entry.
	void emitFunctionStart(const std::string& funcName, Type returnType) {
		output << "export function " << toQbeType(returnType) <<  " $" << funcName << "() {\n";
		output << "@start\n";
		functionMap[funcName].push_back(std::make_pair(returnType, ""));
	}

	// Summary: Emits function prologue with typed parameters.
	// Input: function name, return type, and ordered parameter list.
	// Output: function header in QBE output and function metadata entry.
	void emitFunctionStart(const std::string& funcName, Type returnType, std::vector<std::pair<Type, std::string>> params) {
		output << "export function " << toQbeType(returnType) << " $" << funcName << "(";
		for (size_t i = 0; i < params.size(); ++i) {
			output << toQbeType(params[i].first) << " %" << params[i].second;
			if (i < params.size() - 1)
				output << ", ";
		}
		output << ") {\n";
		output << "@start\n";
		
		params.push_back(std::make_pair(returnType, ""));
		functionMap[funcName] = params;
	}

	void emitFunctionEnd()
	{
		output << "}\n";
	}

	void emitReturn(const std::string& var) {
		output << "ret " << var << "\n";
	}

	// Summary: Emits a typed function call and stores result into resultVar.
	// Input: callee name, evaluated argument registers, and destination register.
	// Output: call instruction appended to output stream.
	void emitFuncCall(const std::string& funcName, std::vector<std::string> argRegs, const std::string& resultVar) {
		auto func = functionMap[funcName];
	
		//Find the return type in ""
		Type returnType;
		for (const auto& param : func) {
			if (param.second == "")
			{
				returnType = param.first;
				break;
			}
		}

		output << resultVar << " =" << toQbeType(returnType) << " call $" << funcName << "(";

		bool first = true;
		for (int i = 0; i < argRegs.size(); ++i) {
			auto param = func[i];
			std::cout << toQbeType(param.first) << " " << argRegs[i] << "\n";
			//If the arg name is "" skip it since it's the return type
			if (param.second == "")
				continue;

			if (!first)
				output << ", ";
			  first = false;

			output << toQbeType(param.first) << " " << argRegs[i];
			
		}
		output << ")\n";
	}

	// Summary: Emits arithmetic instruction for integers or pointer arithmetic.
	// Input: destination var, lhs/rhs operands, operator symbol, pointer-mode flag.
	// Output: arithmetic QBE instruction in output stream.
	void emitArithmetic(const std::string& resultVar, const std::string& lhs, const std::string& rhs, const std::string& op, bool isPointer = false)
	{
		std::cout << isPointer << "\n";
		std::string qbeOp;
		if (op == "+") qbeOp = "add";
		else if (op == "-") qbeOp = "sub";
		else if (op == "*") qbeOp = "mul";
		else if (op == "/") qbeOp = "div";
		else {
			std::cerr << "Unsupported operator: " << op << std::endl;
			return;
		}
		if (isPointer) 
			output << resultVar << " =l " << qbeOp << " " << lhs << ", " << rhs << "\n";
		else
		  output << resultVar << " =w " << qbeOp << " " << lhs << ", " << rhs << "\n";
		
	}

	// Summary: Emits comparison instruction producing boolean-like word result.
	// Input: destination var, lhs/rhs operands, comparison operator.
	// Output: comparison QBE instruction in output stream.
	void emitComparison(const std::string& resultVar, const std::string& lhs, const std::string& rhs, const std::string& op)
	{
		std::string qbeOp = "c";
		if (op == "==") qbeOp += "eq";
		else if (op == "!=") qbeOp += "ne";
		else if (op == "<") qbeOp += "slt";
		else if (op == "<=") qbeOp += "sle";
		else if (op == ">") qbeOp += "sgt";
		else if (op == ">=") qbeOp += "sge";
		else {
			std::cerr << "Unsupported operator: " << op << std::endl;
			return;
		}
		output << resultVar << " =w " << qbeOp << "w " << lhs << ", " << rhs << "\n";
	}

	// Summary: Maps semantic types to QBE register type codes.
	// Input: internal Type enum value.
	// Output: QBE type string ("w", "b", or "l").
	std::string toQbeType(const Type& varType) {
		if (varType == Type::NUM) return "w";
		else if (varType == Type::BOOL) return "w";
		else if (varType == Type::CHAR) return "b";
		else if (varType == Type::STRING) return "l";
		return "w"; // default
	}

	// Summary: Maps semantic types to byte sizes used by allocations.
	// Input: internal Type enum value.
	// Output: allocation size in bytes.
	int toQbeSize(const Type& varType) {
		if (varType == Type::STRING) return 8;	
		return 4; // default
	}


};
