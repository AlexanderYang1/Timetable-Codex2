# Timetable Codex 2

A Qt 6 desktop application that combines a minimalist timetable and task manager tailored for school workflows. The UI is built with Qt Widgets and focuses on a responsive sidebar navigation, an activity driven donut chart, a dynamic timetable view, and a fully featured task/subtask manager.

## Features

- Hover-to-reveal sidebar providing navigation between Homepage, Timetable, Tasks, and Settings.
- Homepage with collapsible activity cards and a real-time donut chart that visualises the next 12 hours of activities and class periods.
- Timetable page that adapts to A/B week structures and year-specific Wednesday templates.
- Task list with modal creation workflow, detailed task view, weighted subtasks, and progress calculations.
- Settings page for choosing the current timetable week and year level.
- JSON-based persistence with automatic bootstrap from default templates on first launch.

## Requirements

- Qt 6.9.3 (Widgets module)
- CMake 3.21+
- A C++17 compatible compiler (clang++ or g++)

## Building

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/Users/alexanderyang/Qt/6.9.3/macos
cmake --build build
```

You can also use the included `Makefile`:

```bash
make
```

## Running

```bash
make run
```

On first execution the application will create a writable data directory (platform dependent) and seed it with the default JSON data stored under `resources/defaults`.

## Project Structure

```
include/        Public headers for application components
src/            C++ implementation files
resources/      Qt resource collection (QRC), styles, and default JSON data
data/           Runtime JSON files (created at runtime)
docs/           Additional documentation
```

## JSON Data

All user data is persisted as JSON files inside the writable data directory:

- `activities.json`: Activity definitions surfaced on the homepage and donut chart.
- `tasks.json`: Tasks and weighted subtasks for the task manager.
- `settings.json`: Stores active week and year level.
- `SchoolPeriods.json`: Provided schedule, period times, and subject metadata.

The application validates ranges for start/end times and prevents the creation of events in the past via its dialogs. Corrupt JSON files fall back to empty defaults to keep the UI responsive.

## Contributing

- Follow the conventions listed in `AGENTS.md`.
- Keep UI logic modular—prefer extending widgets instead of growing `MainWindow`.
- Document complex calculations inline (notably inside custom painting code).
