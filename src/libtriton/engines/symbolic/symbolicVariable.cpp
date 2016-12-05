//! \file
/*
**  Copyright (C) - Triton
**
**  This program is under the terms of the BSD License.
*/

#include <api.hpp>
#include <exceptions.hpp>
#include <cpuSize.hpp>
#include <symbolicVariable.hpp>



namespace triton {
  namespace engines {
    namespace symbolic {

      SymbolicVariable::SymbolicVariable(symkind_e kind,
                                         triton::uint64 kindValue,
                                         triton::usize id,
                                         triton::uint32 size,
                                         const std::string& comment,
                                         triton::uint512 concreteValue) {
        this->alias            = "";
        this->comment          = comment;
        this->concreteValue    = concreteValue;
        this->id               = id;
        this->kind             = kind;
        this->kindValue        = kindValue;
        this->name             = TRITON_SYMVAR_NAME + std::to_string(id);
        this->size             = size;

        if (this->size > MAX_BITS_SUPPORTED)
          throw triton::exceptions::SymbolicVariable("SymbolicVariable::SymbolicVariable(): Size connot be greater than MAX_BITS_SUPPORTED.");
      }


      SymbolicVariable::SymbolicVariable(const SymbolicVariable &copy) {
        this->alias            = copy.alias;
        this->comment          = copy.comment;
        this->concreteValue    = copy.concreteValue;
        this->id               = copy.id;
        this->kind             = copy.kind;
        this->kindValue        = copy.kindValue;
        this->name             = copy.name;
        this->size             = copy.size;
      }


      SymbolicVariable::~SymbolicVariable() {
      }


      symkind_e SymbolicVariable::getKind(void) const {
        return this->kind;
      }


      const std::string& SymbolicVariable::getAlias(void) const {
        return this->alias;
      }


      const std::string& SymbolicVariable::getName(void) const {
        return this->name;
      }


      triton::usize SymbolicVariable::getId(void) const {
        return this->id;
      }


      triton::uint64 SymbolicVariable::getKindValue(void) const {
        return this->kindValue;
      }


      triton::uint32 SymbolicVariable::getSize(void) const {
        return this->size;
      }


      const std::string& SymbolicVariable::getComment(void) const {
        return this->comment;
      }


      triton::uint512 SymbolicVariable::getConcreteValue(void) const {
        return this->concreteValue;
      }


      void SymbolicVariable::setAlias(const std::string& alias) {
        triton::ast::AbstractNode* node = triton::api.getAstVariableNode(this->getName());

        this->alias = alias;
        if (node)
          node->init();
      }


      void SymbolicVariable::setComment(const std::string& comment) {
        this->comment = comment;
      }


      void SymbolicVariable::setConcreteValue(triton::uint512 value) {
        triton::ast::AbstractNode* node = triton::api.getAstVariableNode(this->getName());

        this->concreteValue = value;
        if (node)
          node->init();
      }


      std::ostream& operator<<(std::ostream& stream, const SymbolicVariable& symVar) {
        /* If an alias has been defined, print it */
        if (!symVar.getAlias().empty())
          stream << symVar.getAlias() << ":" << symVar.getSize();

        /* Otherwise, we print the default variable name */
        else
          stream << symVar.getName() << ":" << symVar.getSize();

        return stream;
      }


      std::ostream& operator<<(std::ostream& stream, const SymbolicVariable* symVar) {
        stream << *symVar;
        return stream;
      }

    }; /* symbolic namespace */
  }; /* engines namespace */
}; /*triton namespace */

