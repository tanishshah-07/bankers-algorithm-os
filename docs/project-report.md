
# MULTITHREADED IMPLEMENTATION OF BANKER'S ALGORITHM

## Project Report

**Course:** Operating Systems
**Department:** Computer Science and Engineering
**Institution:** SRM University-AP

**Project Team Members:**
1. Name: ____________________  Roll Number: ____________________
2. Name: ____________________  Roll Number: ____________________
3. Name: ____________________  Roll Number: ____________________
4. Name: ____________________  Roll Number: ____________________

**Academic Year:** 2026–2027

---

# Abstract

Deadlock is a significant problem in operating systems where multiple processes compete for limited resources. Banker's Algorithm is a deadlock-avoidance technique that evaluates resource requests before granting them to ensure that the system remains in a safe state.

This project presents a multithreaded implementation of Banker's Algorithm using the C programming language and POSIX Threads (Pthreads). Multiple customer threads concurrently request and release shared resources. A centralized Banker module validates each request, performs safety checking, and grants resources only when the resulting system state is safe.

Mutex locks are used to protect shared resource data, while condition variables allow threads to wait when the requested resources are temporarily unavailable. The implementation also supports rollback of unsafe allocations and validation of invalid requests.

The project is evaluated through independent functional tests, resource-waiting tests, invalid-operation tests, and concurrent stress testing. The stress test successfully completed 500 resource transactions with no failed transactions, while maintaining resource consistency and a safe final state.

The project demonstrates how deadlock avoidance can be combined with multithreading and synchronization mechanisms in an operating-system environment.

**Keywords:** Operating Systems, Banker's Algorithm, Deadlock Avoidance, Multithreading, POSIX Threads, Mutex, Condition Variable.

---

# 1. Introduction

An operating system manages several processes that compete for limited system resources such as memory, printers, files, and I/O devices.

When processes hold resources while waiting for other resources, a deadlock may occur. A deadlock is a situation in which a group of processes remains blocked because each process is waiting for a resource held by another process in the group.

Deadlock management is an important responsibility of an operating system.

Banker's Algorithm is a deadlock-avoidance algorithm introduced by Edsger W. Dijkstra. It examines resource requests and determines whether granting them will leave the system in a safe state.

A system is safe if there exists at least one sequence in which all processes can complete their execution using the available resources and resources released by previously completed processes.

This project implements Banker's Algorithm in a multithreaded environment. Instead of treating customers only as logical processes, the implementation creates independent POSIX threads that concurrently request and release resources.

The project combines deadlock avoidance with thread synchronization to maintain a consistent shared resource state.

## 1.1 Problem Statement

In a multithreaded operating-system environment, multiple threads may request shared resources simultaneously.

Without proper synchronization, concurrent access to resource data can result in race conditions, incorrect allocation values, and inconsistent system states.

The objective is to develop a resource-management system that:

- Supports multiple concurrent customer threads.
- Validates resource requests.
- Grants requests only when the resulting system state is safe.
- Prevents simultaneous modifications to shared resource data.
- Allows threads to wait when resources are temporarily unavailable.
- Restores resources after customer execution.

## 1.2 Objectives

The main objectives are:

1. Implement the Banker's Algorithm using C.
2. Maintain Available, Maximum, Allocation, and Need data structures.
3. Implement the safety algorithm to detect safe states.
4. Create multiple customer threads using POSIX Threads.
5. Synchronize resource allocation and release using mutex locks.
6. Use condition variables to manage waiting threads.
7. Reject unsafe and invalid resource requests.
8. Verify correctness through functional and concurrent testing.

## 1.3 Scope

The current implementation supports:

- Five customer threads.
- Three resource types.
- Predefined maximum resource requirements.
- Concurrent resource requests and releases.
- Safe-state checking and rollback.
- Blocking waits for temporarily unavailable resources.
- Functional, invalid-operation, and stress testing.

---

# 2. Theoretical Background

## 2.1 Deadlock

Deadlock occurs when a set of processes or threads becomes permanently blocked because each is waiting for a resource held by another.

The four necessary conditions for deadlock are:

