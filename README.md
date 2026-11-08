
# Multithreaded Banker's Algorithm

## Operating Systems Project

A multithreaded implementation of Banker's Algorithm using the C programming language and POSIX Threads (Pthreads).

The project demonstrates deadlock avoidance through safe resource allocation, concurrent customer execution, mutex synchronization, condition variables, and resource management.

---

## 1. Project Overview

Banker's Algorithm is a deadlock-avoidance algorithm used by operating systems to allocate resources safely among multiple processes.

This project implements Banker's Algorithm in a multithreaded environment where multiple customer threads request and release shared system resources concurrently.

Before granting a resource request, the system checks whether the resulting state remains safe. Requests that would lead to an unsafe state are rejected.

The implementation also uses POSIX synchronization mechanisms to prevent race conditions while accessing shared resource data.

## 2. Objectives

- Implement Banker's Safety Algorithm in C.
- Support multiple concurrent customer threads using Pthreads.
- Implement resource request and release operations.
- Prevent unsafe resource allocation.
- Use mutex locks to protect shared resource data.
- Use condition variables to handle temporarily unavailable resources.
- Demonstrate safe resource allocation and deadlock avoidance.
- Perform functional, validation, and concurrent stress testing.

## 3. Technologies Used

| Technology | Purpose |
|---|---|
| C | Core implementation |
| GCC | Compilation |
| POSIX Threads | Multithreading |
| Pthread Mutex | Mutual exclusion |
| Pthread Condition Variables | Thread waiting and notification |
| Ubuntu / Linux | Development environment |
| Git and GitHub | Version control |

## 4. System Configuration

The implementation uses:

- Number of customers: 5
- Number of resource types: 3

### Initial Available Resources

| Resource | Available |
|---|---:|
| R1 | 3 |
| R2 | 3 |
| R3 | 2 |

### Maximum Resource Matrix

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 7 | 5 | 3 |
| C1 | 3 | 2 | 2 |
| C2 | 9 | 0 | 2 |
| C3 | 2 | 2 | 2 |
| C4 | 4 | 3 | 3 |

### Initial Allocation Matrix

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 0 | 1 | 0 |
| C1 | 2 | 0 | 0 |
| C2 | 3 | 0 | 2 |
| C3 | 2 | 1 | 1 |
| C4 | 0 | 0 | 2 |

### Initial Need Matrix

Need = Maximum - Allocation

| Customer | R1 | R2 | R3 |
|---|---:|---:|---:|
| C0 | 7 | 4 | 3 |
| C1 | 1 | 2 | 2 |
| C2 | 6 | 0 | 0 |
| C3 | 0 | 1 | 1 |
| C4 | 4 | 3 | 1 |

One valid initial safe sequence is:

**C1 → C3 → C4 → C0 → C2**

## 5. Project Architecture

```text
bankers-algorithm-os/
│
├── README.md
├── .gitignore
│
├── src/
│   ├── main.c
│   ├── banker.c
│   ├── banker.h
│   ├── customer.c
│   └── customer.h
│
├── tests/
│   ├── test_safe.c
│   ├── test_request.c
│   ├── test_unsafe.c
│   ├── test_wait.c
│   ├── test_invalid.c
│   └── test_stress.c
│
├── docs/
│   ├── algorithm.md
│   ├── design.md
│   └── project-report.md
│
├── results/
└── screenshots/
```

## 6. Module Description

### banker.h
Defines the Banker structure, resource configuration, synchronization objects, and function declarations.

### banker.c
Implements:
- Banker initialization and cleanup.
- Need matrix calculation.
- Safety algorithm.
- Resource request validation.
- Safe resource allocation.
- Unsafe request rollback.
- Resource release.
- Mutex and condition-variable synchronization.

### customer.h
Defines the Customer structure and customer thread function declaration.

### customer.c
Implements customer threads that request resources, simulate execution, and release resources.

### main.c
Initializes the system, creates customer threads, waits for their completion, and verifies final resource consistency.

