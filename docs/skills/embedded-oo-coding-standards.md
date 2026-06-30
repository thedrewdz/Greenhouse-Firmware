# Skill: Embedded OO Coding Standards

## Purpose

Ensure coding agents use object-oriented design patterns that improve maintainability, testability, and safety in embedded firmware.

## Use This Skill When

- Writing new C or C++ firmware modules.
- Refactoring procedural code into cohesive components.
- Reviewing pull requests for architecture and code quality.

## Language Boundary (C / C++)

This repository is pragmatic about language by layer:

- Use C++ for the application, service, and device-abstraction layers, where classes, small interfaces, and dependency injection carry their weight.
- C is acceptable for low-level drivers and the HAL, where a C-style API is clearer or matches an ESP-IDF driver.
- "Object-oriented" here means the discipline, not the keyword. At a C boundary, express it with opaque structs plus a small function-pointer table (or a clear ownership/handle pattern), so the layer above still programs to a small interface and can be tested with a mock.
- Keep the boundary explicit: a C driver exposes a narrow header; the C++ layer wraps it behind the capability interface rather than letting C structs leak upward.
- Apply the file conventions accordingly (`.hpp`/`.cpp` for C++, `.h`/`.c` for C), per `esp-idf-firmware-practices.md`.

## Core OO Rules

- One class or module should have one primary responsibility.
- Program to interfaces, not concrete hardware classes.
- Hide hardware details behind abstractions.
- Keep mutable global state to a minimum.
- Prefer composition over deep inheritance trees.

## Interface Design Rules

- Define small interfaces for each capability domain, for example sensor reader, actuator driver, transport client.
- Avoid broad manager interfaces with unrelated methods.
- Keep method names behavior-based and deterministic.
- Return explicit result objects or error codes, not hidden side effects.

## Class Design Rules

- Separate orchestration classes from hardware access classes.
- Keep constructors lightweight; avoid heavy IO in constructors.
- Place retry and timeout policies in services, not in every driver.
- Keep serialization and parsing isolated in codec classes.

## Testing and Verification Rules

- Make business logic testable without real hardware.
- Use mock implementations for transport and slot modules.
- Add contract tests for command parse, validation, and response mapping.
- Require deterministic behavior for the same inputs.

## Code Review Checklist

- Does each class have one clear responsibility?
- Are dependencies expressed as interfaces?
- Are failure paths explicit and testable?
- Is duplicate logic removed into shared components?
- Is naming consistent with docs and topic contracts?