1. **Mutual Exclusion:** At least one resource cannot be shared simultaneously.
2. **Hold and Wait:** A process holds resources while waiting for additional resources.
3. **No Preemption:** Resources cannot be forcibly taken away from a process.
4. **Circular Wait:** A circular chain of processes exists, with each process waiting for a resource held by the next process.

Banker's Algorithm avoids unsafe resource allocations by checking the system state before granting a request.

## 2.2 Safe and Unsafe States

A safe state is a state in which at least one safe sequence exists that allows all customers to complete.

An unsafe state does not necessarily mean that a deadlock has already occurred. It means that the system can no longer guarantee that every customer can finish.

The Banker grants a tentative request only if the safety algorithm confirms that the resulting state is safe.

## 2.3 Data Structures

Banker's Algorithm uses four important data structures.

### Available

Available represents the number of currently available instances of each resource type.

### Maximum

Maximum represents the maximum number of instances of each resource type that each customer may require.

### Allocation

Allocation represents the number of resources currently allocated to each customer.

### Need

Need represents the remaining resources required by each customer to complete execution.

The relationship is:

**Need = Maximum - Allocation**

## 2.4 Safety Algorithm

The safety algorithm determines whether the system is in a safe state.

It uses two temporary data structures:

- **Work:** Represents the currently available resources during safety checking.
- **Finish:** Indicates whether a customer can complete its execution.

The algorithm follows these steps:

1. Initialize Work with the Available vector.
2. Initialize all Finish values to false.
3. Find a customer whose Finish value is false and whose Need is less than or equal to Work.
4. Assume that customer completes execution.
5. Add its allocated resources to Work.
6. Mark its Finish value as true.
7. Repeat until all customers finish or no eligible customer can be found.
8. If every customer can finish, the system is safe.

## 2.5 Resource Request Algorithm

When a customer requests resources, the Banker performs these checks:

1. Verify that the request does not exceed the customer's remaining Need.
2. Check whether the requested resources are currently available.
3. If resources are unavailable, wait for a resource-release notification.
4. Tentatively allocate the requested resources.
5. Execute the safety algorithm.
6. Grant the request if the resulting state is safe.
7. Otherwise, roll back the tentative allocation and reject the request.

---

# 3. System Configuration

## 3.1 Initial System Values

The project uses the classic Banker's Algorithm example with five customers and three resource types.

Initial Available vector:

| Resource | Available |
|---|---:|
| R1 | 3 |
| R2 | 3 |
| R3 | 2 |

### Maximum Matrix

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 7 | 5 | 3 |
| C1 | 3 | 2 | 2 |
| C2 | 9 | 0 | 2 |
| C3 | 2 | 2 | 2 |
| C4 | 4 | 3 | 3 |

### Allocation Matrix

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 0 | 1 | 0 |
| C1 | 2 | 0 | 0 |
| C2 | 3 | 0 | 2 |
| C3 | 2 | 1 | 1 |
| C4 | 0 | 0 | 2 |

### Need Matrix

Need is calculated using Maximum minus Allocation.

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 7 | 4 | 3 |
| C1 | 1 | 2 | 2 |
| C2 | 6 | 0 | 0 |
| C3 | 0 | 1 | 1 |
| C4 | 4 | 3 | 1 |

## 3.2 Initial Safe Sequence

The initial safety algorithm produces the following safe sequence:

**C1 → C3 → C4 → C0 → C2**

| Step | Customer | Work Before Execution | Allocation Released | Work After Execution |
|---|---|---|---|---|
| Initial | - | [3, 3, 2] | - | [3, 3, 2] |
| 1 | C1 | [3, 3, 2] | [2, 0, 0] | [5, 3, 2] |
| 2 | C3 | [5, 3, 2] | [2, 1, 1] | [7, 4, 3] |
| 3 | C4 | [7, 4, 3] | [0, 0, 2] | [7, 4, 5] |
| 4 | C0 | [7, 4, 5] | [0, 1, 0] | [7, 5, 5] |
| 5 | C2 | [7, 5, 5] | [3, 0, 2] | [10, 5, 7] |

