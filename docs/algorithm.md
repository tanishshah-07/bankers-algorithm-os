
# Banker's Algorithm – Technical Documentation

## 1. Introduction

Banker's Algorithm is a deadlock-avoidance algorithm introduced by Edsger W. Dijkstra.

It determines whether granting a resource request will leave the system in a safe state.

A system is considered safe if there exists at least one sequence in which every process can complete its execution using the currently available resources and the resources released by previously completed processes.

This project implements Banker's Algorithm in C with POSIX Threads to demonstrate safe resource allocation in a concurrent environment.

---

## 2. System Model

The system consists of:

- 5 customers (C0 to C4).
- 3 resource types (R1, R2 and R3).
- Multiple customer threads requesting shared resources.

The following data structures are maintained.

### 2.1 Available Vector

Available represents the number of currently free instances of each resource type.

Initial value:

```text
Available = [3, 3, 2]
```

### 2.2 Maximum Matrix

Maximum represents the maximum number of instances of each resource type that a customer may require.

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 7 | 5 | 3 |
| C1 | 3 | 2 | 2 |
| C2 | 9 | 0 | 2 |
| C3 | 2 | 2 | 2 |
| C4 | 4 | 3 | 3 |

### 2.3 Allocation Matrix

Allocation represents the resources currently assigned to each customer.

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 0 | 1 | 0 |
| C1 | 2 | 0 | 0 |
| C2 | 3 | 0 | 2 |
| C3 | 2 | 1 | 1 |
| C4 | 0 | 0 | 2 |

### 2.4 Need Matrix

Need represents the remaining resources required by each customer to complete its maximum demand.

The formula is:

```text
Need = Maximum - Allocation
```

Calculated Need Matrix:

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 7 | 4 | 3 |
| C1 | 1 | 2 | 2 |
| C2 | 6 | 0 | 0 |
| C3 | 0 | 1 | 1 |
| C4 | 4 | 3 | 1 |

---

## 3. Safety Algorithm

The safety algorithm determines whether the current resource allocation state is safe.

### 3.1 Variables

- Work: Temporary vector representing resources available during the safety simulation.
- Finish: Boolean array indicating whether each customer can finish.
- SafeSequence: Stores the order in which customers can complete execution.

### 3.2 Algorithm

1. Initialize Work = Available.
2. Initialize Finish[i] = false for every customer.
3. Find an unfinished customer whose Need is less than or equal to Work.
4. If such a customer is found:
   - Add its Allocation to Work.
   - Mark Finish[i] = true.
   - Add the customer to SafeSequence.
5. Repeat until all customers finish.
6. If no unfinished customer can proceed, the state is unsafe.
7. If every customer finishes, the state is safe.

### 3.3 Pseudocode

```text
SafetyAlgorithm(Available, Allocation, Need):

    Work = Available

    for each customer i:
        Finish[i] = false

    count = 0

    while count < NumberOfCustomers:

        found = false

        for each customer i:

            if Finish[i] == false:

                if Need[i] <= Work:

                    Work = Work + Allocation[i]

                    Finish[i] = true

                    SafeSequence[count] = i

                    count = count + 1

                    found = true

        if found == false:
            return UNSAFE

    return SAFE
```

### 3.4 Example Safe Sequence

For the initial system configuration, one valid safe sequence is:

```text
C1 -> C3 -> C4 -> C0 -> C2
```

The sequence can be verified using the Work vector.

| Step | Customer | Work Before | Allocation Released | Work After |
|---|---|---|---|---|
| Initial | - | - | - | [3, 3, 2] |
| 1 | C1 | [3, 3, 2] | [2, 0, 0] | [5, 3, 2] |
| 2 | C3 | [5, 3, 2] | [2, 1, 1] | [7, 4, 3] |
| 3 | C4 | [7, 4, 3] | [0, 0, 2] | [7, 4, 5] |
| 4 | C0 | [7, 4, 5] | [0, 1, 0] | [7, 5, 5] |
| 5 | C2 | [7, 5, 5] | [3, 0, 2] | [10, 5, 7] |

All five customers can complete execution.

Therefore, the initial system state is SAFE.

---

## 4. Resource Request Algorithm

When a customer requests resources, the system validates the request before allocation.

Let Request[i] represent the requested resources of customer i.

### 4.1 Validation Conditions

