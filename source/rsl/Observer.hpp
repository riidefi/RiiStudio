#pragma once

#include <set>
#include <utility>

namespace rsl {

// Based on rsl::PubSubChannel
template <typename T> struct IObservable {
  // static_assert(&T::Events, "T must contain a structure Events with
  // callbacks");

  IObservable() = default;
  IObservable(const IObservable&) = delete;
  IObservable(IObservable&&) = delete;

  struct IObserver : public T::Events {
    virtual ~IObserver() = default;
    IObserver() = default;
    IObserver(const IObserver&) = delete;
    IObserver(IObserver&&) = delete;

    // Called when the object is destroyed
    virtual void onDetach(const T& tex) {}
  };

  class Observer : public IObserver {
  public:
    Observer() = default;
    ~Observer() {
      for (const auto* t : mListeningTo) {
        unlisten(*t);
      }
    }

    void onDetach(const T& t) override { unlisten(t); }

    void listen(const T& t) {
      t.observers.emplace(this);
      mListeningTo.emplace(&t);
    }
    void unlisten(const T& t) {
      mListeningTo.erase(&t);
      t.observers.erase(this);
    }

  private:
    std::set<const T*> mListeningTo;
  };

  template <typename E, typename... A>
  void notifyObservers(E event, A&&... args) {
    for (auto* it : observers) {
      (it->*event)(std::forward<A>(args)...);
    }
  }
  mutable std::set<IObserver*> observers;

  virtual ~IObservable() {
    for (auto* it : observers)
      it->onDetach(*static_cast<T*>(this));
  }
};

} // namespace rsl

// EXAMPLE

#if 0
#include <iostream>
#include <string>

#include <rsl/Observer.hpp>

class Dog : public rsl::IObservable<Dog> {
public:
  Dog(const std::string& name) : mName(name) {}
  ~Dog() = default;

  struct Events {
    virtual void onNameChanged(const Dog&) = 0;
  };

  void setName(const std::string& name) {
    mName = name;
    notifyObservers(&Events::onNameChanged, *this);
  }
  std::string getName() const { return mName; }

private:
  std::string mName;
};

class DogObserver : public Dog::Observer {
  void onNameChanged(const Dog& dog) override {
    std::cout << "Dog name changed to: " << dog.getName() << std::endl;
  }
};

void DogExample() {
  Dog old_dog("Grandpa");

  {
    DogObserver watcher;
    watcher.listen(old_dog);

    {
      Dog dog("Max");
      watcher.listen(dog);

      dog.setName("Max 2");
      dog.setName("Max 3");

      // Dog is destroyed as it exits scope
    }

    // watcher is destroyed
  }

  // No message issued
  old_dog.setName("Grandpa 2");
}

int main() { DogExample(); }
#endif