Since all five customers can complete, the initial system is in a safe state.

---

# 4. System Architecture and Implementation

## 4.1 Technologies Used

| Technology | Purpose |
|---|---|
| C Programming Language | Core implementation |
| GCC | Compilation |
| POSIX Threads | Concurrent customer execution |
| Pthreads Mutex | Protecting shared resource data |
| Pthreads Condition Variables | Managing waiting threads |
| Ubuntu / WSL | Development environment |
| Git and GitHub | Version control and collaboration |

## 4.2 Project Modules

The implementation is divided into the following modules.

### Main Module – main.c

The main module initializes the Banker structure, defines the initial resource matrices, calculates Need, checks the initial safety state, creates customer threads, joins them, and verifies the final resource state.

### Banker Module – banker.c and banker.h

This module implements the core Banker's Algorithm functionality.

Its major functions include:

- bankerInit()
- bankerDestroy()
- calculateNeed()
- isSafeState()
- requestResources()
- releaseResources()

### Customer Module – customer.c and customer.h

This module represents customers as independent threads.

Each thread requests resources, performs simulated execution after receiving approval, releases its resources, and terminates.

### Testing Module – tests/

The tests directory contains independent programs for checking safety, resource requests, unsafe allocations, resource waiting, invalid operations, and concurrent stress scenarios.

## 4.3 Multithreading Implementation

The project creates five customer threads using POSIX Threads.

Each thread receives a customer ID and a pointer to the shared Banker structure.

The main thread waits for all successfully created customer threads using pthread_join().

This ensures that final resource verification is performed only after the customer threads have completed.

## 4.4 Mutex Synchronization

The Banker structure contains a pthread mutex.

The mutex protects the shared resource matrices during allocation, safety checking, rollback, and release operations.

Only one thread at a time can modify the protected resource state.

This prevents race conditions caused by simultaneous updates.

## 4.5 Condition Variable Synchronization

A customer may request resources that are temporarily unavailable.

In this situation, the request operation uses pthread_cond_wait().

The waiting operation releases the mutex while the thread sleeps and reacquires it when the thread is notified.

When resources are released, the Banker calls pthread_cond_broadcast() to notify waiting threads.

Each awakened thread checks the availability condition again before proceeding.

## 4.6 Rollback Mechanism

The Banker temporarily modifies Available, Allocation, and Need before executing the safety algorithm.

If the resulting state is unsafe, the previous values are restored.

This rollback mechanism prevents unsafe resource allocations from becoming permanent.

---

# 5. Testing and Validation

Testing is performed using independent test programs.

All tests are compiled using GCC with warning checks and POSIX thread support.

The compilation options used include:

- -Wall
- -Wextra
- -std=c11
- -pthread

## 5.1 Safe-State Test

**Test file:** tests/test_safe.c

Purpose:

To verify the Need matrix calculation and determine whether the initial system is safe.

Observed result:

- Need matrix calculated correctly.
- System identified as SAFE.
- Safe sequence generated: C1 → C3 → C4 → C0 → C2.

**Result: PASSED**

## 5.2 Resource Request Test

**Test file:** tests/test_request.c

Purpose:

To verify successful resource allocation and release.

Customer C1 requests:

[1, 0, 2]

Observed result:

- Request granted.
- Available resources updated to [2, 3, 0].
- Resources released successfully.
- Available resources restored to [3, 3, 2].

**Result: PASSED**

## 5.3 Unsafe Request Test

**Test file:** tests/test_unsafe.c

Purpose:

To verify that an unsafe request is rejected and the previous resource state is restored.

Customer C4 requests:

[3, 3, 0]

Observed result:

- Request correctly denied.
- Rollback verification passed.
- Available resources restored to [3, 3, 2].
- Customer C4 allocation restored to [0, 0, 2].
- Final system state remained SAFE.

**Result: PASSED**

## 5.4 Resource Waiting Test

**Test file:** tests/test_wait.c

Purpose:

To verify condition-variable-based waiting when resources are temporarily unavailable.

Observed result:

