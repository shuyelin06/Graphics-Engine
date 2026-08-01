#pragma once

#include <memory>
#include <stdexcept>

namespace Engine
{
// UniqueFunction Implementation:
// Lets us create lambdas that are move-only (does not support
// copying), which will let us pass unique ptrs into these functions.
//
// How do we do this? Well, lambdas are essentially unique unnamed types.
// We need to do type erasure so that we can wrap them under "one type"
// To store them together.
// 1) Define an interface that all of the lambdas will use: CallableFunction
// 2) We implement that interface for all possible lambda types. So anytime we
//    execute call(), we call the lambda.
// 3) Now, using pointers, we can store a pointer of the base interface under
//    our UniqueFunction class so it is not a template, but can accept arbitrary
//    lambdas.

// Template to allow type enforcing with UniqueFunctions
template <typename Signature> class UniqueFunction;

// Specialize the template. By breaking the function down into
// UniqueFunction<Ret(Args...)>, we are setting this pattern where the compiler
// will use this specialization if it matches (basically any function
// signature). Furthermore, this pattern lets us separate the return value and
// argument list.
template <typename Ret, typename... Args> class UniqueFunction<Ret(Args...)>
{
  private:
    struct CallableFunction
    {
        virtual ~CallableFunction() = default;
        virtual Ret call(Args... args) = 0;
    };

    template <typename T> struct CallableWrapper : public CallableFunction
    {
        T callable;

        CallableWrapper(T&& callable)
            : callable(std::move(callable))
        {
        }
        Ret call(Args... args) override
        {
            return callable(std::forward<Args>(args)...);
        }
    };

    std::unique_ptr<CallableFunction> callable;

  public:
    UniqueFunction() = default;
    template <typename T> UniqueFunction(T&& function)
    {
        callable = std::unique_ptr<CallableFunction>(
            new CallableWrapper<T>(std::move(function)));
    }

    // Enable Moving via the Move Constructor
    UniqueFunction(UniqueFunction&&) = default;
    UniqueFunction& operator=(UniqueFunction&&) = default;

    // Disable Copying by Deleting the Copy Constructor
    UniqueFunction(const UniqueFunction&) = delete;
    UniqueFunction& operator=(const UniqueFunction&) = delete;

    // Override the () operator to invoke the function
    Ret operator()(Args... args)
    {
        if (callable == nullptr)
            throw std::runtime_error("Function Invalid");
        return callable->call(std::forward<Args>(args)...);
    }
};

} // namespace Engine