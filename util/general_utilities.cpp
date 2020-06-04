/*

SIN Toolchain (x86 target)
general_utilities.cpp
Copyright 2020 Riley Lannon

*/

#include "general_utilities.h"

bool general_utilities::returns(StatementBlock to_check) {
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
                to_return = ite_returns(dynamic_cast<IfThenElse*>(s.get()));
			}

			// increment the iterator
			it++;
		}

		// return our value
		return to_return;
	}
}

bool general_utilities::ite_returns(IfThenElse *to_check) {
    // both must be true for it to return true
    bool if_returns;
    bool else_returns;
    
    // check the 'if' branch
    if (to_check->get_if_branch()->get_statement_type() == SCOPE_BLOCK) {
        ScopedBlock *if_branch = dynamic_cast<ScopedBlock*>(to_check->get_if_branch().get());
        if_returns = returns(if_branch->get_statements());
    }
    else if (to_check->get_if_branch()->get_statement_type() == RETURN_STATEMENT) {
        if_returns = true;
    }
    else {
        if_returns = false;
    }

    // check the else branch
    if (to_check->get_else_branch()) {
        if (to_check->get_else_branch()->get_statement_type() == SCOPE_BLOCK) {
            ScopedBlock* else_branch = dynamic_cast<ScopedBlock*>(to_check->get_else_branch().get());
            else_returns = returns(else_branch->get_statements());
        }
        else if (to_check->get_else_branch()->get_statement_type() == RETURN_STATEMENT) {
            else_returns = true;
        }
    }
    else {
        // otherwise, return false; if there is no else branch and there is no return statement in this block, then if the condition is false, we will not have a return value
        else_returns = false;
    }

    return if_returns && else_returns;
}

bool general_utilities::is_bitwise(exp_operator op) {
    /*

    is_bitwise
    Returns whether the operator is bitwise or not

    */

    return (op == BIT_AND || op == BIT_OR || op == BIT_XOR || op == BIT_NOT);
}
