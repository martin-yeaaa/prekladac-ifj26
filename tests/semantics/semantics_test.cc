// Semantics test
// Author: Martin Vaško

#include <gtest/gtest.h>


extern "C" {
#include "../../src/semantics.h"
#include "../../src/bst.h"
#include "../../src/symtable.h"
#include <string.h>
}


TEST(build_AST, node_init_w_symbol) {

  ASTNode tree;

  // init node equivalent to: "x"
  int err = AST_init_node(&tree, NODE_IDENTIFIER, DT_INTEGER, "x");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_IDENTIFIER);
  EXPECT_TRUE(tree->data_type == DT_INTEGER);
  EXPECT_TRUE(strcmp(tree->data.string_val, "x") == 0);
  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
}


TEST(build_AST, node_init_w_int_literal) {

  ASTNode tree;

  // init node equivalent to: "67"
  int err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "67");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_LITERAL);
  EXPECT_TRUE(tree->data_type == DT_INTEGER);
  EXPECT_TRUE(tree->data.int_val == 67);
  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
}

TEST(build_AST, node_init_w_int_literal_invalid) {

  ASTNode tree;

  // init node equivalent to: "2147483648"
  int err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "2147483648");
  // Integer is out of range
  ASSERT_TRUE(err == 6);

  // "A"
  err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "A");
  // Invalid value
  ASSERT_TRUE(err == 6);
  
  // String cannot be "", that is a programmming error, checked by assert()

  // init node equivalent to: "2147483647"
  err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "2147483647");
  // Integer is not out of range
  ASSERT_TRUE(err == 0);
}

TEST(build_AST, node_init_w_double_literal) {

  ASTNode tree;

  // init node equivalent to: "123.456"
  int err = AST_init_node(&tree, NODE_LITERAL, DT_DOUBLE, "123.456");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_LITERAL);
  EXPECT_TRUE(tree->data_type == DT_DOUBLE);

  EXPECT_DOUBLE_EQ(tree->data.double_val, 123.456);

  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
}

TEST(build_AST, node_init_w_double_literal_invalid) {

  ASTNode tree;

  // init node equivalent to: too large number
  int err = AST_init_node(&tree, NODE_LITERAL, DT_DOUBLE, "1211111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111113.456");
  ASSERT_TRUE(err == 6);

  // invalid value
  err = AST_init_node(&tree, NODE_LITERAL, DT_DOUBLE, "3.2 - that is three point two");
  ASSERT_TRUE(err == 6);
}

TEST(build_AST, node_init_w_string_literal) {

  ASTNode tree;

  // init node equivalent to: ""what how""
  int err = AST_init_node(&tree, NODE_LITERAL, DT_STRING, "what how");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_LITERAL);
  EXPECT_TRUE(tree->data_type == DT_STRING);

  EXPECT_TRUE(strcmp(tree->data.string_val, "what how") == 0);

  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);

  // One more node
  ASTNode second;
  
  // This can be empty string
  err = AST_init_node(&second, NODE_LITERAL, DT_STRING, "");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(second->node_type == NODE_LITERAL);
  EXPECT_TRUE(second->data_type == DT_STRING);

  EXPECT_TRUE(strcmp(second->data.string_val, "") == 0);

  EXPECT_TRUE(second->symbol_ptr == NULL);
  EXPECT_TRUE(second->child_arr == NULL);
  EXPECT_TRUE(second->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
}


TEST(build_AST, make_tree_var_def) {

  ASTNode definition;
  ASTNode variable;
  ASTNode integer;

  // convert into AST: var xd: Int = 5

  int err = AST_init_node(&definition, NODE_VAR_DEF, DT_INTEGER, NULL);
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&variable, NODE_IDENTIFIER, DT_INTEGER, "xd");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&integer, NODE_LITERAL, DT_INTEGER, "5");
  ASSERT_TRUE(err == 0);

  // Make tree by adding children separately
  err = AST_node_add_child(definition, variable);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(definition, integer);
  ASSERT_TRUE(err == 0);

  // Check tree
  EXPECT_TRUE(definition->child_arr[0]->node_type == NODE_IDENTIFIER);
  EXPECT_TRUE(definition->child_arr[1]->node_type == NODE_LITERAL);
}

