// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: onufry@google.com (Onufry Wojtaszczyk)
//
// These tests only test the shortcuts provided in
// projecting_bound_expressions.cc, and not the main logic of evaluation, which
// is tested via the projecting_expressions tests.

#include "supersonic/expression/core/projecting_bound_expressions.h"
#include "supersonic/utils/std_namespace.h"

#include "supersonic/base/infrastructure/block.h"
#include "supersonic/base/infrastructure/projector.h"
#include "supersonic/base/infrastructure/tuple_schema.h"
#include "supersonic/base/memory/memory.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/testing/block_builder.h"
#include "supersonic/testing/comparators.h"
#include "gtest/gtest.h"
#include "supersonic/utils/container_literal.h"

namespace supersonic {
namespace {

using util::gtl::Container;

class ProjectingBoundExpressionsTest : public ::testing::Test {
 protected:
  ProjectingBoundExpressionsTest()
      : block_(CreateBlock()) {}

  TupleSchema schema() {
    return block_->view().schema();
  }

  const View& view() {
    return block_->view();
  }

  rowcount_t rows() {
    return view().row_count();
  }

 private:
  static unique_ptr<Block> CreateBlock() {
    return BlockBuilder<STRING, INT32, DOUBLE, INT32>()
        .AddRow("1", 12, 5.1, 22)
        .AddRow("2", 13, 6.2, 23)
        .AddRow("3", 14, 7.3, 23)
        .AddRow("4", __, 8.4, 24)
        .AddRow(__,  16, __,  26)
        .Build();
  }

  unique_ptr<Block> block_;
};

// TODO(onufry): add some framework to reuse code.
TEST_F(ProjectingBoundExpressionsTest, BoundAttributeAt) {
  FailureOrOwned<BoundExpression> attribute_at =
      BoundAttributeAt(schema(), 2);
  ASSERT_TRUE(attribute_at.is_success());
  EXPECT_EQ(Container("col2").As<set<string> >(),
            attribute_at->referred_attribute_names());
  FailureOrOwned<BoundExpressionTree> tree_attribute_at =
      CreateBoundExpressionTree(attribute_at.move(),
                                HeapBufferAllocator::Get(), 10);
  ASSERT_TRUE(tree_attribute_at.is_success());
  EvaluationResult result = tree_attribute_at->Evaluate(view());
  ASSERT_TRUE(result.is_success());
  EXPECT_EQ(1, result.get().column_count());
  EXPECT_COLUMNS_EQUAL(view().column(2), result.get().column(0), rows());
}

TEST_F(ProjectingBoundExpressionsTest, BoundNamedAttribute) {
  FailureOrOwned<BoundExpression> named_attribute =
      BoundNamedAttribute(schema(), "col3");
  ASSERT_TRUE(named_attribute.is_success());
  EXPECT_EQ(Container("col3").As<set<string> >(),
            named_attribute->referred_attribute_names());
  FailureOrOwned<BoundExpressionTree> tree_named_attribute =
      CreateBoundExpressionTree(named_attribute.move(),
                                HeapBufferAllocator::Get(), 10);
  ASSERT_TRUE(tree_named_attribute.is_success());
  EvaluationResult result = tree_named_attribute->Evaluate(view());
  ASSERT_TRUE(result.is_success());
  EXPECT_EQ(1, result.get().column_count());
  EXPECT_COLUMNS_EQUAL(view().column(3), result.get().column(0), rows());
}

TEST_F(ProjectingBoundExpressionsTest, BoundAlias) {
  FailureOrOwned<BoundExpression> named_attribute =
      BoundNamedAttribute(schema(), "col3");
  ASSERT_TRUE(named_attribute.is_success());
  FailureOrOwned<BoundExpression> alias =
      BoundAlias("Brand New Name", named_attribute.move(),
                 HeapBufferAllocator::Get(), 4);
  ASSERT_TRUE(alias.is_success());
  FailureOrOwned<BoundExpressionTree> tree_alias =
      CreateBoundExpressionTree(alias.move(),
                                HeapBufferAllocator::Get(), 10);
  ASSERT_TRUE(tree_alias.is_success());
  EvaluationResult result = tree_alias->Evaluate(view());
  ASSERT_TRUE(result.is_success());
  EXPECT_EQ(1, result.get().column_count());
  EXPECT_COLUMNS_EQUAL(view().column(3), result.get().column(0), rows());
}

TEST_F(ProjectingBoundExpressionsTest,
       ProjectionExpressionCollectReferredAttributeNames) {
  auto expression_list = make_unique<BoundExpressionList>(
      SucceedOrDie(BoundNamedAttribute(schema(), "col0")),
      SucceedOrDie(BoundNamedAttribute(schema(), "col1")),
      SucceedOrDie(BoundNamedAttribute(schema(), "col2")),
      SucceedOrDie(BoundNamedAttribute(schema(), "col3"))
  );

  vector<TupleSchema> schemas;
  for (auto const& expr: *expression_list) {
    schemas.push_back(expr->result_schema());
  }

  auto projector = make_unique<BoundMultiSourceProjector>(schemas);
  projector->Add(3, 0);
  projector->Add(0, 0);
  projector->Add(1, 0);
  projector->Add(3, 0);
  projector->Add(1, 0);

  FailureOrOwned<BoundExpression> projection_expression =
      BoundProjection(std::move(projector), std::move(expression_list));
  ASSERT_TRUE(projection_expression.is_success())
      << projection_expression.exception().PrintStackTrace();
  EXPECT_EQ(Container("col3", "col0", "col1", "col2").As<set<string> >(),
            projection_expression->referred_attribute_names());
}

TEST_F(ProjectingBoundExpressionsTest,
       ProjectionExpressionPartialCollectReferredAttributeNames) {
  auto source_list_1 = make_unique<BoundExpressionList>();
  source_list_1->add(SucceedOrDie(BoundNamedAttribute(schema(), "col0")));
  source_list_1->add(SucceedOrDie(BoundNamedAttribute(schema(), "col1")));
  FailureOrOwned<BoundExpression> source_1(
      BoundCompoundExpression(std::move(source_list_1)));
  ASSERT_TRUE(source_1.is_success())
      << source_1.exception().PrintStackTrace();

  auto source_list_2 = make_unique<BoundExpressionList>();
  source_list_2->add(SucceedOrDie(BoundNamedAttribute(schema(), "col2")));
  source_list_2->add(SucceedOrDie(BoundNamedAttribute(schema(), "col3")));
  FailureOrOwned<BoundExpression> source_2(
      BoundCompoundExpression(std::move(source_list_2)));
  ASSERT_TRUE(source_2.is_success())
      << source_2.exception().PrintStackTrace();

  vector<TupleSchema> schemas{
      source_1->result_schema(),
      source_2->result_schema(),
  };

  auto expression_list = make_unique<BoundExpressionList>(
      source_1.move(),
      source_2.move()
  );

  auto projector = make_unique<BoundMultiSourceProjector>(schemas);
  projector->Add(0, 1);
  projector->Add(1, 0);

  FailureOrOwned<BoundExpression> projection_expression =
      BoundProjection(std::move(projector), std::move(expression_list));
  ASSERT_TRUE(projection_expression.is_success())
      << projection_expression.exception().PrintStackTrace();
  EXPECT_EQ(Container("col0", "col1", "col2", "col3").As<set<string> >(),
            projection_expression->referred_attribute_names());
}

// TODO(onufry): Add tests for CompoundExpression and RenameCompoundExpression.

}  // namespace
}  // namespace supersonic
