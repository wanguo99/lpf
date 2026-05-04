# Contributing to EMS

Thank you for your interest in contributing to EMS! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help maintain a welcoming environment for all contributors

## How to Contribute

### Reporting Bugs

Before creating a bug report:
1. Check existing issues to avoid duplicates
2. Collect relevant information (OS, architecture, build configuration)
3. Create a minimal reproducible example if possible

Include in your bug report:
- Clear description of the issue
- Steps to reproduce
- Expected vs actual behavior
- Environment details (OS, compiler, architecture)
- Relevant logs or error messages

### Suggesting Enhancements

Enhancement suggestions are welcome! Please:
- Clearly describe the proposed feature
- Explain the use case and benefits
- Consider backward compatibility
- Discuss implementation approach if you have ideas

### Pull Requests

1. **Fork and Branch**
   - Fork the repository
   - Create a feature branch: `git checkout -b feature/your-feature-name`

2. **Code Standards**
   - Follow the coding style in [docs/CODING_STANDARDS.md](docs/CODING_STANDARDS.md)
   - Aim for MISRA C compliance where practical
   - Write clear, self-documenting code
   - Add comments only when the "why" is non-obvious

3. **Testing**
   - Add tests for new functionality
   - Ensure all existing tests pass
   - Test on multiple architectures if possible
   - Run the test suite: `cmake --build build --target test`

4. **Documentation**
   - Update relevant documentation
   - Add API documentation for new interfaces
   - Update CHANGELOG.md following [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) format

5. **Commit Messages**
   - Use clear, descriptive commit messages
   - Format: `<type>: <description>`
   - Types: feat, fix, docs, style, refactor, test, chore
   - Example: `feat: add I2C bus timeout configuration`

6. **Submit PR**
   - Push to your fork
   - Create a pull request with clear description
   - Reference any related issues
   - Be responsive to review feedback

## Development Setup

### Prerequisites

- CMake 3.20+
- GCC 9+ or Clang 10+
- Git

### Building

```bash
# Configure
cmake --preset=debug

# Build
cmake --build build/debug

# Run tests
ctest --test-dir build/debug
```

### Cross-Compilation

See [docs/BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md) for cross-compilation instructions.

## Architecture Guidelines

### Layer Boundaries

- **OSAL**: OS abstraction only, no hardware access
- **HAL**: Hardware drivers, no business logic
- **PCL**: Configuration management, no runtime logic
- **PDL**: Peripheral services, uses HAL for hardware access
- **Apps**: Application logic, uses PDL/PCL interfaces

### Dependency Rules

- Lower layers cannot depend on upper layers
- Each layer has clear, minimal interfaces
- Avoid circular dependencies

### Error Handling

- Use return codes for recoverable errors
- Log errors with appropriate severity
- Clean up resources on error paths
- Document error conditions in API

### Thread Safety

- Document thread-safety guarantees
- Use OSAL primitives for synchronization
- Avoid global mutable state
- Protect shared resources with mutexes

## Testing

### Unit Tests

- Test individual functions in isolation
- Use the test framework in `tests/`
- Aim for high code coverage
- Test error paths and edge cases

### Integration Tests

- Test layer interactions
- Verify configuration handling
- Test multi-threaded scenarios
- Validate error recovery

### Hardware Tests

- Test on real hardware when possible
- Document hardware requirements
- Provide simulation alternatives

## Review Process

1. Automated checks run on PR submission
2. Maintainers review code and provide feedback
3. Address review comments
4. Maintainers approve and merge

## License

By contributing, you agree that your contributions will be licensed under the GNU General Public License v3.0.

## Questions?

Feel free to open an issue for questions or discussion.