TEST(build_AST, make_tree_fun_call) {

  ASTNode func_id;
  ASTNode func_call;
  ASTNode arg_list;
  ASTNode arg1;
  ASTNode arg2;
  ASTNode arg3;
  ASTNode arg4;
  ASTNode arg5;


  // convert into AST: func_id("too", "plus", 2, "equals", 4)

  // Init all nodes
  int err = AST_init_node(&func_id, NODE_IDENTIFIER, DT_INTEGER, "foo");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&func_call, NODE_FUNC_CALL, DT_UNDEF, NULL);
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg_list, NODE_ARG_LIST, DT_UNDEF, NULL);
  ASSERT_TRUE(err == 0);

  err = AST_init_node(&arg1, NODE_LITERAL, DT_STRING, "too");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg2, NODE_LITERAL, DT_STRING, "plus");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg3, NODE_LITERAL, DT_INTEGER, "2");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg4, NODE_LITERAL, DT_STRING, "equals");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg5, NODE_LITERAL, DT_INTEGER, "4");
  ASSERT_TRUE(err == 0);

  // Make tree by adding children separately
  err = AST_node_add_child(func_call, func_id);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(func_call, arg_list);
  ASSERT_TRUE(err == 0);

  err = AST_node_add_child(arg_list, arg1);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg2);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg3);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg4);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg5);
  ASSERT_TRUE(err == 0);


  // Check tree
  EXPECT_TRUE(func_call->node_type == NODE_FUNC_CALL);
  EXPECT_TRUE(func_call->child_arr[0]->node_type == NODE_IDENTIFIER);
  EXPECT_TRUE(func_call->child_arr[1]->node_type == NODE_ARG_LIST);

  EXPECT_TRUE(arg_list->child_arr[0]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[1]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[2]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[3]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[4]->node_type == NODE_LITERAL);

}



// AI TESTS BECAUSE IM TOO LAZY!! Also i could never write this wtf
// It has some small adjustments so i cover most of my code.

