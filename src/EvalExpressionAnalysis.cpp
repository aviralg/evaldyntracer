#include "EvalExpressionAnalysis.h"

EvalExpressionAnalysis::EvalExpressionAnalysis(
    const Configuration &configuration,
    std::shared_ptr<ExecutionContextStack> execution_context_stack)
    : configuration_{configuration},
      execution_context_stack_{std::move(execution_context_stack)},
      expression_table_writer_{configuration.get_raw_analysis_dirpath() + "/" +
                                   "eval-expressions.tdf",
                               {"eval_type", "caller", "caller_type",
                                "callsite", "function", "expression_type",
                                "symbols", "applications", "size",
                                "expression"}} {}

void EvalExpressionAnalysis::closure_entry(
    const ClosureExecutionContext &context) {

    for (auto &key_value : configuration_.get_evals()) {

        const std::string eval_name = key_value.first;
        const SEXP eval_function = key_value.second;

        if (context.get_function() == eval_function) {

            SEXP value = context.get_argument("expr", true);

            if (value == R_UnboundValue) {
                dyntrace_log_error(
                    "'expr' not bound to a value in a call to '%s'",
                    eval_name.c_str());
            }

            sexptype_t type = TYPEOF(value);
            SEXP function = R_NilValue;

            if (type == LANGSXP) {
                function = CAR(value);
            }

            const auto[caller_name, caller_type] = get_caller_information();

            const auto[symbol_count, application_count] =
                get_expression_complexity(value);

            std::string expression{serialize_sexp(value)};

            expression_table_writer_.write_row(
                eval_name, caller_name, sexptype_to_string(caller_type),
                std::string(serialize_sexp(context.get_call())),
                std::string(serialize_sexp(function)), sexptype_to_string(type),
                symbol_count, application_count, expression.length(),
                expression);

            break;
        }
    }
}

std::tuple<std::string, sexptype_t>
EvalExpressionAnalysis::get_caller_information() {
    std::string caller_name;
    sexptype_t caller_type;

    auto last_context =
        execution_context_stack_->get_last_execution_context(CLOSXP);

    if (last_context) {
        auto closure_context = std::get<ClosureExecutionContext>(*last_context);
        caller_name = closure_context.get_name();
        caller_type = closure_context.get_type();
    } else {
        auto top_level_context =
            execution_context_stack_->get_top_level_execution_context();
        caller_name = top_level_context.get_name();
        caller_type = top_level_context.get_type();
    }

    return std::make_tuple(caller_name, caller_type);
}

std::tuple<size_t, size_t>
EvalExpressionAnalysis::get_expression_complexity(const SEXP expression) {
    size_t symbol_count = 0;
    size_t application_count = 0;
    switch (TYPEOF(expression)) {
        case LANGSXP:
            ++application_count;
            ++symbol_count;
            for (SEXP argument = CDR(expression); argument != R_NilValue;
                 argument = CDR(argument)) {
                const auto[child_symbol_count, child_application_count] =
                    get_expression_complexity(CAR(argument));
                symbol_count += child_symbol_count;
                application_count += child_application_count;
            }
            break;
        case SYMSXP:
            ++symbol_count;
            break;
        default:
            break;
    }
    return std::make_tuple(symbol_count, application_count);
}