### tests/
Contains independent test programs for safety, resource requests, unsafe requests, waiting, invalid operations, and concurrent stress testing.

## 7. Synchronization Mechanisms

### Mutex

A pthread mutex protects shared Banker data.

It ensures that only one thread at a time can perform a resource allocation or release transaction.

### Condition Variable

A pthread condition variable allows a customer thread to wait when the requested resources are temporarily unavailable.

When another customer releases resources, the system broadcasts a notification to waiting threads.

### Deadlock Avoidance

Before a request is granted, the system performs a safety check.

If the resulting state is unsafe, the tentative allocation is rolled back and the request is rejected.

## 8. Compilation and Execution

### Requirements

- GCC compiler
- Linux or Ubuntu environment
- POSIX Threads support

### Clone the Repository

```bash
git clone https://github.com/tanishshah-07/bankers-algorithm-os.git
cd bankers-algorithm-os
```

### Compile the Main Program

```bash
gcc -Wall -Wextra -std=c11 -pthread \
src/main.c src/banker.c src/customer.c -o banker
```

### Run

```bash
./banker
```

## 9. Testing

The project includes six independent test programs.

| Test File | Description |
|---|---|
| test_safe.c | Verifies safe-state detection and safe sequence |
| test_request.c | Tests resource allocation and release |
| test_unsafe.c | Tests unsafe request rejection and rollback |
| test_wait.c | Tests condition-variable waiting and notification |
| test_invalid.c | Tests invalid requests and releases |
| test_stress.c | Tests concurrent resource operations |

### Compile and Run Tests

```bash
gcc -Wall -Wextra -std=c11 -pthread tests/test_safe.c src/banker.c -o test_safe
./test_safe

gcc -Wall -Wextra -std=c11 -pthread tests/test_request.c src/banker.c -o test_request
./test_request

gcc -Wall -Wextra -std=c11 -pthread tests/test_unsafe.c src/banker.c -o test_unsafe
./test_unsafe

gcc -Wall -Wextra -std=c11 -pthread tests/test_wait.c src/banker.c -o test_wait
./test_wait

gcc -Wall -Wextra -std=c11 -pthread tests/test_invalid.c src/banker.c -o test_invalid
./test_invalid

gcc -Wall -Wextra -std=c11 -pthread tests/test_stress.c src/banker.c -o test_stress
./test_stress
```

## 10. Verified Results

The following results were obtained during project testing:

| Test | Verified Result |
|---|---|
| Safety test | Safe sequence identified |
| Resource request test | Request granted and resources released |
| Unsafe request test | Request denied and allocation rolled back |
| Waiting test | Waiting customer awakened after resource release |
| Invalid request test | All 6 validation cases passed |
| Stress test | 500/500 successful transactions |
| Final consistency | Available and Allocation matrices restored |
| Final system state | SAFE |

### Stress Test Summary

The concurrent stress test used:

- 5 customer threads.
- 100 transactions per customer.
- 500 total resource-request transactions.

Results:

- Successful transactions: 500
- Failed transactions: 0
- Resource consistency: PASSED
- Final system safety: SAFE

## 11. Conclusion

The project successfully implements a multithreaded version of Banker's Algorithm using C and POSIX Threads.

The implementation demonstrates safe resource allocation, deadlock avoidance, concurrent customer execution, mutual exclusion, condition-variable synchronization, and resource release.

Functional tests, validation tests, waiting tests, and concurrent stress tests were successfully executed.

The project demonstrates how operating-system synchronization mechanisms can be combined with deadlock-avoidance algorithms to manage shared resources safely.

## 12. Future Enhancements

- Support dynamically configurable customers and resources.
- Add interactive resource request input.
- Implement detailed execution logging.
- Add graphical visualization of resource allocation.
- Extend testing to larger workloads and more complex request patterns.

---

## Repository

GitHub: https://github.com/tanishshah-07/bankers-algorithm-os
