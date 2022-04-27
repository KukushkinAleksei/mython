#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

  using runtime::Closure;
  using runtime::Context;
  using runtime::ObjectHolder;

  namespace {
    const string ADD_METHOD = "__add__"s;
    const string INIT_METHOD = "__init__"s;
  }  // namespace

  ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_] = move(rv_->Execute(closure, context));
    return closure.at(var_);
  }

  Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    : var_(move(var))
    , rv_(move(rv)) {
  }

  VariableValue::VariableValue(const std::string& var_name) {
    dotted_ids_.push_back(var_name);
  }

  VariableValue::VariableValue(std::vector<std::string> dotted_ids)
    : dotted_ids_(move(dotted_ids)) {
  }

  ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    Closure* cur_closure = &closure;
    Closure::iterator cur_obj_ptr;

    for (const string id : dotted_ids_) {
      cur_obj_ptr = cur_closure->find(id);

      if (cur_obj_ptr == cur_closure->end()) {
        throw std::runtime_error("variable is not found");
      }
      runtime::ClassInstance* class_instance = cur_obj_ptr->second.TryAs<runtime::ClassInstance>();
      if (class_instance == nullptr) {
        return cur_obj_ptr->second;
      }
      cur_closure = &class_instance->Fields();
    }
    return cur_obj_ptr->second;
  }

  unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(make_unique<VariableValue>(name));
  }

  Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(move(argument));
  }

  Print::Print(vector<unique_ptr<Statement>> args)
    : args_(move(args)) {
  }

  ObjectHolder Print::Execute(Closure& closure, Context& context) {
    auto& output = context.GetOutputStream();
    for (const auto& argument : args_) {
      if (&argument != &args_[0]) {
        output << " ";
      }
      auto holder = argument->Execute(closure, context);
      if (holder.Get() != nullptr) {
        holder.Get()->Print(output, context);
      }
      else {
        output << "None";
      }
    }
    output << "\n";
    return {};
  }

  MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
    std::vector<std::unique_ptr<Statement>> args)
    : object_(move(object))
    , method_(move(method))
    , args_(move(args)) {
  }

  ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    auto obj = object_->Execute(closure, context);
    auto instance_ptr = obj.TryAs<runtime::ClassInstance>();
    //instance_ptr->HasMethod(method_, args_.size());
    vector<ObjectHolder> args_obj;
    for (const auto& arg_statement : args_) {
      args_obj.push_back(arg_statement->Execute(closure, context));
    }
    return instance_ptr->Call(method_, args_obj, context);
  }

  ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    auto res = operation_->Execute(closure, context);
    if (!res) {
      return ObjectHolder::Own(runtime::String{ "None"s });
    }
    runtime::DummyContext dummy_context;

    res->Print(dummy_context.GetOutputStream(), dummy_context);

    return ObjectHolder::Own(runtime::String{ dummy_context.output.str() });
  }

  ObjectHolder Add::Execute(Closure& closure, Context& context) {

    if (!rhs_ || !lhs_) {
      throw std::runtime_error("null in add operation"s);
    }

    auto lhs_obj = lhs_->Execute(closure, context);
    auto rhs_obj = rhs_->Execute(closure, context);

    auto ptr_lhs_n = lhs_obj.TryAs<runtime::Number>();
    auto ptr_rhs_n = rhs_obj.TryAs<runtime::Number>();

    if (ptr_lhs_n && ptr_rhs_n) {
      auto l_num = ptr_lhs_n->GetValue();
      auto r_num = ptr_rhs_n->GetValue();
      return ObjectHolder::Own(runtime::Number{ l_num + r_num });
    }
    auto ptr_lhs_str = lhs_obj.TryAs<runtime::String>();
    auto ptr_rhs_str = rhs_obj.TryAs<runtime::String>();

    if (ptr_lhs_str && ptr_rhs_str) {
      auto l_str = ptr_lhs_str->GetValue();
      auto r_str = ptr_rhs_str->GetValue();
      return ObjectHolder::Own(runtime::String{ l_str + r_str });
    }

    auto ptr_lhs_inst = lhs_obj.TryAs<runtime::ClassInstance>();

    if (ptr_lhs_inst) {
      const int ADD_METHOD_ARGS_COUNT = 1;
      if (ptr_lhs_inst->HasMethod(ADD_METHOD, ADD_METHOD_ARGS_COUNT)) {
        return ptr_lhs_inst->Call(ADD_METHOD, { rhs_obj }, context);
      }
    }

    throw std::runtime_error("incorrect add arguments");
  }

  ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    if (!rhs_ || !lhs_) {
      throw std::runtime_error("null in sub operation"s);
    }

    auto lhs_obj = lhs_->Execute(closure, context);
    auto rhs_obj = rhs_->Execute(closure, context);

    auto ptr_lhs_n = lhs_obj.TryAs<runtime::Number>();
    auto ptr_rhs_n = rhs_obj.TryAs<runtime::Number>();

    if (ptr_lhs_n && ptr_rhs_n) {
      auto l_num = ptr_lhs_n->GetValue();
      auto r_num = ptr_rhs_n->GetValue();
      return ObjectHolder::Own(runtime::Number{ l_num - r_num });
    }
    throw std::runtime_error("incorrect sub arguments");
  }

  ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    if (!rhs_ || !lhs_) {
      throw std::runtime_error("null in mult operation"s);
    }

    auto lhs_obj = lhs_->Execute(closure, context);
    auto rhs_obj = rhs_->Execute(closure, context);

    auto ptr_lhs_n = lhs_obj.TryAs<runtime::Number>();
    auto ptr_rhs_n = rhs_obj.TryAs<runtime::Number>();

    if (ptr_lhs_n && ptr_rhs_n) {
      auto l_num = ptr_lhs_n->GetValue();
      auto r_num = ptr_rhs_n->GetValue();
      return ObjectHolder::Own(runtime::Number{ l_num * r_num });
    }
    throw std::runtime_error("incorrect mult arguments");
  }

  ObjectHolder Div::Execute(Closure& closure, Context& context) {
    if (!rhs_ || !lhs_) {
      throw std::runtime_error("null in Div operation"s);
    }

    auto lhs_obj = lhs_->Execute(closure, context);
    auto rhs_obj = rhs_->Execute(closure, context);

    auto ptr_lhs_n = lhs_obj.TryAs<runtime::Number>();
    auto ptr_rhs_n = rhs_obj.TryAs<runtime::Number>();

    if (ptr_lhs_n && ptr_rhs_n) {
      auto l_num = ptr_lhs_n->GetValue();
      auto r_num = ptr_rhs_n->GetValue();
      if (r_num == 0)
        throw runtime_error("zero divizion");

      return ObjectHolder::Own(runtime::Number{ l_num / r_num });
    }
    throw std::runtime_error("incorrect Div arguments");
  }


  // Добавляет очередную инструкцию в конец составной инструкции

  void Compound::AddStatement(std::unique_ptr<Statement> stmt) {
    statements_.push_back(move(stmt));
  }

  ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (const auto& statement : statements_) {
      statement->Execute(closure, context);
    }
    return ObjectHolder::None();
  }

  Return::Return(std::unique_ptr<Statement> statement) : statement_(move(statement)) {
  }

  ObjectHolder Return::Execute(Closure& closure, Context& context) {
    return statement_->Execute(closure, context);
  }

  ClassDefinition::ClassDefinition(ObjectHolder cls) :cls_(cls) {
  }

  ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    auto obj = cls_.TryAs<runtime::Class>();
    closure[obj->GetName()] = move(cls_);
    return {};
  }

  FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
    std::unique_ptr<Statement> rv)
    : object_(move(object))
    , field_name_(move(field_name))
    , rv_(move(rv)) {
  }

  ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    auto object_holder = object_.Execute(closure, context);

    auto class_instance_ptr = object_holder.TryAs<runtime::ClassInstance>();
    Closure& fields = class_instance_ptr->Fields();
    fields[field_name_] = move(rv_->Execute(closure, context));

    return fields.at(field_name_);
  }

  IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
    std::unique_ptr<Statement> else_body)
    : condition_(move(condition))
    , if_body_(move(if_body))
    , else_body_(move(else_body)) {
  }

  ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    auto cond_result = condition_->Execute(closure, context);
    auto cond_res_ptr = cond_result.TryAs<runtime::Bool>();
    if (cond_res_ptr) {
      if (cond_res_ptr->GetValue()) {
        return if_body_->Execute(closure, context);
      }
      else {
        return else_body_->Execute(closure, context);
      }
    }
    return {};
  }

  ObjectHolder Or::Execute(Closure& closure, Context& context) {
    if (!rhs_ || !lhs_) {
      throw std::runtime_error("null in OR operation"s);
    }

    auto lhs_obj = lhs_->Execute(closure, context);
    auto rhs_obj = rhs_->Execute(closure, context);

    auto ptr_lhs_b = lhs_obj.TryAs<runtime::Bool>();
    auto ptr_rhs_b = rhs_obj.TryAs<runtime::Bool>();

    if (ptr_lhs_b && ptr_rhs_b) {
      auto l_bool = ptr_lhs_b->GetValue();
      auto r_bool = ptr_rhs_b->GetValue();
      return ObjectHolder::Own(runtime::Bool{ l_bool || r_bool });
    }
    throw std::runtime_error("invalid OR operands"s);
  }

  ObjectHolder And::Execute(Closure& closure, Context& context) {
    if (!rhs_ || !lhs_) {
      throw std::runtime_error("null in AND operation"s);
    }

    auto lhs_obj = lhs_->Execute(closure, context);
    auto rhs_obj = rhs_->Execute(closure, context);

    auto ptr_lhs_b = lhs_obj.TryAs<runtime::Bool>();
    auto ptr_rhs_b = rhs_obj.TryAs<runtime::Bool>();

    if (ptr_lhs_b && ptr_rhs_b) {
      auto l_bool = ptr_lhs_b->GetValue();
      auto r_bool = ptr_rhs_b->GetValue();
      return ObjectHolder::Own(runtime::Bool{ l_bool && r_bool });
    }
    throw std::runtime_error("invalid AND operands"s);
  }

  ObjectHolder Not::Execute(Closure& closure, Context& context) {

    auto obj = operation_->Execute(closure, context);

    auto ptr_b = obj.TryAs<runtime::Bool>();

    if (ptr_b) {
      auto l_bool = ptr_b->GetValue();
      return ObjectHolder::Own(runtime::Bool{ !l_bool });
    }
    throw std::runtime_error("invalid AND operands"s);
  }

  Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp) {
  }

  ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    bool result = cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context);
    return ObjectHolder::Own(runtime::Bool(result));
  }

  NewInstance::NewInstance(const runtime::Class& class_,
    std::vector<std::unique_ptr<Statement>> args)
    : class_ref(class_)
    , args_(move(args)) {
  }

  NewInstance::NewInstance(const runtime::Class& class_)
    : class_ref(class_) {
  }

  ObjectHolder NewInstance::Execute(Closure& /*closure*/, Context& /*context*/) {
    return ObjectHolder::Own(runtime::ClassInstance(class_ref));
  }

  MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_(move(body)) {
  }

  ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    return body_->Execute(closure, context);
  }

}  // namespace ast