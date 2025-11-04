# Architecture Overview

The application follows a modular Qt Widgets architecture:

- `MainWindow` hosts the hover-activated `Sidebar` and a `QStackedWidget` for page navigation.
- Each page (homepage, timetable, tasks, settings) is implemented as a dedicated widget deriving from `QWidget`.
- Persistent data is managed through `JsonManager`, which ensures JSON files are created from defaults on first launch.
- Custom painting (e.g., the donut chart) lives in specialised widgets such as `DonutChartWidget`.

Further details are documented inline with each component.
