#pragma once

#include <functional>
#include <vector>

namespace Engine {
namespace Datamodel {
class Object;

// CreationCallback Class Template:
// Adds the static methods necessary to make a datamodel
// class connect with non-datamodel systems.
// To use:
// 1) Make datamodel class inherit CreationCallback<Class> publicly
// 2) In the constructor, call Bindable<Class>(this).
// 3) In the system you want to connect with the creation, call (once)
//      Class::ConnectToCreation(lambda)
template <typename Derived> class CreationCallback {
  private:
    static std::function<void(Object*)> ConstructorCallback;

  public:
    CreationCallback(Derived* derived) {
        if (ConstructorCallback != nullptr)
            ConstructorCallback(derived);
    }

    static void ConnectToCreation(std::function<void(Object*)> func) {
        ConstructorCallback = func;
    }
};

template <typename Derived>
std::function<void(Object*)> CreationCallback<Derived>::ConstructorCallback = nullptr;

} // namespace Datamodel
} // namespace Engine