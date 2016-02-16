//! \file
/*
**  Copyright (C) - Triton
**
**  This program is under the terms of the LGPLv3 License.
*/

#ifdef TRITON_PYTHON_BINDINGS

#include <api.hpp>
#include <pythonUtils.hpp>
#include <ast.hpp>
#include <pythonObjects.hpp>

#ifdef __unix__
  #include <python2.7/Python.h>
#elif _WIN32
  #include <Python.h>
#endif



/*! \page py_ast_page SMT2-Lib
    \brief [**python api**] All information about the ast python module.
    \anchor ast

\tableofcontents

\section ast_py_description Description
<hr>

The [SMT-LIB](http://smtlib.cs.uiowa.edu/) is an international initiative aimed at facilitating research and development in Satisfiability Modulo Theories (SMT).

~~~~~~~~~~~~~{.py}
>>> from triton import ast
>>> ast
<module 'ast' (built-in)>
~~~~~~~~~~~~~

Triton uses the SMT2-Lib to represent the instruction's semantics. Then, via the API you will be able to build and to modify your own symbolic expressions.

~~~~~~~~~~~~~{.asm}
Instruction:  add rax, rdx
Expression:   #41 = (bvadd ((_ extract 63 0) #40) ((_ extract 63 0) #39))
~~~~~~~~~~~~~

As all Triton's expressions are on [SSA form](http://en.wikipedia.org/wiki/Static_single_assignment_form), the id `#41` is the new expression of the `RAX`
register, the id `#40` is the previous expression of the `RAX` register and the id `#39` is the previous expression of the `RDX` register.
An \ref py_Instruction_page may contain several expressions (\ref py_SymbolicExpression_page). For example, the previous `add rax, rdx` instruction contains
7 expressions: 1 `ADD` semantics and 6 flags (`AF, CF, OF, PF, SF and ZF`) semantics where each flag is stored in a new \ref py_SymbolicExpression_page.

~~~~~~~~~~~~~{.asm}
Instruction: add rax, rdx
Expressions: #41 = (bvadd ((_ extract 63 0) #40) ((_ extract 63 0) #39))
             #42 = (ite (= (_ bv16 64) (bvand (_ bv16 64) (bvxor #41 (bvxor ((_ extract 63 0) #40) ((_ extract 63 0) #39))))) (_ bv1 1) (_ bv0 1))
             #43 = (ite (bvult #41 ((_ extract 63 0) #40)) (_ bv1 1) (_ bv0 1))
             #44 = (ite (= ((_ extract 63 63) (bvand (bvxor ((_ extract 63 0) #40) (bvnot ((_ extract 63 0) #39))) (bvxor ((_ extract 63 0) #40) #41))) (_ bv1 1)) (_ bv1 1) (_ bv0 1))
             #45 = (ite (= (parity_flag ((_ extract 7 0) #41)) (_ bv0 1)) (_ bv1 1) (_ bv0 1))
             #46 = (ite (= ((_ extract 63 63) #41) (_ bv1 1)) (_ bv1 1) (_ bv0 1))
             #47 = (ite (= #41 (_ bv0 64)) (_ bv1 1) (_ bv0 1))
~~~~~~~~~~~~~

Triton deals with 64-bits registers (and 128-bits for SSE). It means that it uses the `concat` and `extract` functions when operations are performed on subregister.

~~~~~~~~~~~~~{.asm}
mov al, 0xff  -> #193 = (concat ((_ extract 63 8) #191) (_ bv255 8))
movsx eax, al -> #195 = ((_ zero_extend 32) ((_ sign_extend 24) ((_ extract 7 0) #193)))
~~~~~~~~~~~~~

On the line 1, a new 64bit-vector is created with the concatenation of `RAX[63..8]` and the concretization of the value `0xff`. On the line 2, according
to the AMD64 behavior, if a 32-bit register is written, the CPU clears the 32-bit MSB of the corresponding register. So, in this case, we apply a sign
extension from al to `EAX`, then a zero extension from `EAX` to `RAX`.

\section ast_representation AST representation
<hr>

An abstract representation tree ([AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree)) is a representation of a grammar as tree. Triton uses its own SMT
representation as AST for all symbolic expressions. As all SMT expressions are built at runtime, an AST is available at each program point. For example,
let assume this set of instructions:

~~~~~~~~~~~~~{.asm}
mov al, 1
mov cl, 10
mov dl, 20
xor cl, dl
add al, cl
~~~~~~~~~~~~~

At the line 5, the AST of the `AL` register looks like this:

<p align="center"><img width="400" src="http://triton.quarkslab.com/files/smt_ast.svg"/></p>

This AST represents the semantics of the `AL` register at the program point 5 from the program point 1. Note that this AST has been simplified for
a better comprehension. The real AST contains some `concat` and `extract` as mentioned in the previous chapter. According to the API you can build
your own AST or to modify an AST given by Triton and perform some modifications and simplifications before sending it to the solver.

\section ast_grammar The SMT's grammar slightly modified

To manage easiest the subgraphs and to keep the SSA form of registers and memory, we have been constraint to modify slightly the grammar
of the SMT. Actually, only one kind of node has been added in the grammar. We have added a `REFERENCE` node which is a "terminate" node of a
graph but contains a reference to a subgraph. Below an example of one "partial" graph linked with two others subgraphs.

<p align="center"><img width="600" src="http://triton.quarkslab.com/files/smt_ast_ref.svg"/></p>

If you try to go through the full AST you will fail at the first reference node because a reference node does not contains child nodes.
The only way to jump from a reference node to the targeted node is to use the triton::engines::symbolic::SymbolicEngine::getFullAst() function.

~~~~~~~~~~~~~{.py}
[IN]  zfId = getSymbolicRegisterId(REG.ZF)
[IN]  partialGraph = getSymbolicExpressionFromId(zfId).getAst()
[IN]  print partialGraph
[OUT] (ite (= #89 (_ bv0 32)) (_ bv1 1) (_ bv0 1))

[IN]  fullGraph = getFullAst(partialGraph)
[IN]  print fullGraph
[OUT] (ite (= (bvsub ((_ extract 31 0) ((_ zero_extend 32) ((_ extract 31 0) ((_ zero_extend 32) (bvxor ((_ extract 31 0) ((_ zero_extend 32) (bvsub ((_ extract 31 0) ((_ zero_extend 32) ((_ sign_extend 24) ((_ extract 7 0) SymVar_0)))) (_ bv1 32)))) (_ bv85 32)))))) ((_ extract 31 0) ((_ zero_extend 32) ((_ sign_extend 24) ((_ extract 7 0) ((_ zero_extend 32) ((_ zero_extend 24) ((_ extract 7 0) (_ bv49 8))))))))) (_ bv0 32)) (_ bv1 1) (_ bv0 1))
~~~~~~~~~~~~~

\section ast_py_example Examples
<hr>

\subsection ast_py_example_1 Get a register's expression and create an assert

~~~~~~~~~~~~~{.py}
# Get the symbolic expression of the ZF flag
zfId    = getSymbolicRegisterId(REG.ZF)
zfExpr  = getFullAst(getSymbolicExpressionFromId(zfId).getAst())

# (assert (= zf True))
newExpr = ast.assert_(
            ast.equal(
                zfExpr,
                ast.bvtrue()
            )
          )

# Get a model
models  = getModel(newExpr)
~~~~~~~~~~~~~


\subsection ast_py_example_2 Play with the AST

~~~~~~~~~~~~~{.py}
# Node information

>>> node = bvadd(bv(1, 8), bvxor(bv(10, 8), bv(20, 8)))
>>> print type(node)
<type 'SmtAstNode'>

>>> print node
(bvadd (_ bv1 8) (bvxor (_ bv10 8) (_ bv20 8)))

>>> subchild = node.getChilds()[1].getChilds()[0]
>>> print subchild
(_ bv10 8)

>>> print subchild.getChilds()[0].getValue()
10
>>> print subchild.getChilds()[1].getValue()
8

# Node modification

>>> node = bvadd(bv(1, 8), bvxor(bv(10, 8), bv(20, 8)))
>>> print node
(bvadd (_ bv1 8) (bvxor (_ bv10 8) (_ bv20 8)))

>>> node.setChild(0, bv(123, 8))
>>> print node
(bvadd (_ bv123 8) (bvxor (_ bv10 8) (_ bv20 8)))
~~~~~~~~~~~~~

\subsection ast_py_example_3 Python operators

~~~~~~~~~~~~~{.py}
>>> a = bv(1, 8)
>>> b = bv(2, 8)
>>> c = (a & ~b) | (~a & b)
>>> print c
(bvor (bvand (_ bv1 8) (bvnot (_ bv2 8))) (bvand (bvnot (_ bv1 8)) (_ bv2 8)))
~~~~~~~~~~~~~

As we can't overload all SMT2-Lib's operators only these following operators are overloaded:

Python's Operator | SMT2-Lib format
------------------|------------------
a + b             | (bvadd a b)
a - b             | (bvsub a b)
a \* b            | (bvmul a b)
a \ b             | (bvsdiv a b)
a \| b            | (bvor a b)
a & b             | (bvand a b)
a ^ b             | (bvxor a b)
a % b             | (bvsmod a b)
a << b            | (bvshl a b)
a \>> b           | (bvlshr a b)
~a                | (bvnot a)
-a                | (bvneg a)

\section ast_py_api Python API - Methods of the ast module
<hr>

- **bv(integer value, integer size)**<br>
Returns the ast `triton::ast::bv()` representation as \ref py_SmtAstNode_page. The `size` is in bits.<br>
e.g: `(_ bvValue size)`.

- **bvadd(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvadd()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvadd expr1 epxr2)`.

- **bvand(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvand()` as \ref py_SmtAstNode_page.<br>
e.g: `(bvand expr1 epxr2)`.

- **bvashr(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvashr()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvashr expr1 epxr2)`.

- **bvdecl(integer size)**<br>
Returns the ast `triton::ast::bvdecl()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(_ BitVec size)`.

- **bvfalse()**<br>
This is an alias on the `(_ bv0 1)` ast expression.

- **bvlshr(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvlshr()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(lshr expr1 epxr2)`.

- **bvmul(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvmul()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvmul expr1 expr2)`.

- **bvnand(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvnand()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvnand expr1 expr2)`.

- **bvneg(SmtAstNode expr1)**<br>
Returns the ast `triton::ast::bvneg()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvneg expr1)`.

- **bvnor(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvnor()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvnor expr1 expr2)`.

- **bvnot(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvnot()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvnot expr1)`.

- **bvor(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvor()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvor expr1 expr2)`.

- **bvror(integer displacement, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvror()` representation as \ref py_SmtAstNode_page.<br>
e.g: `((_ rotate_right displacement) expr)`.

- **bvrol(integer displacement, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvrol()` representation as \ref py_SmtAstNode_page.<br>
e.g: `((_ rotate_left displacement) expr)`.

- **bvsdiv(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvsdiv()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvsdiv expr1 epxr2)`.

- **bvsge(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvsge()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvsge expr1 epxr2)`.

- **bvsgt(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvsgt()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvsgt expr1 epxr2)`.

- **bvshl(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvshl()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvshl expr1 expr2)`.

- **bvsle(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvsle()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvsle expr1 epxr2)`.

- **bvslt(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvslt()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvslt expr1 epxr2)`.

- **bvsmod(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvsmod()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvsmod expr1 expr2)`.

- **bvsrem(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvsrem()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvsrem expr1 expr2)`.

- **bvsub(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvsub()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvsub expr1 epxr2)`.

- **bvtrue()**<br>
This is an alias on the `(_ bv1 1)` ast expression.

- **bvudiv(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvudiv()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvudiv expr1 epxr2)`.

- **bvuge(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvuge()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvuge expr1 epxr2)`.

- **bvugt(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvugt()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvugt expr1 epxr2)`.

- **bvule(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvule()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvule expr1 epxr2)`.

- **bvult(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvult()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvult expr1 epxr2)`.

- **bvurem(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvurem()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvurem expr1 expr2)`.

- **bvxnor(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvxnor()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvxnor expr1 expr2)`.

- **bvxor(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::bvxor()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(bvxor expr1 epxr2)`.

- **compound([SmtAstNode expr ...])**<br>
Returns the `triton::ast::compound()` node as \ref py_SmtAstNode_page.

- **concat([SmtAstNode expr ...])**<br>
Returns the `triton::ast::concat()` node as \ref py_SmtAstNode_page.

- **distinct(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::distinct()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(distinct expr1 expr2)`

- **equal(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::equal()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(= expr1 epxr2)`.

- **extract(integer high, integer low, SmtAstNode expr1)**<br>
Returns the ast `triton::ast::extract()` representation as \ref py_SmtAstNode_page.<br>
e.g: `((_ extract high low) expr1)`.

- **ite(SmtAstNode ifExpr, SmtAstNode thenExpr, SmtAstNode elseExpr)**<br>
Returns the ast `triton::ast::ite()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(ite ifExpr thenExpr elseExpr)`.

- **land(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::land()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(and expr1 expr2)`.

- **let(string alias, SmtAstNode expr2, SmtAstNode expr3)**<br>
Returns the ast `triton::ast::let()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(let ((alias expr2)) expr3)`.

- **lnot(SmtAstNode expr)**<br>
Returns the ast `triton::ast::lnot()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(not expr)`.

- **lor(SmtAstNode expr1, SmtAstNode expr2)**<br>
Returns the ast `triton::ast::lor()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(or expr1 expr2)`.

- **reference(integer exprId)**<br>
Returns the reference (`triton::ast::reference()`) node representation as \ref py_SmtAstNode_page.
Be careful, the targeted node reference is always on the max vector size, except for volatile
expressions.<br>
e.g: `#123`.

- **assert_(SmtAstNode expr1)**<br>
Returns the ast `triton::ast::assert_()` representation as \ref py_SmtAstNode_page.<br>
e.g: `(assert expr1)`.

- **string(string s)**<br>
Returns a `triton::ast::string()` node as \ref py_SmtAstNode_page.

- **sx(integer sizeExt, SmtAstNode expr1)**<br>
Returns the ast `triton::ast::sx()` representation as \ref py_SmtAstNode_page.<br>
e.g: `((_ sign_extend sizeExt) expr1)`.

- **variable(string s)**<br>
Returns a `triton::ast::variable()` node as \ref py_SmtAstNode_page.

- **zx(integer sizeExt, SmtAstNode expr1)**<br>
Returns the ast `triton::ast::zx()` representation as \ref py_SmtAstNode_page.<br>
e.g: `((_ zero_extend sizeExt) expr1)`.

*/



