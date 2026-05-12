# Domain Docs

How engineering skills should consume this repo's domain documentation when exploring the codebase.

## Layout

This repo uses a single-context layout:

```text
/
├── CONTEXT.md
├── docs/
│   └── adr/
└── Projects/
```

## Before exploring, read these

- `CONTEXT.md` at the repo root, if present.
- ADRs under `docs/adr/`, if present and relevant to the area being changed.
- Existing project docs under `docs/`, especially hardware and workflow notes that touch the current task.

If any of these files do not exist, proceed silently. Do not require creating them before doing normal work. The `grill-with-docs` skill can create or update them when terminology or architectural decisions become stable.

## Use project vocabulary

When output names a project concept in an issue title, refactor proposal, diagnosis hypothesis, or test name, prefer the term used in `CONTEXT.md` and existing docs.

For this firmware repo, common areas include:

- RT1064 board support and clock configuration
- Keil MDK build, flash, and debug workflow
- UART command interface and serial self-check workflow
- Drive, wheel, encoder, motor, and IMU modules
- FreeRTOS application tasks under `Projects/user/`

If a term is missing from `CONTEXT.md`, treat that as a documentation gap to resolve with `grill-with-docs` when it matters.

## Flag ADR conflicts

If an output contradicts an existing ADR, surface the conflict explicitly instead of silently overriding it.

