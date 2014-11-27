#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "RefrenceCounter.hpp"
#include "ASTNode.hpp"

#include "Types/Vector.hpp"


class ASTVector : public ASTNode {
public:
	std::vector<std::shared_ptr<ASTNode>> val;

public:
	ASTVector(std::vector<std::shared_ptr<ASTNode>> val) : val(val) {}

	virtual DataType type() const { return DataType::Vector; }
	virtual GTPtr return_type(Enviroment &env) const {
		return std::make_shared<VectorType>(val.at(0)->return_type(env));
	}
	GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
		std::vector<GenericValue> vals;


        Scope s;
		auto env2 = env.with_new_scope(s);

		for (auto &i : val) {
			vals.push_back(i->to_value(env2, builder));
		}
		auto ptr = env.malloc_fn.to_ptr(env, builder, vals);

		auto rc = to_rc(env, builder, ptr);

		auto st_ty = llvm::StructType::get(
			llvm::getGlobalContext(),
			std::vector<llvm::Type *>({ IntegerType().llvm_type(), rc->getType() }));

		auto ret_base = llvm::ConstantStruct::get(
			st_ty, std::vector<llvm::Constant *>(
		{ builder.getInt32(val.size()),
		llvm::Constant::getNullValue(rc->getType()) }));
		auto ret_llvm = builder.CreateInsertValue(ret_base, rc, { 1 });
		auto ret = VectorType(vals.at(0).type).create(ret_llvm);
		env.scope.add(ret);
		return ret;
	};

	virtual std::string as_string() const {
		
		if (val.at(0)->type() == DataType::Char) {
			std::string ret = "\"";
			for (const auto &i : val) {
				ret += i->as_string();
			}
			ret += "\"";
			return ret;
		}

		std::string ret = "( ";
		for (const auto &i : val) {
			ret += i->as_string();
			ret += ' ';
		}
		ret += ")";
		return ret;
	}
};