namespace triton {
  namespace bindings {
    namespace python {


      static PyObject* ast_bv(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || (!PyLong_Check(op1) && !PyInt_Check(op1)))
          return PyErr_Format(PyExc_TypeError, "bv(): expected an integer as first argument");

        if (op2 == nullptr || (!PyLong_Check(op2) && !PyInt_Check(op2)))
          return PyErr_Format(PyExc_TypeError, "bv(): expected an integer as second argument");

        try {
          return PySmtAstNode(triton::ast::bv(PyLong_AsUint128(op1), PyLong_AsUint(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvadd(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvadd(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvadd(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvadd(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvand(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvand(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvand(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvand(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvashr(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvashr(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvashr(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvashr(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvdecl(PyObject* self, PyObject* size) {
        if (size == nullptr || (!PyLong_Check(size) && !PyInt_Check(size)))
          return PyErr_Format(PyExc_TypeError, "bvdecl(): expected an integer as argument");

        try {
          return PySmtAstNode(triton::ast::bvdecl(PyLong_AsUint(size)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvfalse(PyObject* self, PyObject* args) {
        try {
          return PySmtAstNode(triton::ast::bvfalse());
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvlshr(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvlshr(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvlshr(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvlshr(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvmul(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvmul(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvmul(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvmul(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvnand(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvnand(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvnand(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvnand(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvneg(PyObject* self, PyObject* op1) {
        if (!PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvneg(): expected a SmtAstNode as first argument");

        try {
          return PySmtAstNode(triton::ast::bvneg(PySmtAstNode_AsSmtAstNode(op1)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvnor(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvnor(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvnor(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvnor(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvnot(PyObject* self, PyObject* op1) {
        if (!PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvnot(): expected a SmtAstNode as first argument");

        try {
          return PySmtAstNode(triton::ast::bvnot(PySmtAstNode_AsSmtAstNode(op1)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvor(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvor(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvor(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvor(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvror(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || (!PyLong_Check(op1) && !PyInt_Check(op1)))
          return PyErr_Format(PyExc_TypeError, "bvror(): expected an integer as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvror(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvror(PyLong_AsUint(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvrol(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || (!PyLong_Check(op1) && !PyInt_Check(op1)))
          return PyErr_Format(PyExc_TypeError, "bvrol(): expected a integer as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvrol(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvrol(PyLong_AsUint(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvsdiv(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvsdiv(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvsdiv(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvsdiv(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvsge(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvsge(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvsge(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvsge(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvsgt(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvsgt(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvsgt(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvsgt(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvshl(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvshl(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvshl(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvshl(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvsle(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvsle(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvsle(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvsle(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvslt(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvslt(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvslt(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvslt(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvsmod(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvsmod(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvsmod(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvsmod(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvsrem(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvsrem(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvsrem(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvsrem(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvsub(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvsub(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvsub(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvsub(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvtrue(PyObject* self, PyObject* args) {
        try {
          return PySmtAstNode(triton::ast::bvtrue());
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvudiv(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvudiv(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvudiv(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvudiv(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvuge(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvuge(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvuge(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvuge(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvugt(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvugt(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvugt(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvugt(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvule(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvule(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvule(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvule(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvult(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvult(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvult(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvult(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvurem(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvurem(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvurem(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvurem(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvxnor(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvxnor(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvxnor(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvxnor(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_bvxor(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "bvxor(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "bvxor(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::bvxor(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_distinct(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "distinct(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "distinct(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::distinct(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_compound(PyObject* self, PyObject* exprsList) {
        std::vector<triton::ast::AbstractNode *> exprs;

        if (exprsList == nullptr || !PyList_Check(exprsList))
          return PyErr_Format(PyExc_TypeError, "compound(): expected a list of SmtAstNodes as first argument");

        /* Check if the mems list contains only integer item and craft a std::list */
        for (Py_ssize_t i = 0; i < PyList_Size(exprsList); i++){
          PyObject* item = PyList_GetItem(exprsList, i);

          if (!PySmtAstNode_Check(item))
            return PyErr_Format(PyExc_TypeError, "compound(): Each element from the list must be a SmtAstNode");

          exprs.push_back(PySmtAstNode_AsSmtAstNode(item));
        }

        try {
          return PySmtAstNode(triton::ast::compound(exprs));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_concat(PyObject* self, PyObject* exprsList) {
        std::vector<triton::ast::AbstractNode *> exprs;

        if (exprsList == nullptr || !PyList_Check(exprsList))
          return PyErr_Format(PyExc_TypeError, "concat(): expected a list of SmtAstNodes as first argument");

        /* Check if the list contains only PySmtAstNode */
        for (Py_ssize_t i = 0; i < PyList_Size(exprsList); i++){
          PyObject* item = PyList_GetItem(exprsList, i);

          if (!PySmtAstNode_Check(item))
            return PyErr_Format(PyExc_TypeError, "concat(): Each element from the list must be a SmtAstNode");

          exprs.push_back(PySmtAstNode_AsSmtAstNode(item));
        }

        try {
          return PySmtAstNode(triton::ast::concat(exprs));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_equal(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "equal(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "equal(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::equal(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_extract(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;
        PyObject* op3 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OOO", &op1, &op2, &op3);

        if (op1 == nullptr || (!PyLong_Check(op1) && !PyInt_Check(op1)))
          return PyErr_Format(PyExc_TypeError, "extract(): expected an integer as first argument");

        if (op2 == nullptr || (!PyLong_Check(op2) && !PyInt_Check(op2)))
          return PyErr_Format(PyExc_TypeError, "extract(): expected an integer as second argument");

        if (op3 == nullptr || !PySmtAstNode_Check(op3))
          return PyErr_Format(PyExc_TypeError, "extract(): expected a SmtAstNode as third argument");

        try {
          return PySmtAstNode(triton::ast::extract(PyLong_AsUint(op1), PyLong_AsUint(op2), PySmtAstNode_AsSmtAstNode(op3)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_ite(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;
        PyObject* op3 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OOO", &op1, &op2, &op3);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "ite(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "ite(): expected a SmtAstNode as second argument");

        if (op3 == nullptr || !PySmtAstNode_Check(op3))
          return PyErr_Format(PyExc_TypeError, "ite(): expected a SmtAstNode as third argument");

        try {
          return PySmtAstNode(triton::ast::ite(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2), PySmtAstNode_AsSmtAstNode(op3)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_land(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "land(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "land(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::land(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_let(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;
        PyObject* op3 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OOO", &op1, &op2, &op3);

        if (op1 == nullptr || !PyString_Check(op1))
          return PyErr_Format(PyExc_TypeError, "let(): expected a string as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "let(): expected a SmtAstNode as second argument");

        if (op3 == nullptr || !PySmtAstNode_Check(op3))
          return PyErr_Format(PyExc_TypeError, "let(): expected a SmtAstNode as third argument");

        try {
          return PySmtAstNode(triton::ast::let(PyString_AsString(op1), PySmtAstNode_AsSmtAstNode(op2), PySmtAstNode_AsSmtAstNode(op3)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_lnot(PyObject* self, PyObject* expr) {
        if (expr == nullptr || !PySmtAstNode_Check(expr))
          return PyErr_Format(PyExc_TypeError, "lnot(): expected a SmtAstNode as argument");

        try {
          return PySmtAstNode(triton::ast::lnot(PySmtAstNode_AsSmtAstNode(expr)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_lor(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || !PySmtAstNode_Check(op1))
          return PyErr_Format(PyExc_TypeError, "lor(): expected a SmtAstNode as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "lor(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::lor(PySmtAstNode_AsSmtAstNode(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_reference(PyObject* self, PyObject* exprId) {
        if (!PyInt_Check(exprId) && !PyLong_Check(exprId))
          return PyErr_Format(PyExc_TypeError, "reference(): expected an integer as argument");

        if (!triton::api.isSymbolicExpressionIdExists(PyLong_AsUint(exprId)))
          return PyErr_Format(PyExc_TypeError, "reference(): symbolic expression id not found");

        try {
          return PySmtAstNode(triton::ast::reference(PyLong_AsUint(exprId)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_assert(PyObject* self, PyObject* expr) {
        if (!PySmtAstNode_Check(expr))
          return PyErr_Format(PyExc_TypeError, "assert_(): expected a SmtAstNode as first argument");

        try {
          return PySmtAstNode(triton::ast::assert_(PySmtAstNode_AsSmtAstNode(expr)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_string(PyObject* self, PyObject* expr) {
        if (!PyString_Check(expr))
          return PyErr_Format(PyExc_TypeError, "string(): expected a string as first argument");

        try {
          return PySmtAstNode(triton::ast::string(PyString_AsString(expr)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_sx(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || (!PyLong_Check(op1) && !PyInt_Check(op1)))
          return PyErr_Format(PyExc_TypeError, "sx(): expected an integer as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "sx(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::sx(PyLong_AsUint(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_variable(PyObject* self, PyObject* expr) {
        if (!PyString_Check(expr))
          return PyErr_Format(PyExc_TypeError, "variable(): expected a string as first argument");

        try {
          return PySmtAstNode(triton::ast::variable(PyString_AsString(expr)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      static PyObject* ast_zx(PyObject* self, PyObject* args) {
        PyObject* op1 = nullptr;
        PyObject* op2 = nullptr;

        /* Extract arguments */
        PyArg_ParseTuple(args, "|OO", &op1, &op2);

        if (op1 == nullptr || (!PyLong_Check(op1) && !PyInt_Check(op1)))
          return PyErr_Format(PyExc_TypeError, "zx(): expected an integer as first argument");

        if (op2 == nullptr || !PySmtAstNode_Check(op2))
          return PyErr_Format(PyExc_TypeError, "zx(): expected a SmtAstNode as second argument");

        try {
          return PySmtAstNode(triton::ast::zx(PyLong_AsUint(op1), PySmtAstNode_AsSmtAstNode(op2)));
        }
        catch (const std::exception& e) {
          return PyErr_Format(PyExc_TypeError, "%s", e.what());
        }
      }


      PyMethodDef astCallbacks[] = {
        {"bv",          (PyCFunction)ast_bv,         METH_VARARGS,     ""},
        {"bvadd",       (PyCFunction)ast_bvadd,      METH_VARARGS,     ""},
        {"bvand",       (PyCFunction)ast_bvand,      METH_VARARGS,     ""},
        {"bvashr",      (PyCFunction)ast_bvashr,     METH_VARARGS,     ""},
        {"bvdecl",      (PyCFunction)ast_bvdecl,     METH_O,           ""},
        {"bvfalse",     (PyCFunction)ast_bvfalse,    METH_NOARGS,      ""},
        {"bvlshr",      (PyCFunction)ast_bvlshr,     METH_VARARGS,     ""},
        {"bvmul",       (PyCFunction)ast_bvmul,      METH_VARARGS,     ""},
        {"bvnand",      (PyCFunction)ast_bvnand,     METH_VARARGS,     ""},
        {"bvneg",       (PyCFunction)ast_bvneg,      METH_O,           ""},
        {"bvnor",       (PyCFunction)ast_bvnor,      METH_VARARGS,     ""},
        {"bvnot",       (PyCFunction)ast_bvnot,      METH_O,           ""},
        {"bvor",        (PyCFunction)ast_bvor,       METH_VARARGS,     ""},
        {"bvrol",       (PyCFunction)ast_bvrol,      METH_VARARGS,     ""},
        {"bvror",       (PyCFunction)ast_bvror,      METH_VARARGS,     ""},
        {"bvsdiv",      (PyCFunction)ast_bvsdiv,     METH_VARARGS,     ""},
        {"bvsge",       (PyCFunction)ast_bvsge,      METH_VARARGS,     ""},
        {"bvsgt",       (PyCFunction)ast_bvsgt,      METH_VARARGS,     ""},
        {"bvshl",       (PyCFunction)ast_bvshl,      METH_VARARGS,     ""},
        {"bvsle",       (PyCFunction)ast_bvsle,      METH_VARARGS,     ""},
        {"bvslt",       (PyCFunction)ast_bvslt,      METH_VARARGS,     ""},
        {"bvsmod",      (PyCFunction)ast_bvsmod,     METH_VARARGS,     ""},
        {"bvsrem",      (PyCFunction)ast_bvsrem,     METH_VARARGS,     ""},
        {"bvsub",       (PyCFunction)ast_bvsub,      METH_VARARGS,     ""},
        {"bvtrue",      (PyCFunction)ast_bvtrue,     METH_NOARGS,      ""},
        {"bvudiv",      (PyCFunction)ast_bvudiv,     METH_VARARGS,     ""},
        {"bvuge",       (PyCFunction)ast_bvuge,      METH_VARARGS,     ""},
        {"bvugt",       (PyCFunction)ast_bvugt,      METH_VARARGS,     ""},
        {"bvule",       (PyCFunction)ast_bvule,      METH_VARARGS,     ""},
        {"bvult",       (PyCFunction)ast_bvult,      METH_VARARGS,     ""},
        {"bvurem",      (PyCFunction)ast_bvurem,     METH_VARARGS,     ""},
        {"bvxnor",      (PyCFunction)ast_bvxnor,     METH_VARARGS,     ""},
        {"bvxor",       (PyCFunction)ast_bvxor,      METH_VARARGS,     ""},
        {"distinct",    (PyCFunction)ast_distinct,   METH_VARARGS,     ""},
        {"compound",    (PyCFunction)ast_compound,   METH_O,           ""},
        {"concat",      (PyCFunction)ast_concat,     METH_O,           ""},
        {"equal",       (PyCFunction)ast_equal,      METH_VARARGS,     ""},
        {"extract",     (PyCFunction)ast_extract,    METH_VARARGS,     ""},
        {"ite",         (PyCFunction)ast_ite,        METH_VARARGS,     ""},
        {"land",        (PyCFunction)ast_land,       METH_VARARGS,     ""},
        {"let",         (PyCFunction)ast_let,        METH_VARARGS,     ""},
        {"lnot",        (PyCFunction)ast_lnot,       METH_O,           ""},
        {"lor",         (PyCFunction)ast_lor,        METH_VARARGS,     ""},
        {"reference",   (PyCFunction)ast_reference,  METH_O,           ""},
        {"assert_",     (PyCFunction)ast_assert,      METH_O,           ""},
        {"string",      (PyCFunction)ast_string,     METH_O,           ""},
        {"sx",          (PyCFunction)ast_sx,         METH_VARARGS,     ""},
        {"variable",    (PyCFunction)ast_variable,   METH_O,           ""},
        {"zx",          (PyCFunction)ast_zx,         METH_VARARGS,     ""},
        {nullptr,       nullptr,                         0,                nullptr}
      };

    }; /* python namespace */
  }; /* bindings namespace */
}; /* triton namespace */

#endif /* TRITON_PYTHON_BINDINGS */

