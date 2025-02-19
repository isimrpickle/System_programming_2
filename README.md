# Job Executor System

## Overview
This project implements a job execution system that enables clients to submit commands to a server, manage their execution, and retrieve results. The system supports concurrent execution with a thread pool and synchronizes operations using mutexes and conditional variables.

## Compilation & Cleanup
To compile the project, run:
```bash
make
```
To clean up executables, FIFO files, and temporary output files:
```bash
make clean
```

## System Components
The system consists of two main components:

### **1. Job Executor Server (`jobExecutorServer`)**
- Creates worker threads (equal to `buffersize`).
- Waits for incoming client connections and creates controller threads to handle them.
- Uses two mutexes and four conditional variables to ensure proper synchronization.
- Manages job execution using a buffer with a thread pool mechanism.
- Handles client commands such as `issueJob`, `stop`, `poll`, `setConcurrency`, and `exit`.

### **2. Job Commander (`jobCommander`)**
- Connects to the server and sends job-related commands.
- Receives and displays results from the server.
- Supports the following commands:
  - `issueJob`: Submits a command for execution.
  - `stop`: Stops a specific job.
  - `poll`: Retrieves the current job status.
  - `setConcurrency`: Adjusts the concurrency level.
  - `exit`: Shuts down the server.

## Synchronization Mechanism
The server utilizes:
- **2 Mutexes**: Ensure safe access to shared resources.
- **4 Conditional Variables**:
  - Control the execution of `exit` and concurrency management.
  - Enforce buffer size and thread pool constraints.

## Thread Functionality
### **Worker Threads**
- Pre-initialized before any client connection.
- Wait until a job is available in the buffer or until concurrency constraints allow execution.
- Upon receiving a job:
  - Extracts the command.
  - Forks a new process to execute the command.
  - Redirects output to a file (`pid.output`).
  - Reads the file and sends the output back to the client.
- After dequeuing a job, signals the controller thread to allow more jobs to enter the buffer.

### **Controller Threads**
- Created for each client connection.
- If the buffer is full, waits for space to become available.
- Signals worker threads to start executing jobs when necessary.
- Implements `setConcurrency` by broadcasting worker threads to run within the concurrency limit.
- Handles `exit` by:
  1. Waiting for all jobs to finish.
  2. Ensuring job results are written to the socket before shutdown.

## Known Issues
- If a job does not return an output, the commander may display gibberish characters due to uninitialized memory being sent.

## Example Execution
### **Start the Server**
```bash
./bin/jobExecutorServer 9001 2 4
```
- `9001`: Port number
- `2`: Thread pool size
- `4`: Buffer size

### **Client Commands**
#### **Submit a job**
```bash
./bin/jobCommander linux07.di.uoa.gr 9001 issueJob ./bin/sleepingtime &
```
- Runs in background mode due to `dup2` usage.

#### **Stop a job**
```bash
./bin/jobCommander linux07.di.uoa.gr 9001 stop job_1
```

#### **Check job status**
```bash
./bin/jobCommander linux07.di.uoa.gr 9001 poll
```

#### **Set concurrency level**
```bash
./bin/jobCommander linux07.di.uoa.gr 9001 setConcurrency 4
```

#### **Terminate the server**
```bash
./bin/jobCommander linux07.di.uoa.gr 9001 exit
```

