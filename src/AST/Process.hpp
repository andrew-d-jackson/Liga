#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTProcess : public ASTNode {
public:
	std::vector<std::shared_ptr<ASTNode>> val;

public:
	ASTProcess(std::vector<std::shared_ptr<ASTNode>> val) : val(val) {}

	virtual DataType type() const { return DataType::Process; }

	GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
		auto first_val = val.at(0)->to_value(env, builder);
		if (first_val.type->data_type() == DataType::Macro) {
			auto mac_args =
				std::vector<std::shared_ptr<ASTNode>>(val.begin() + 1, val.end());
			return first_val.macro->call(env, builder, mac_args);
		}

		std::vector<GenericValue> values;
		values.push_back(first_val);
		for (auto it = val.begin() + 1; it != val.end(); it++) {
			values.push_back((*it)->to_value(env, builder));
		}

		if (values.at(0).type->data_type() == DataType::Function) {
			auto fn_args =
				std::vector<GenericValue>(values.begin() + 1, values.end());
			return values.at(0).func->call(env, builder, fn_args);
		}
		else {
			return values.at(0);
		}
	};

	virtual GTPtr return_type(Enviroment &env) const {
		std::vector<GTPtr> val_types;
		for (const auto &i : val) {
			val_types.push_back(i->return_type(env));
		}

		if (val_types.at(0)->data_type() == DataType::Macro) {
			auto args_list =
				std::vector<std::shared_ptr<ASTNode>>(val.begin() + 1, val.end());
			return static_cast<MacroType *>(val_types.at(0).get())
				->return_type(env, args_list);
		}

		if (val_types.at(0)->data_type() == DataType::Function) {
			auto args_list =
				std::vector<GTPtr>(val_types.begin() + 1, val_types.end());
			return static_cast<FunctionType *>(val_types.at(0).get())
				->return_type(env, args_list);
		}
		return *(val_types.end() - 1);
	}

	virtual std::string as_string() const {
		std::string ret = "[ ";
		for (const auto &i : val) {
			ret += i->as_string();
		}
		ret += "] ";
		return ret;
	}
};
