#pragma once

#include <vector>

namespace Engine {
namespace Datamodel {
class Object;

// DMBinding Template Class:
// This class interfaces with datamodel objects.
// Systems that create entities that pull data from the datamodel
// should have the classes inherit this, where the typename is the
// datamodel object being pulled from.
// The system to which this binding belongs to is in charge of
// cleaning up this. It should be destroyed if dm_object == null
class DMBinding {
  private:
    Object* dm_object;

  public:
    DMBinding(Object* obj);
    ~DMBinding();

    // Used for management / clean-up of bindings
    void unbind();
    bool shouldDestroy() const;

    // Main interface for pulling datamodel data.
    // Override pullDatamodelDataImpl to pull data.
    void pullDatamodelData();

  protected:
    virtual void pullDatamodelDataImpl(Object* object);
};

// CleanAndPullDatamodelData
// Helper template method that will pull datamodel data, and clean the vector
// of invalid bindings.
template <typename T>
void cleanAndPullDatamodelData(std::vector<T*>& bindings) {
    static_assert(std::is_base_of<DMBinding, T>::value,
                  "T must inherit from DMBinding");

    typename std::vector<T*>::iterator iter = bindings.begin();
    while (iter != bindings.end()) {
        if ((*iter)->shouldDestroy()) {
            delete *iter;
            iter = bindings.erase(iter);
        } else {
            (*iter)->pullDatamodelData();
            iter++;
        }
    }
}

}; // namespace Datamodel
}; // namespace Engine