
# System Design – Multithreaded Banker's Algorithm

## 1. Introduction

This document describes the architecture and design of the Multithreaded Banker's Algorithm project.

The project implements deadlock avoidance using the C programming language and POSIX Threads (Pthreads).

Multiple customer threads request and release shared resources. A centralized Banker module manages resource allocation and ensures that every granted request leaves the system in a safe state.

Mutex locks and condition variables coordinate concurrent access to shared resource data.

## 2. System Architecture

The project follows a modular architecture consisting of:

1. Main Module
2. Banker Module
3. Customer Module
4. Synchronization Mechanism
5. Testing Module

### 2.1 Architecture Overview

The main module initializes the Banker structure and creates customer threads.

Each customer thread submits resource requests to the Banker module. The Banker validates the request, checks resource availability, and executes the safety algorithm before granting the request.

After completing its simulated work, the customer releases its resources. The release operation updates the shared resource data and notifies waiting threads.

### 2.2 High-Level Workflow

```text
                 START
                   |
                   v
          Initialize Banker
                   |
                   v
          Initialize Resources
                   |
                   v
          Calculate Need Matrix
                   |
                   v
          Check Initial Safety
                   |
                   v
          Create Customer Threads
                   |
                   v
        Customer Requests Resources
                   |
                   v
          Validate the Request
                   |
                   v
       Are Requested Resources Valid?
              /           \
            No             Yes
            |               |
            v               v
       Reject Request   Check Availability
                            |
                            v
                  Resources Available?
                     /          \
                   No            Yes
                   |              |
                   v              v
              Wait on         Tentative
           Condition Variable Allocation
                                  |
                                  v
                           Check Safe State
                                  |
                                  v
                            Safe State?
                            /       \
                          No         Yes
                          |           |
                          v           v
                       Rollback    Grant Request
                          |           |
                          v           v
                       Reject     Customer Executes
                                      |
                                      v
                                Release Resources
                                      |
                                      v
                              Notify Waiting Threads
                                      |
                                      v
                              Join Customer Threads
                                      |
                                      v
                              Verify Final State
                                      |
                                      v
                                     END
```

## 3. Module Design

The implementation is divided into separate modules to improve readability, maintainability, and testing.

### 3.1 Main Module – src/main.c

The main module controls the overall execution of the application.

Responsibilities:

- Initialize the Banker structure.
- Define the initial Available, Maximum, and Allocation matrices.
- Calculate the Need matrix.
- Check the initial system safety.
- Create multiple customer threads.
- Wait for successfully created threads to finish.
- Verify that all resources have been restored.
- Perform a final safety check.
- Destroy synchronization objects before termination.

The main module does not directly perform resource allocation. Resource management is handled by the Banker module.

### 3.2 Banker Module – src/banker.c and src/banker.h

The Banker module implements the core resource-management functionality.

The Banker structure maintains the shared system state.

| Field | Description |
|---|---|
| available | Number of currently free instances of each resource |
| maximum | Maximum resource requirement of each customer |
| allocation | Resources currently allocated to each customer |
| need | Remaining resource requirement of each customer |
| mutex | Protects shared resource data |
| resourcesChanged | Notifies waiting threads when resources are released |

The module provides the following major functions:

**bankerInit()**

Initializes the mutex and condition variable required for thread synchronization.

**bankerDestroy()**

Destroys the synchronization objects after all customer threads have completed.

**calculateNeed()**

Calculates the remaining resource requirement:

Need = Maximum - Allocation

**isSafeState()**

Checks whether the current resource allocation has at least one safe execution sequence.

**requestResources()**

Processes a customer's resource request. It validates the request, waits if resources are temporarily unavailable, tentatively allocates resources, and checks system safety.

**releaseResources()**

Releases allocated resources, updates the shared matrices, and notifies waiting threads.

### 3.3 Customer Module – src/customer.c and src/customer.h

The customer module represents individual customers as independent POSIX threads.

Each customer thread:

1. Receives its customer ID and a pointer to the shared Banker structure.
2. Creates its predefined resource request.
3. Calls requestResources().
4. Executes simulated work if the request is granted.
5. Releases the requested resources.
6. Terminates after completing its operation.

The project creates five customer threads, representing customers C0 to C4.

Each thread operates independently, while all resource operations are coordinated through the Banker module.

### 3.4 Testing Module – tests/

The testing module contains independent test programs for verifying the correctness of the implementation.

| Test file | Purpose |
|---|---|
| test_safe.c | Verifies safe-state detection and safe sequence generation |
| test_request.c | Verifies successful resource allocation and release |
| test_unsafe.c | Verifies rejection and rollback of unsafe requests |
| test_wait.c | Verifies that a customer waits when resources are unavailable |
| test_invalid.c | Verifies rejection of invalid requests and releases |
| test_stress.c | Tests repeated concurrent resource transactions |

Each test program is compiled and executed independently.

## 4. Synchronization Design

Since multiple customer threads access shared resource data, synchronization is necessary to prevent race conditions and inconsistent resource allocation.

This project uses two POSIX synchronization mechanisms:

- Mutex locks
- Condition variables

### 4.1 Mutex Lock

A mutex ensures that only one thread at a time can execute a protected resource-management operation.

The mutex protects:

- Available resources
- Allocation matrix
- Need matrix
- Safety checking during resource allocation
- Resource release operations

