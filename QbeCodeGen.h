#pragma once
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <iostream>

#include "SymbolTable.h"

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

	std::string newTempVar() {
		return "%." + std::to_string(labelCount++);
	}

	std::string newLabel(std::string name)
	{
		return "@" + name + std::to_string(tempCount++);
	}

	void emitIndent(int indent) {
		for (int i = 0; i < indent; ++i)
			output << "  ";
	}

	void RegisterStringLiteral(std::string& value)
	{
		if (stringLiteralNames.find(value) == stringLiteralNames.end()) {
			std::string name = "str" + std::to_string(stringLiteralNames.size());
			stringLiteralNames[value] = name;
		}
	}

	void produceFinalFile(std::string fileName)
	{
		std::ofstream outFile(fileName);
		if (!outFile) {
			std::cerr << "Error opening file for writing: " << fileName << std::endl;
			return;
		}
		for (const auto& pair : stringLiteralNames) {
            outFile << "data $" << pair.second << " = { b \"" << pair.first << "\\n\", b 0 }\n";
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

	void allocatePointer(const std::string& var) {
		if (!varMap.count(var))
		{
			output << "%" << var << " =alloc8\n";
			varMap[var] = "%" + var;
		}
	}

	void emitFunctionStart(const std::string& funcName, Type returnType) {
		output << "export function " << toQbeType(returnType) <<  " $" << funcName << "() {\n";
		output << "@start\n";
		functionMap[funcName].push_back(std::make_pair(returnType, ""));
	}

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

	void emitArithmetic(const std::string& resultVar, const std::string& lhs, const std::string& rhs, const std::string& op)
	{
		std::string qbeOp;
		if (op == "+") qbeOp = "add";
		else if (op == "-") qbeOp = "sub";
		else if (op == "*") qbeOp = "mul";
		else if (op == "/") qbeOp = "div";
		else {
			std::cerr << "Unsupported operator: " << op << std::endl;
			return;
		}
		output << resultVar << " =w " << qbeOp << " " << lhs << ", " << rhs << "\n";
	}

	void emitComparison(const std::string& resultVar, const std::string& lhs, const std::string& rhs, const std::string& op)
	{
		std::string qbeOp = "c";
		if (op == "==") qbeOp += "eq";
		else if (op == "!=") qbeOp += "ne";
		else if (op == "<") qbeOp += "lt";
		else if (op == "<=") qbeOp += "le";
		else if (op == ">") qbeOp += "gt";
		else if (op == ">=") qbeOp += "ge";
		else {
			std::cerr << "Unsupported operator: " << op << std::endl;
			return;
		}
		output << resultVar << " =w " << qbeOp << "w " << lhs << ", " << rhs << "\n";
	}

	std::string toQbeType(const Type& varType) {
		if (varType == Type::NUM) return "w";
		else if (varType == Type::BOOL) return "w";
		else if (varType == Type::CHAR) return "b";
		else if (varType == Type::STRING) return "l";
		return "w"; // default
	}

	int toQbeSize(const Type& varType) {
		if (varType == Type::STRING) return 8;	
		return 4; // default
	}


};
