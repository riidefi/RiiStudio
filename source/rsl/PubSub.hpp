#pragma once

#include <set>
#include <utility>

namespace rsl {

// Based on rsl::IObservable
template <typename Events> struct PubSubChannel {
  PubSubChannel() = default;
  PubSubChannel(const PubSubChannel&) = delete;
  PubSubChannel(PubSubChannel&&) = delete;

  struct IObserver : public Events {
    virtual ~IObserver() = default;
    IObserver() = default;
    IObserver(const IObserver&) = delete;
    IObserver(IObserver&&) = delete;

    // Called when the object is destroyed
    virtual void onDetach(PubSubChannel& tex) {}
  };

  class Subscriber : public IObserver {
  public:
    Subscriber() = default;
    // Can't copy: mListeningTo would hold a stale reference
    Subscriber(const Subscriber&) = delete;
    // Can't move: mListeningTo would hold a stale reference
    Subscriber(Subscriber&&) = delete;

    ~Subscriber() {
      for (auto* t : mListeningTo) {
        unsubscribe(*t);
      }
    }

    void onDetach(PubSubChannel& t) override { unsubscribe(t); }

    void subscribe(PubSubChannel& t) {
      t.observers.emplace(this);
      mListeningTo.emplace(&t);
    }
    void unsubscribe(PubSubChannel& t) {
      mListeningTo.erase(&t);
      t.observers.erase(this);
    }

  private:
    std::set<PubSubChannel*> mListeningTo;
  };

  template <typename E, typename... A> void publish(E event, A&&... args) {
    // We don't call nextGenerationId here as this is called when reverting from
    // an undo
    for (auto* it : observers) {
      (it->*event)(std::forward<A>(args)...);
    }
  }
  std::set<IObserver*> observers;

  virtual ~PubSubChannel() {
    for (auto* it : observers)
      it->onDetach(*this);
  }
};

template <typename T> using Subscriber = typename PubSubChannel<T>::Subscriber;

} // namespace rsl

// Example

#if 0
#include <rsl/PubSub.hpp>

#include <iostream>
#include <string>

struct Dog {
  std::string mName;
};

struct DogEvents {
  virtual void onBark(const Dog&) = 0;
};

class DogObserver : public rsl::Subscriber<DogEvents> {
  void onNameChanged(const Dog& dog) override {
    std::cout << "Dog barked: " << dog.mName << std::endl;
  }
};

void DogExample() {
  Dog dog{.name = "Max"};
  rsl::PubSubChannel<DogEvents> channel;

  DogObserver watcher;
  watcher.subscribe(channel);

  channel.publish(&DogEvents::onBark, dog);
  // Prints: "Dog barked: Max"
}

int main() { DogExample(); }
#endif
