#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"
#include "Types/Char.hpp"

class ASTChar : public ASTNode {
private:
	char val;

public:
    ASTChar(char val) : val(val) {}

	virtual GTPtr return_type(Enviroment &env) const {
		return std::make_shared<CharType>();
	}

	virtual DataType type() const { return DataType::Char; }

	GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
		auto gen = builder.getInt8(val);
		return CharType().create(gen);
	};

	virtual std::string as_string() const {
        auto x = std::string("'");
        x.push_back(val);
        x.push_back('\'');
        x.push_back(' ');
        return x;
	}
};