/*

PROMPT:

    Im making semantic analysis of abstract syntax tree in C lang. I want to test it.
    However, its very laborous to write all the code for instantiating an AST, like 
    super annoying and long. Can you write a fixture that works with googletest that 
    will make an AST of some simple program? What do you need to know so it works
    with my function calls and my AST structure?

Then there was a little back and forth where i fed it my code

*/
namespace ast {

// ---------- basic builders ----------
inline ASTNode N(NodeType t, DataType dt = DT_UNDEF, const char *data = nullptr) {
  ASTNode n = nullptr;
  if (AST_init_node(&n, t, dt, const_cast<char *>(data)) != SUCCESS)
    throw std::runtime_error("AST_init_node failed");
  return n;
}

inline void AddChild(ASTNode parent, ASTNode kid) {
  if (AST_node_add_child(parent, kid) != SUCCESS)
    throw std::runtime_error("AST_node_add_child failed");
}

// Adds children in order, returns parent
template <typename... C> ASTNode With(ASTNode parent, C... kids) {
  (AddChild(parent, kids), ...);
  return parent;
}

// ---------- leaves ----------
inline ASTNode Id(const char *name, DataType dt = DT_UNDEF) { return N(NODE_IDENTIFIER, dt, name); }
inline ASTNode Int(const char *v)  { return N(NODE_LITERAL, DT_INTEGER, v); }
inline ASTNode Dbl(const char *v)  { return N(NODE_LITERAL, DT_DOUBLE, v); }
inline ASTNode Str(const char *v)  { return N(NODE_LITERAL, DT_STRING, v); }
inline ASTNode Bool(const char *v) { return N(NODE_LITERAL, DT_BOOL, v); }
// UNIT
inline ASTNode Unit() { return N(NODE_LITERAL, DT_UNIT); }

// ---------- expressions ----------
// Explicit NODE_EXPR wrapper (only the TOP of an expression gets one)
inline ASTNode Expr(ASTNode inner) { return With(N(NODE_EXPR), inner); }

// Wraps in NODE_EXPR unless the node is already a NODE_EXPR, NODE_FUNC_CALL
// or NODE_BLOCK. Used wherever the grammar accepts "expr or call or block".
inline ASTNode Wrap(ASTNode n) {
  switch (n->node_type) {
  case NODE_EXPR:
  case NODE_FUNC_CALL:
  case NODE_BLOCK:
    return n;
  default:
    return Expr(n);
  }
}

inline ASTNode Bin(NodeType op, ASTNode l, ASTNode r) { return With(N(op), l, r); }
inline ASTNode Add(ASTNode l, ASTNode r) { return Bin(NODE_ADD, l, r); }
inline ASTNode Sub(ASTNode l, ASTNode r) { return Bin(NODE_SUB, l, r); }
inline ASTNode Mul(ASTNode l, ASTNode r) { return Bin(NODE_MUL, l, r); }
inline ASTNode Div(ASTNode l, ASTNode r) { return Bin(NODE_DIV, l, r); }
inline ASTNode Neg(ASTNode x)  { return With(N(NODE_UN_MINUS), x); }
inline ASTNode Plus(ASTNode x) { return With(N(NODE_UN_PLUS), x); }

// Condition: Cond(NODE_LT, lhs, rhs); operands are NOT wrapped in NODE_EXPR
inline ASTNode Cond(NodeType rel, ASTNode l, ASTNode r) {
  return With(N(NODE_COND), Bin(rel, l, r));
}

// Function call; args must be Id(...) or literals, added directly
template <typename... A> ASTNode Call(const char *name, A... args) {
  ASTNode list = With(N(NODE_ARG_LIST), args...);
  return With(N(NODE_FUNC_CALL), Id(name), list);
}

// ---------- statements / definitions ----------
// Statements are added as given (a Call(...) can be used as a statement)
template <typename... S> ASTNode Block(S... stmts) { return With(N(NODE_BLOCK), stmts...); }

// rhs can be an expression (auto-wrapped in NODE_EXPR) or a Call(...) (used as is)
inline ASTNode Var(const char *name, DataType dt, ASTNode rhs) {
  return With(N(NODE_VAR_DEF), Id(name, dt), Wrap(rhs));
}
inline ASTNode Val(const char *name, DataType dt, ASTNode rhs) {
  return With(N(NODE_VAL_DEF), Id(name, dt), Wrap(rhs));
}
inline ASTNode Assign(const char *name, ASTNode rhs) {
  return With(N(NODE_ASSIGNMENT), Id(name), Wrap(rhs));
}
// e: expression (auto-wrapped) or Call(...) (used as is)
inline ASTNode Return(ASTNode e) { return With(N(NODE_RETURN), Wrap(e)); }

inline ASTNode If(ASTNode cond, ASTNode then_b, ASTNode else_b) {
  return With(N(NODE_IF), cond, then_b, else_b);
}
inline ASTNode While(ASTNode cond, ASTNode body) { return With(N(NODE_WHILE), cond, body); }

// Params: Param("x", DT_INTEGER)
inline ASTNode Param(const char *name, DataType dt) { return Id(name, dt); }
template <typename... P> ASTNode Params(P... ps) { return With(N(NODE_PARAM_LIST), ps...); }

// Function def: body is a Block, a Call, or an expression (auto-wrapped)
inline ASTNode Func(const char *name, DataType ret, ASTNode params, ASTNode body) {
  return With(N(NODE_FUNC_DEF, ret), Id(name, ret), params, Wrap(body));
}
inline ASTNode MainFunc(ASTNode body) {
  return With(N(NODE_MAIN_FUNC_DEF, DT_UNIT), Id("main", DT_UNIT), Params(), Wrap(body));
}

template <typename... D> ASTNode Program(D... defs) { return With(N(NODE_PROGRAM), defs...); }

}  // namespace ast

// ---------- fixture ----------
class AstFixture : public ::testing::Test {
protected:
  ASTNode root = nullptr;
  SymbTables st;
  bool st_ready = false;

  // Override in derived fixtures to build other programs
  virtual ASTNode BuildProgram() {
    using namespace ast;
    /*
      def add(x: Int, y: Int): Int { return x + y }
      def twice(x: Int): Int { return add(x, x) }
      def main() {
        var a: Int = 1
        val b: Int = add(a, 2)
        if (a < b) { a = a + 1 } else { a = 0 }
        while (a < 10) { a = a + 1 }
      }
    */
    return Program(
        Func("add", DT_INTEGER, Params(Param("x", DT_INTEGER), Param("y", DT_INTEGER)),
             Block(Return(Add(Id("x"), Id("y"))))),
        Func("twice", DT_INTEGER, Params(Param("x", DT_INTEGER)),
             Block(Return(Call("add", Id("x"), Id("x"))))),
        MainFunc(Block(
            Var("a", DT_INTEGER, Int("1")),
            Val("b", DT_INTEGER, Call("add", Id("a"), Int("2"))),
            If(Cond(NODE_LT, Id("a"), Id("b")),
               Block(Assign("a", Add(Id("a"), Int("1")))),
               Block(Assign("a", Int("0")))),
            While(Cond(NODE_LT, Id("a"), Int("10")),
                  Block(Assign("a", Add(Id("a"), Int("1"))))))),
        Func("lolz", DT_UNIT, Params(), Expr(Unit())));
  }

