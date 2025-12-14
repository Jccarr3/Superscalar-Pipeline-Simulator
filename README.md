# Superscalar Out-of-Order Processor Simulator

This project implements a cycle-accurate simulator of a superscalar, out-of-order processor pipeline with register renaming, an instruction queue (IQ), and a reorder buffer (ROB). The simulator is parameterized by ROB size, IQ size, and pipeline width, and is designed to study how these microarchitectural parameters affect performance (IPC) across different instruction traces.

The project also includes an experimentation driver and script to automate sweeping architectural configurations and collecting performance results.

## Features

- Superscalar pipeline with configurable fetch/issue width

- Out-of-order execution using:

  - Register Alias Table (RMT)

  - Reorder Buffer (ROB)

  - Instruction Queue (IQ)

- Register renaming and dependency tracking

- Multiple functional unit latencies

  - Type 0: 1 cycle

  - Type 1: 2 cycles

  - Type 2: 5 cycles

- Wakeup and select logic across IQ, Dispatch, and Register Read

- In-order retirement

- Cycle-accurate timing for all pipeline stages

- Automated performance experimentation via shell script

## Pipeline Stages

The simulator models the following pipeline stages:

1. DE – Decode

2. RN – Rename

3. RR – Register Read

4. DI – Dispatch

5. IS – Issue

6. EX – Execute

7. WB – Writeback

8. RT – Retire

Each instruction records the cycle it enters every stage, enabling detailed timing analysis.

## Experimentation Mode (experimentation.cc)

- Outputs only IPC

- Intended for batch execution and plotting

## Automated Experiments

The proc.sh script runs a series of experiments to explore how ROB size, IQ size, and pipeline width affect performance across multiple traces.

#### Example Experiment Types

- Effect of ROB size with near-ideal IQ sizing

- Effect of pipeline width scaling

- Comparison across different benchmark traces

## Design Notes

- The ROB is implemented as a circular buffer

- Instructions issue based on oldest-ready-first policy

- Register readiness propagates through wakeup logic in:

  - IQ

  - Dispatch stage

  - Register Read stage

- Simulation terminates only after:

  - Pipeline stages are empty

  - IQ is empty

  - ROB is empty

## Intended Use

This simulator is designed for:

- Studying superscalar and out-of-order execution

- Performance sensitivity analysis of microarchitectural structures
