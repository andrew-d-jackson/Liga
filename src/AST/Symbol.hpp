#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTSymbol : public ASTNode {
public:
	std::string val;

public:
	ASTSymbol(std::string val) : val(val) {}

	virtual DataType type() const { return DataType::Symbol; }

	virtual GTPtr return_type(Enviroment &env) const {
		return env.value_map[val].type;
	}

	GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
		if (env.value_map.find(val) == env.value_map.end()) {
			std::cerr << "Could not find symbol: " << val << std::endl;
			throw;
			exit(1);
		}
		auto found_value = env.value_map[val];
		return found_value;
	}

	virtual std::string as_string() const { return val + " "; }
};