  void SetUp() override {
    ASSERT_EQ(ST_init(&st), SUCCESS);
    st_ready = true;
    root = BuildProgram();
    ASSERT_NE(root, nullptr);
  }

  void TearDown() override {
    if (root) AST_destroy_tree(root);
    if (st_ready) ST_destroy(&st);
  }
};

// ---------- examples ----------
TEST_F(AstFixture, ValidProgramPasses) {
  EXPECT_EQ(semantic_analysis(root, &st), SUCCESS);

  // Check symbol table
  Symbol *sb = ST_get_symbol(&st, "main");
  ASSERT_TRUE(sb != NULL);
  EXPECT_TRUE(sb->symb_type == SB_MAIN_FUNC);
  EXPECT_TRUE(sb->data_type == DT_UNIT); // return value
  EXPECT_TRUE(sb->param_count == 0);

  EXPECT_TRUE(sb == root->child_arr[2]->child_arr[I_FD_ID]->symbol_ptr);


  sb = ST_get_symbol(&st, "add");
  ASSERT_TRUE(sb != NULL);
  EXPECT_TRUE(sb->symb_type == SB_FUNC);
  EXPECT_TRUE(sb->data_type == DT_INTEGER); // return value
  EXPECT_TRUE(sb->param_count == 2);
  EXPECT_TRUE(sb->param_type_arr[0] == DT_INTEGER);
  EXPECT_TRUE(sb->param_type_arr[1] == DT_INTEGER);

  EXPECT_TRUE(sb == root->child_arr[0]->child_arr[I_FD_ID]->symbol_ptr);

  sb = ST_get_symbol(&st, "twice");
  ASSERT_TRUE(sb != NULL);
  EXPECT_TRUE(sb->symb_type == SB_FUNC);
  EXPECT_TRUE(sb->data_type == DT_INTEGER); // return value
  EXPECT_TRUE(sb->param_count == 1);
  EXPECT_TRUE(sb->param_type_arr[0] == DT_INTEGER);

  EXPECT_TRUE(sb == root->child_arr[1]->child_arr[I_FD_ID]->symbol_ptr);

  sb = ST_get_symbol(&st, "lolz");
  ASSERT_TRUE(sb != NULL);
  EXPECT_TRUE(sb->symb_type == SB_FUNC);
  EXPECT_TRUE(sb->data_type == DT_UNIT); // return value
  EXPECT_TRUE(sb->param_count == 0);

  EXPECT_TRUE(sb == root->child_arr[3]->child_arr[I_FD_ID]->symbol_ptr);

}


// Function cannot have return without explicit typing
TEST_F(AstFixture, InvalidReturn) {
    ASTNode undef_func = root->child_arr[0];
  undef_func->data_type = DT_UNDEF;
  undef_func->child_arr[I_FD_ID]->data_type = DT_UNDEF;

  EXPECT_EQ(semantic_analysis(root, &st), ERR_OTHER_SEMANTIC);
}


TEST_F(AstFixture, MissingMain) {
    // Set main to normal fn
  ASTNode main_fn = root->child_arr[2];
  main_fn->node_type = NODE_FUNC_DEF;
  EXPECT_EQ(semantic_analysis(root, &st), ERR_UNDEF_SYM);
}

TEST_F(AstFixture, MultipleMain) {
    // Set normal fn to main
  ASTNode fn = root->child_arr[3]; // function lolz
  fn->node_type = NODE_MAIN_FUNC_DEF;
  EXPECT_EQ(semantic_analysis(root, &st), ERR_REDEFINITION);
}

TEST_F(AstFixture, BadMainParam) {
    // Set main to normal fn

  ASTNode bad_param;
  ErrCode err = AST_init_node(&bad_param, NODE_IDENTIFIER, DT_BOOL, "hi");
  ASSERT_TRUE(err == SUCCESS);

  ASTNode main_fn = root->child_arr[2];

  err = AST_node_add_child(main_fn->child_arr[I_FD_PAR], bad_param);
  ASSERT_TRUE(err == SUCCESS);

  EXPECT_EQ(semantic_analysis(root, &st), ERR_FUN_ARGS);
}

