# Skill: Embedded OO Coding Standards

## Purpose

Ensure coding agents use object-oriented design patterns that improve maintainability, testability, and safety in embedded firmware.

## Use This Skill When

- Writing new C or C++ firmware modules.
- Refactoring procedural code into cohesive components.
- Reviewing pull requests for architecture and code quality.

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
