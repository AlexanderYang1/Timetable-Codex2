# AI Assistant Guidelines

- Follow C++17 standard and Qt best practices.
- Keep UI-related logic separated into dedicated widgets; do not overload `MainWindow`.
- Maintain consistent naming conventions: classes in PascalCase, members and functions in camelCase.
- Document complex calculations, especially in custom painting code, with inline comments.
- Provide JSON schema references in comments when modifying persistence structures.
- Update this file if new global conventions are introduced.
- When touching documentation, ensure build instructions remain accurate for macOS.
- Include unit or integration test notes in PR descriptions when applicable.