The request and release operations acquire the mutex before accessing shared resource data and release it after completing the operation.

This prevents multiple threads from modifying the resource matrices simultaneously.

### 4.2 Condition Variable

A condition variable allows a thread to wait until a particular condition becomes true.

In this project, a customer may request more resources than are currently available.

Instead of immediately terminating, the customer waits using:

```c
pthread_cond_wait(&banker->resourcesChanged, &banker->mutex);
```

This operation atomically releases the mutex while the thread waits. When notified, the thread reacquires the mutex before continuing.

When another customer releases resources, the Banker module calls:

```c
pthread_cond_broadcast(&banker->resourcesChanged);
```

This wakes waiting threads so they can check resource availability again.

The availability check is performed inside a while loop because a notification does not guarantee that the required resources are available when a thread resumes.

### 4.3 Critical Sections

The following operations are protected by the mutex:

1. Validating a request against the customer's remaining Need.
2. Checking Available resources.
3. Tentatively updating Available, Allocation, and Need.
4. Executing the safety check during a request.
5. Rolling back an unsafe allocation.
6. Validating and processing resource releases.
7. Broadcasting the resource-availability notification.

The mutex ensures that these operations are performed consistently.

### 4.4 Avoiding Recursive Mutex Locking

The implementation uses an internal safety-checking function that assumes the mutex is already held.

The public isSafeState() function acquires the mutex before performing the safety check.

However, requestResources() already holds the mutex while checking a tentative allocation. Therefore, it calls the internal safety-checking function directly instead of calling the public function.

This avoids attempting to acquire the same non-recursive mutex twice from the same thread.

## 5. Resource Allocation Design

The Banker module follows a controlled resource-allocation process.

### Step 1: Request Validation

The requested resources must satisfy:

Request[i] <= Need[customer][i]

Requests containing negative values or exceeding the customer's remaining Need are rejected.

### Step 2: Availability Check

The request is compared with the currently available resources:

Request[i] <= Available[i]

If sufficient resources are not available, the customer waits on the condition variable.

### Step 3: Tentative Allocation

If resources are available, the Banker temporarily updates the resource matrices:

Available = Available - Request

Allocation = Allocation + Request

Need = Need - Request

### Step 4: Safety Check

The Banker executes the safety algorithm to determine whether the tentative allocation leaves the system in a safe state.

### Step 5: Decision

- If the state is safe, the request is granted.
- If the state is unsafe, the tentative allocation is rolled back and the request is rejected.

This ensures that an unsafe resource allocation is never committed.

## 6. Resource Release Design

After completing its simulated execution, a customer releases its allocated resources.

The release operation performs the following steps:

1. Validate the customer ID and release values.
2. Ensure that the customer does not release more resources than it owns.
3. Increase the Available vector.
4. Decrease the customer's Allocation.
5. Increase the customer's Need.
6. Notify waiting threads using pthread_cond_broadcast().

The resource release operation is also protected by the mutex.

## 7. Thread Lifecycle

Each customer thread follows this lifecycle:

```text
        Thread Created
               |
               v
       Request Resources
               |
               v
      Wait if Not Available
               |
               v
       Request Processed
               |
          +----+----+
          |         |
        Safe      Unsafe
          |         |
          v         v
       Granted   Rejected
          |
          v
     Simulated Work
          |
          v
    Release Resources
          |
          v
      Thread Ends
```

The main thread uses pthread_join() to wait for customer threads to complete before checking the final resource state and destroying synchronization objects.

## 8. Resource Consistency

The implementation maintains consistency between the Available, Allocation, and Need data structures.

For every resource type:

Total Resources = Available + Sum of Allocations

The Need matrix must also satisfy:

Need = Maximum - Allocation

The project verifies resource consistency by comparing the final Available and Allocation values with their initial snapshots after all customer threads complete.

The final safety check verifies that the system remains in a safe state.

## 9. Error Handling

The implementation includes checks for invalid operations.

Examples include:

- Invalid customer IDs.
- Negative resource requests.
- Requests exceeding the customer's remaining Need.
- Invalid release values.
- Attempts to release more resources than the customer owns.
- Initialization failures for synchronization objects.

Invalid operations are rejected without modifying the shared resource state.

The test suite verifies that invalid operations do not corrupt the resource matrices.

## 10. Design Limitations

The current implementation has the following limitations:

1. The number of customers and resource types is fixed at compile time.
2. Customer resource requests are predefined.
3. The implementation uses a single Banker structure for shared resource management.
4. A customer may wait indefinitely if the resources it requires are never released.
5. The condition-variable implementation does not guarantee fairness or a particular order among waiting threads.
6. Request timeouts and thread cancellation are not implemented.

These limitations can be addressed in future versions.

## 11. Future Enhancements

Possible improvements include:

- Accepting resource requests dynamically from users.
- Supporting configurable numbers of customers and resource types.
- Adding request timeouts and cancellation.
- Implementing a fair waiting queue.
- Adding detailed execution logs and performance measurements.
- Developing a graphical interface to visualize resource allocation and safe sequences.

## 12. Conclusion

The system design separates resource management, customer execution, synchronization, and testing into independent modules.

The Banker module ensures that resource requests are validated and checked for safety before allocation. Mutex locks protect shared data, while condition variables allow customers to wait for resources and resume when resources are released.

This modular design demonstrates how deadlock avoidance can be implemented in a multithreaded operating-system environment using C and POSIX Threads.
