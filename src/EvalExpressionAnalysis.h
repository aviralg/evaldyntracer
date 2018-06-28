#ifndef __EVAL_EXPRESSION_ANALYSIS_H__
#define __EVAL_EXPRESSION_ANALYSIS_H__

#include "Configuration.h"
#include "ExecutionContextStack.h"
#include "Rincludes.h"
#include "TableWriter.h"
#include <memory>

class EvalExpressionAnalysis {
  public:
    explicit EvalExpressionAnalysis(
        const Configuration &configuration,
        std::shared_ptr<ExecutionContextStack> execution_context_stack);
    void closure_entry(const FunctionExecutionContext &context);

  private:
    const Configuration configuration_;
    const std::shared_ptr<ExecutionContextStack> execution_context_stack_;
    TableWriter expression_table_writer_;
};

#endif /* __EVAL_EXPRESSION_ANALYSIS_H__ */
