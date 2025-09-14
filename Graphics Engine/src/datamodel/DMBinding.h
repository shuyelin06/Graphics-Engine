#pragma once

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

}; // namespace Datamodel
}; // namespace Engine