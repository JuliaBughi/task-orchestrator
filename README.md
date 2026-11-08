# Task Orchestrator

A lightweight task orchestration system written in **C** that allows users to submit, schedule, execute, and monitor system commands through a client-server architecture.

The orchestrator supports concurrent task execution, configurable scheduling policies, command pipelines, and automatic output logging. Communication between the client and server is handled through **named pipes (FIFOs)**.

## Features

- **Client-server architecture:** Submit and manage tasks through a dedicated client.
- **Concurrent execution:** Configure the maximum number of tasks running simultaneously.
- **Scheduling policies:**
  - **FCFS (First Come, First Served):** Executes tasks in submission order.
  - **SJF (Shortest Job First):** Prioritizes pending tasks based on their estimated execution time.
- **Command execution:** Execute individual system commands or pipelines.
- **Task monitoring:** Inspect executing, scheduled, and completed tasks.
- **Output management:** Store the output of each task in a dedicated file.
- **Execution statistics:** Track and record task execution times.
- **FIFO-based communication:** Exchange messages between clients and the orchestrator using named pipes.

## Architecture

The project consists of two main components:

| Component | Description |
|---|---|
| `orchestrator` | Server responsible for task scheduling, execution, and monitoring. |
| `client` | Command-line interface used to submit tasks, check their status, and shut down the server. |

The client sends requests to the orchestrator through a server FIFO. Each client also creates its own FIFO to receive responses.

The orchestrator manages pending and running tasks, launches child processes using `fork()` and executes commands using `execvp()`.

## Requirements

The project requires a Linux environment with:

- GCC
- GNU Make
- GLib 2.0 development libraries
- ncurses development libraries
- pkg-config

On Debian/Ubuntu, install the dependencies with:

```bash
sudo apt update
sudo apt install build-essential libglib2.0-dev libncurses-dev pkg-config
```

## Building

Compile the project:

```bash
make
```

## Usage

### 1. Start the orchestrator

Run the server using:

```bash
./bin/orchestrator <output_directory> <max_parallel_tasks> <scheduling_policy>
```

Arguments:

| Argument | Description |
|---|---|
| `output_directory` | Directory where task output files are stored. |
| `max_parallel_tasks` | Maximum number of tasks that can execute simultaneously. |
| `scheduling_policy` | Scheduling algorithm: `fcfs` or `sjf`. |

### 2. Submit tasks

Use the client to submit individual commands:

```bash
./bin/client execute <expected_time> -u "<command>"
```

### 3. Execute command pipelines

Pipelines can be submitted using the `-p` flag:

```bash
./bin/client execute <expected_time> -p "<command1> | <command2>"
```

### 4. Check task status

To inspect the current state of tasks:

```bash
./bin/client status
```

### 5. Shut down the orchestrator

To request a graceful server shutdown:

```bash
./bin/client close
```

## Cleanup

The Makefile provides several cleanup targets:

Remove generated object files and executables:

```bash
make clean_build
```

Remove temporary FIFOs:

```bash
make clean_fifos
```

Remove generated task output:

```bash
make clean_output
```

Remove generated build files, temporary files, logs, and test files:

```bash
make clean
```

## Testing

A helper script is included to execute a collection of example tasks:

```bash
./script.sh
```
