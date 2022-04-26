#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace {
  const string SELF_OBJECT = "self"s;
  const string STR_METHOD = "__str__"s;
  const string EQ_METHOD = "__eq__"s;
  const string LT_METHOD = "__lt__"s;
} // namespace


namespace runtime {

  ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
  }

  void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
  }

  ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
  }

  ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
  }

  Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
  }

  Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
  }

  Object* ObjectHolder::Get() const {
    return data_.get();
  }

  ObjectHolder::operator bool() const {
    return Get() != nullptr;
  }

  bool IsTrue(const ObjectHolder& object) {
    Bool* bool_ptr = object.TryAs<Bool>();
    if (bool_ptr) {
      return bool_ptr->GetValue();
    }

    Number* number_ptr = object.TryAs<Number>();
    if (number_ptr) {
      return number_ptr->GetValue() != 0;
    }

    String* str_ptr = object.TryAs<String>();
    if (str_ptr) {
      return str_ptr->GetValue() != ""s;
    }

    return false;
  }

  void ClassInstance::Print(std::ostream& os, Context& context) {

    auto method_ptr = this->cls_.GetMethod(STR_METHOD);

    if (method_ptr) {
      auto res = Call(STR_METHOD, {}, context);
      res->Print(os, context);
    }
    else {
      os << this;
    }
  }

  bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    const Method* method_ptr = cls_.GetMethod(method);
    if (!method_ptr)
      return false;

    if (method_ptr->formal_params.size() == argument_count) {
      return true;
    }
    return false;
  }

  Closure& ClassInstance::Fields() {
    return fields_;
  }

  const Closure& ClassInstance::Fields() const {
    return fields_;
  }

  ClassInstance::ClassInstance(const Class& cls)
    :cls_(cls) {
  }

  ObjectHolder ClassInstance::Call(const std::string& method,
    const std::vector<ObjectHolder>& actual_args,
    Context& context) {

    if (!this->HasMethod(method, actual_args.size())) {
      throw std::runtime_error("No method found");
    }

    const Method* method_ptr = cls_.GetMethod(method);
    Closure closure;
    closure["self"] = ObjectHolder::Share(*this);

    for (size_t i = 0; i < method_ptr->formal_params.size(); ++i) {
      auto arg = method_ptr->formal_params[i];
      closure[arg] = actual_args[i];
    }

    return  method_ptr->body->Execute(closure, context);
  }

  Class::Class(std::string name,
    std::vector<Method> methods, const Class* parent)
    : parent_(parent)
    , name_(move(name))
    , methods_(move(methods)) {

    if (parent_) {
      name_to_method_.insert(parent_->name_to_method_.begin(), parent_->name_to_method_.end());
    }

    for (const auto& method : methods_) {
      name_to_method_[method.name] = &method;
    }

  }

  const Method* Class::GetMethod(const std::string& name) const {
    auto name_method_pair = name_to_method_.find(name);
    if (name_method_pair != name_to_method_.end())
      return name_method_pair->second;
    else
      return nullptr;
  }

  [[nodiscard]] const std::string& Class::GetName() const {
    return name_;
  }

  void Class::Print(ostream& os, Context& /*context*/) {
    os << "Class "sv;
    os << GetName();
  }

  const Class* Class::GetParent() const {
    return parent_;
  }

  void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
  }

  template<typename T, typename Comparator>
  bool CmpOpImpl(const ObjectHolder& lhs, const ObjectHolder& rhs, const Comparator& cmp, bool& result) {
    T* lhs_ptr = lhs.TryAs<T>();
    T* lrhs_ptr = rhs.TryAs<T>();
    if (lhs_ptr && lrhs_ptr) {
      result = cmp(lhs_ptr->GetValue(), lrhs_ptr->GetValue());
      return true;
    }
    return false;
  }

  bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (!lhs && !rhs) {
      return true;
    }

    bool result = false;
    if (CmpOpImpl<Number>(lhs, rhs, equal_to<int>(), result))
      return result;

    if (CmpOpImpl<Bool>(lhs, rhs, equal_to<bool>(), result))
      return result;

    if (CmpOpImpl<String>(lhs, rhs, equal_to<string>(), result))
      return result;

    auto l_ptr_class_inst = lhs.TryAs<ClassInstance>();
    const int EQ_METHOD_ARGS_COUNT = 1;
    if (l_ptr_class_inst && l_ptr_class_inst->HasMethod(EQ_METHOD, EQ_METHOD_ARGS_COUNT)) {
      auto res = l_ptr_class_inst->Call(EQ_METHOD, { rhs }, context);
      return res.TryAs<Bool>()->GetValue();
    }

    throw std::runtime_error("Cannot compare objects for equality"s);
  }

  bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    bool result = false;
    if (CmpOpImpl<Number>(lhs, rhs, less<int>(), result))
      return result;

    if (CmpOpImpl<Bool>(lhs, rhs, less<bool>(), result))
      return result;

    if (CmpOpImpl<String>(lhs, rhs, less<string>(), result))
      return result;

    auto l_ptr_class_inst = lhs.TryAs<ClassInstance>();
    const int LT_METHOD_ARGS_COUNT = 1;

    if (l_ptr_class_inst && l_ptr_class_inst->HasMethod(LT_METHOD, LT_METHOD_ARGS_COUNT)) {
      auto res = l_ptr_class_inst->Call(LT_METHOD, { rhs }, context);
      return res.TryAs<Bool>()->GetValue();
    }

    throw std::runtime_error("Cannot compare objects with less"s);
  }

  bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
  }

  bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
  }

  bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
  }

  bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
  }
}  // namespace runtime