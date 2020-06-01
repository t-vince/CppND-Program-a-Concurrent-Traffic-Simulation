#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a    
    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a
    std::lock_guard<std::mutex> uLock(_mutex);
    _queue.push_back(std::move(msg));
    _cond.notify_one(); // Notify client
}


/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b
    while (true) {
        
        auto currentPhase = _queue.receive();
        if (currentPhase == green) {
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

int getRandomWaitTime(int min, int max) {
    return ( rand()%(max-min + 1) + min );
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a
    int waitDuration = getRandomWaitTime(4, 6);
    auto lastSwitchedTime = std::chrono::system_clock::now();

    while (true) {
        auto currentLoopTime = std::chrono::system_clock::now();
        int SecondsSinceSwitch = std::chrono::duration_cast<std::chrono::seconds>
                                (currentLoopTime - lastSwitchedTime)
                                .count();

        if (SecondsSinceSwitch >= waitDuration) {
            _currentPhase = (_currentPhase == red)? green : red;

            auto ftr = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_queue, std::move(_currentPhase));
            ftr.wait();

            // Reset loop start time & duration
            lastSwitchedTime = currentLoopTime;
            waitDuration = getRandomWaitTime(4, 6);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