The request must satisfy:

```text
Request[i] <= Need[i]
```

This ensures that the customer does not exceed its declared maximum requirement.

The requested amount must also be nonnegative.

If the requested resources are currently unavailable, the customer waits on the condition variable.

When resources are released, the waiting customer wakes up and checks availability again.

### 4.2 Tentative Allocation

If resources are available, the system temporarily allocates them.

The following operations are performed:

```text
Available = Available - Request

Allocation[i] = Allocation[i] + Request

Need[i] = Need[i] - Request
```

### 4.3 Safety Verification

The safety algorithm is executed after tentative allocation.

Two outcomes are possible.

**Case 1: Safe state**

If the resulting state is safe, the request is granted.

**Case 2: Unsafe state**

If the resulting state is unsafe, the allocation is rolled back.

```text
Available = Available + Request

Allocation[i] = Allocation[i] - Request

Need[i] = Need[i] + Request
```

The request is rejected.

### 4.4 Pseudocode

```text
RequestResources(customer i, Request):

    acquire mutex

    if Request < 0 or Request > Need[i]:
        release mutex
        return INVALID

    while Request > Available:
        wait on condition variable

    Available = Available - Request

    Allocation[i] = Allocation[i] + Request

    Need[i] = Need[i] - Request

    if SafetyAlgorithm() == SAFE:

        release mutex

        return GRANTED

    else:

        Available = Available + Request

        Allocation[i] = Allocation[i] - Request

        Need[i] = Need[i] + Request

        release mutex

        return UNSAFE
```

The condition-variable wait atomically releases the mutex while waiting and reacquires it before returning.

---

## 5. Resource Release Algorithm

When a customer completes its work, it releases the resources it acquired.

The release operation validates that the customer actually owns the resources being released.

The following conditions must hold:

```text
Release >= 0

Release <= Allocation[i]
```

The resource matrices are then updated.

```text
Available = Available + Release

Allocation[i] = Allocation[i] - Release

Need[i] = Need[i] + Release
```

After releasing resources, the system broadcasts a notification using:

```c
pthread_cond_broadcast(&banker->resourcesChanged);
```

Waiting customer threads can then wake up and recheck resource availability.

---

## 6. Synchronization in the Multithreaded Implementation

Multiple customer threads access shared Banker data.

Without synchronization, concurrent modifications could cause race conditions and inconsistent resource allocation.

The project uses two POSIX synchronization mechanisms.

### 6.1 Mutex Lock

The Banker structure contains:

```c
pthread_mutex_t mutex;
```

The mutex protects the shared resource matrices.

It ensures that the following operations are performed atomically with respect to other customer threads:

- Resource request validation.
- Tentative allocation.
- Safety checking.
- Allocation rollback.
- Resource release.

### 6.2 Condition Variable

The Banker structure also contains:

```c
pthread_cond_t resourcesChanged;
```

A customer waits using:

```c
pthread_cond_wait(
    &banker->resourcesChanged,
    &banker->mutex
);
```

When another customer releases resources, the system broadcasts a notification.

The waiting customer wakes up and checks availability again.

### 6.3 Thread Lifecycle

Each customer thread performs the following operations:

1. Start execution.
2. Submit a resource request.
3. Wait if the requested resources are unavailable.
4. Receive resources if the request is safe.
5. Simulate execution.
6. Release acquired resources.
7. Terminate.

---

## 7. Resource Accounting Invariant

The total number of instances of each resource type must remain constant.

For every resource type j:

```text
Total[j] = Available[j] + Sum(Allocation[i][j])
```

Resource requests transfer resources from Available to Allocation.

Resource releases transfer resources from Allocation back to Available.

Neither operation should change the total number of resource instances.

The project verifies this property by comparing the final Available and Allocation matrices with their initial values after the customer threads complete.

---

## 8. Conclusion

Banker's Algorithm provides a systematic approach to deadlock avoidance by checking the safety of every proposed allocation.

The multithreaded implementation combines this algorithm with POSIX mutexes and condition variables to coordinate concurrent customer operations.

The implementation was tested for safe-state detection, resource requests, unsafe-state rollback, condition-variable waiting, invalid operations, and concurrent stress workloads.

The stress test successfully completed 500 resource-request transactions across five customer threads, with zero failed transactions and correct final resource accounting.
```
