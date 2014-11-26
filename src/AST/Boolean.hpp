#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

#include "Types/Boolean.hpp"

class ASTBoolean : public ASTNode {
private:
	bool val;

public:
	ASTBoolean(bool val) : val(val) {}

	virtual DataType type() const { return DataType::Boolean; }

	virtual GTPtr return_type(Enviroment &env) const {
		return std::make_shared<BooleanType>();
	}

	GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
		auto gen = builder.getInt1(val);
		return BooleanType().create(gen);
	}

	virtual std::string as_string() const {
		return val ? "true " : "false ";
	};
};