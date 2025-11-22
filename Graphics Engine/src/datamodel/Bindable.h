#pragma once

#include <functional>
#include <vector>

#include "Object.h"

namespace Engine {
namespace Datamodel {

// Bindable Class Template:
// Adds the static methods necessary to make a datamodel
// class connect with non-datamodel systems.
// To use:
// 1) Make datamodel class inherit CreationCallback<Class> publicly
// 2) In the constructor, call Bindable<Class>(this).
// 3) In the system you want to connect with the creation, call (once)
//      Class::ConnectToCreation(lambda)
// Also gives a class identifier that can be used
// to identify the object
template <typename Derived> class Bindable {
  private:
    static std::function<void(Object*)> ConstructorCallback;
    static uint16_t ClassId;

  public:
    Bindable(Object* object) { object->setClassID(ClassID()); }

    static void ConnectToCreation(std::function<void(Object*)> func) {
        ConstructorCallback = func;
    }
    static void SignalObjectCreation(Object* obj) {
        if (ConstructorCallback != nullptr)
            ConstructorCallback(obj);
    }
    static uint16_t ClassID() { return ClassId; }
};

template <typename Derived>
std::function<void(Object*)> Bindable<Derived>::ConstructorCallback = nullptr;

struct ClassIDCounter {
    static uint16_t next_id;
};

template <typename Derived>
uint16_t Bindable<Derived>::ClassId = ClassIDCounter::next_id++;

} // namespace Datamodel
} // namespace Engine