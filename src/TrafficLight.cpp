#include <iostream>
#include <random>
#include <stdlib.h> // includes rand
#include <algorithm>  // std::for_each
#include <future>
#include <mutex>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */
 
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
  
  	// perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    /*
    If the vector is empty, wait is called. 
    When the thread wakes up again, the condition is immediately re-checked 
    and - in case it has not been a spurious wake-up we can continue with 
    our job and retrieve the vector.
    */
    _condition.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable
  	// remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();
	return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
  	
  	// perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);
  	// add vector to queue
    _queue.push_back(std::move(msg));
    _condition.notify_one(); // notify client after pushing new msg into vector
}


/* Implementation of class "TrafficLight" */

 
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
  	while(true)
    {
      if(_trafficLightPhaseQueue.receive() == TrafficLightPhase::green)
      {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5)); // save some processor workload by letting this sleep for 5 ms
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
  	threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
  	int numberOfCycles = 0;
  	/*Generate a random value between 4s and 6s by adding (2s * x / 100) to 4s, where x is a random number between 1 and 100.  */
  	int maxNumberOfCycles = 4000 + (std::rand() % 100 + 1) * 20; 
  	while(true)
    {
      	std::this_thread::sleep_for(std::chrono::milliseconds(1));
      	numberOfCycles++;
      	if(numberOfCycles > maxNumberOfCycles)
        {
          	if(_currentPhase==TrafficLightPhase::green)
          	{
            	_currentPhase=TrafficLightPhase::red;
            }else
            {
              	_currentPhase=TrafficLightPhase::green;
            }
          	numberOfCycles = 0; // Re-init number of cycle
          	maxNumberOfCycles = maxNumberOfCycles = 4000 + (std::rand() % 100 + 1) * 20; // Find a new cylce-duration
          	_trafficLightPhaseQueue.send(std::move(_currentPhase)); // push the new traffic light phase into the message queue using moving semantics
        }
    }
}