TEST_F(AstFixture, BadMainReturnType) {
    // Set main to normal fn

  ASTNode main_fn = root->child_arr[2];

  main_fn->data_type = DT_DOUBLE;
  main_fn->child_arr[I_FD_ID]->data_type = DT_DOUBLE;

  EXPECT_EQ(semantic_analysis(root, &st), ERR_INCOMPATIBLE_TYPE);
}



// Negative test: tweak the tree after building
TEST_F(AstFixture, UndefinedVariableFails) {
  ASTNode main_fn = root->child_arr[2];
  ASTNode body = main_fn->child_arr[I_FBODY];
  ASTNode var_a = body->child_arr[0];
  var_a->child_arr[0]->data.string_val[0] = 'z';  // rename definition -> uses of "a" undefined
  EXPECT_EQ(semantic_analysis(root, &st), ERR_UNDEF_SYM);
}



// Direct recursion: fact calls itself
class RecursionFixture : public AstFixture {
protected:
  ASTNode BuildProgram() override {
    using namespace ast;
    /* Technically invalid, infinite cycle but I don't care

      def fact(n: Int): Int 
        val m: Int = n - 1
        fact(m)
      
      def main() 
        val x: Int = fact(5)
      
    */
    return Program(
        Func("fact", DT_INTEGER, Params(Param("n", DT_INTEGER)),
             Block(Val("m", DT_INTEGER, Sub(Id("n"), Int("1"))),
                    Call("fact", Id("m")))),
        MainFunc(Block(   Val("x", DT_INTEGER, Call("fact", Int("5")))   )));
  }
};

TEST_F(RecursionFixture, DirectRecursionPasses) {
  EXPECT_EQ(semantic_analysis(root, &st), SUCCESS);
}



TEST_F(RecursionFixture, DirectRecursionInvalid) {

  // Invalid if type is undefined

  ASTNode rec_func = root->child_arr[0];
  rec_func->data_type = DT_UNDEF;

  EXPECT_EQ(semantic_analysis(root, &st), ERR_OTHER_SEMANTIC);
}


// Indirect recursion: a -> b -> c -> d -> a (no if, no return)
// Also tests using symbols before definitions.
class IndirectRecursionFixture : public AstFixture {
protected:
  ASTNode BuildProgram() override {
    using namespace ast;
    /*
      def a(n: Int): Int { val t: Int = n - 1; b(t) }
      def b(n: Int): Int = c(n)
      def c(n: Int): Int { val t: Int = n + 1; d(t) }
      def d(n: Int): Int = a(n)
      def main() { val x: Int = a(5) }
    */
    return Program(
        Func("a", DT_INTEGER, Params(Param("n", DT_INTEGER)),
             Block(Val("t", DT_INTEGER, Sub(Id("n"), Int("1"))),
                   Call("b", Id("t")))),
        Func("b", DT_INTEGER, Params(Param("n", DT_INTEGER)),
             Call("c", Id("n"))),
        Func("c", DT_INTEGER, Params(Param("n", DT_INTEGER)),
             Block(Val("t", DT_INTEGER, Add(Id("n"), Int("1"))),
                   Call("d", Id("t")))),
        Func("d", DT_INTEGER, Params(Param("n", DT_INTEGER)),
             Call("a", Id("n"))),
        MainFunc(Block(Val("x", DT_INTEGER, Call("a", Int("5"))))));
  }
};

TEST_F(IndirectRecursionFixture, IndirectRecursionPasses) {
  EXPECT_EQ(semantic_analysis(root, &st), SUCCESS);
}

TEST_F(IndirectRecursionFixture, IndirectRecursionInvalid) {
  
  // Program only checks indirect recursion if type of function isn't explicitly declared
  // Because only then is it invalid

  ASTNode undef_fn = root->child_arr[0];
  undef_fn->data_type = DT_UNDEF;

  EXPECT_EQ(semantic_analysis(root, &st), ERR_OTHER_SEMANTIC);
}

TEST_F(IndirectRecursionFixture, NoRecursion) {
  
  // Program only checks indirect recursion if type of function isn't explicitly declared
  // Because only then is it invalid

  ASTNode undef_fn = root->child_arr[0];
  undef_fn->data_type = DT_UNDEF;
  undef_fn->child_arr[0]->data_type = DT_UNDEF;

  ASTNode changed_fn = root->child_arr[3];

  char * change_call_name = changed_fn->child_arr[I_FBODY]->child_arr[I_FC_ID]->data.string_val;
  strcpy(change_call_name, "b");

  EXPECT_EQ(semantic_analysis(root, &st), SUCCESS);
}