- Customer C0 received the available R1 resources.
- Customer C1 waited for resources.
- Customer C0 released its resources.
- Customer C1 was notified and successfully received resources.
- Final Available vector was [3, 3, 2].

**Result: PASSED**

## 5.5 Invalid Request Test

**Test file:** tests/test_invalid.c

Purpose:

To verify that invalid requests and release operations are rejected.

The test checks:

1. Invalid customer ID during a request.
2. Negative resource request.
3. Request exceeding the customer's remaining Need.
4. Invalid customer ID during release.
5. Negative resource release.
6. Release exceeding the customer's allocation.

Observed result:

- All six invalid operations were rejected.
- Shared resource data remained unchanged.
- Resource-state integrity was maintained.

**Result: PASSED**

## 5.6 Concurrent Stress Test

**Test file:** tests/test_stress.c

Purpose:

To evaluate resource management under repeated concurrent operations.

Test configuration:

- Five customer threads.
- One hundred transactions per customer.
- Five hundred expected successful transactions in total.

Observed result:

| Metric | Result |
|---|---:|
| Customer threads | 5 |
| Transactions per customer | 100 |
| Expected transactions | 500 |
| Successful transactions | 500 |
| Failed transactions | 0 |
| Resource consistency | PASSED |
| Final system safety | SAFE |

**Final Result: STRESS TEST PASSED**

The test confirms that the implementation completed the tested concurrent workload while maintaining the expected resource state.

---

# 6. Results and Discussion

The implementation successfully demonstrates deadlock avoidance in a multithreaded environment.

The safety test confirms that the initial resource configuration has a valid safe sequence.

The successful request test demonstrates that resources can be allocated and released while maintaining the resource accounting values.

The unsafe request test confirms that tentative allocations are rolled back when they would result in an unsafe state.

The resource waiting test demonstrates the use of condition variables to suspend and resume customer threads when resources become available.

The invalid-operation test confirms that invalid requests and releases do not modify the shared resource state.

Finally, the concurrent stress test completed 500 successful transactions without failed transactions or resource inconsistency.

These results demonstrate the effectiveness of combining Banker's Algorithm with mutex and condition-variable synchronization for the tested workloads.

---

# 7. Limitations

The current implementation has the following limitations:

1. The number of customers and resource types is fixed at compile time.
2. Customer resource requests are predefined.
3. The implementation uses a single shared Banker structure.
4. A customer may wait indefinitely if the required resources are never released.
5. The condition-variable implementation does not guarantee fairness among waiting threads.
6. Request timeouts and thread cancellation are not implemented.
7. Performance evaluation across different numbers of threads and resource configurations has not yet been performed.

---

# 8. Future Enhancements

The following improvements can be considered in future versions:

- Dynamic input for customer resource requests.
- Configurable numbers of customers and resource types.
- A graphical interface to visualize resource allocation.
- Fair scheduling of waiting customer threads.
- Request timeout and cancellation support.
- Detailed execution logging.
- Performance comparison under different thread counts.
- Additional randomized stress testing.

---

# 9. Conclusion

The project successfully implements a multithreaded version of Banker's Algorithm using C and POSIX Threads.

The system supports concurrent customer threads that request and release shared resources. Before granting a request, the Banker validates the request and checks whether the resulting system state is safe.

Mutex locks ensure synchronized access to shared resource data, while condition variables allow customer threads to wait until resources become available. Unsafe allocations are rolled back, and invalid operations are rejected without modifying the system state.

The implementation was validated through functional tests, unsafe-request tests, resource-waiting tests, invalid-operation tests, and a concurrent stress test involving 500 successful transactions.

Overall, the project demonstrates the practical application of deadlock avoidance, multithreading, synchronization, and resource management concepts in operating systems.

---

# 10. References

1. Abraham Silberschatz, Peter B. Galvin, and Greg Gagne, *Operating System Concepts*, Wiley.
2. William Stallings, *Operating Systems: Internals and Design Principles*, Pearson.
3. POSIX Threads Programming documentation.
4. Linux manual pages: pthread_create(), pthread_join(), pthread_mutex_lock(), pthread_cond_wait(), and pthread_cond_broadcast().
5. GCC documentation.
