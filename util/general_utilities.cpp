/*

SIN Toolchain (x86 target)
general_utilities.cpp
Copyright 2020 Riley Lannon

*/

#include "general_utilities.h"

bool general_utilities::returns(const StatementBlock& to_check) {
	/*
	
	returns
	Checks whether a given AST returns a value
	
	*/

	if (to_check.has_return) {
		return true;
	} else {
		// sentinel variable
		bool to_return = true;

		// iterate through statements to see if we have an if/else block; if so, check *those* for return values
		std::vector<std::shared_ptr<Statement>>::iterator it = to_check.statements_list.begin();
		while (it != to_check.statements_list.end() && to_return) {
			// get the statement pointer
            std::shared_ptr<Statement> s = *it;

            // handle ite
			if (s->get_statement_type() == stmt_type::IF_THEN_ELSE) {
                to_return = ite_returns(static_cast<IfThenElse*>(s.get()));
			}

			// increment the iterator
			it++;
		}

		// return our value
		return to_return;
	}
}

bool general_utilities::returns(const Statement &to_check) {
    if (to_check.get_statement_type() == SCOPE_BLOCK) {
        ScopedBlock &block = static_cast<ScopedBlock&>(to_check);
        return returns(block.get_statements());
    }
    else {
        return to_check.get_statement_type() == RETURN_STATEMENT;
    }
}

bool general_utilities::ite_returns(const IfThenElse *to_check) {
    // both must be true for it to return true
    bool if_returns = false;
    if (to_check->get_if_branch())
        if_returns = returns(*to_check->get_if_branch());
    
    bool else_returns = false;
    if (to_check->get_else_branch())
        else_returns = returns(*to_check->get_else_branch());

    return if_returns && else_returns;
}

bool general_utilities::is_bitwise(const exp_operator op) {
    /*

    is_bitwise
    Returns whether the operator is bitwise or not

    */

    return (op == BIT_AND || op == BIT_OR || op == BIT_XOR || op == BIT_NOT);
}
