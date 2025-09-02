# Contributing to DiskSense64

First off, thank you for considering contributing to DiskSense64! It's people like you that make DiskSense64 such a great tool.

## Code of Conduct

This project and everyone participating in it is governed by the [DiskSense64 Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to conduct@disksense64.org.

## What We're Looking For

We welcome many types of contributions:

- **Bug Reports**: Issues you've discovered
- **Feature Requests**: Ideas for new functionality
- **Code Contributions**: Bug fixes, new features, performance improvements
- **Documentation**: Improvements to docs, tutorials, examples
- **Testing**: Writing tests, identifying edge cases
- **Translations**: Localizing the UI and documentation
- **Design**: UI/UX improvements, icons, graphics
- **Community**: Answering questions, helping newcomers, organizing events

## How to Contribute

### Reporting Bugs

This section guides you through submitting a bug report for DiskSense64. Following these guidelines helps maintainers and the community understand your report, reproduce the behavior, and find related reports.

**Before Submitting A Bug Report**
- Check the [GitHub issues](https://github.com/yourusername/disksense64/issues) to see if the issue has already been reported
- Try reproducing the issue with the latest version
- Check if the issue is already fixed in the development branch

**How Do I Submit A Good Bug Report?**
- **Use a clear and descriptive title** for the issue
- **Describe the exact steps** which reproduce the problem
- **Provide specific examples** to demonstrate the steps
- **Describe the behavior you observed** after following the steps
- **Explain which behavior you expected** to see instead and why
- **Include screenshots** which show you following the described steps
- **Include details about your configuration**:
  - Operating system version
  - DiskSense64 version
  - Installation method
  - Any special configuration settings

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion, including completely new features and minor improvements to existing functionality.

**Before Submitting An Enhancement**
- Check if there's already a feature request for your idea
- Check if the enhancement is already planned in the roadmap
- Consider if the enhancement fits with the project's vision

**How Do I Submit A Good Enhancement Suggestion?**
- **Use a clear and descriptive title** for the issue
- **Provide a step-by-step description** of the suggested enhancement
- **Provide specific examples** to demonstrate the steps
- **Describe the current behavior** and **explain which behavior you expected** to see instead
- **Explain why this enhancement would be useful** to most DiskSense64 users
- **Specify the platform(s)** where this enhancement would be useful

### Your First Code Contribution

Unsure where to begin contributing to DiskSense64? You can start by looking through these `beginner` and `help-wanted` issues:

- [Beginner issues](https://github.com/yourusername/disksense64/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) - issues which should only require a few lines of code, and a test or two.
- [Help wanted issues](https://github.com/yourusername/disksense64/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22) - issues which should be a bit more involved than `beginner` issues.

Both issue lists are sorted by total number of comments. While not perfect, number of comments is a reasonable proxy for impact a given change will have.

### Pull Requests

The process described here has several goals:

- Maintain DiskSense64's quality
- Fix problems that are important to users
- Engage the community in working toward the best possible DiskSense64
- Enable a sustainable system for DiskSense64's maintainers to review contributions

**Steps for Submitting a Pull Request:**

1. **Fork** the repo and create your branch from `main`.
2. **If you've added code that should be tested**, add tests.
3. **If you've changed APIs**, update the documentation.
4. **Ensure the test suite passes**.
5. **Make sure your code lints**.
6. **Issue that pull request**!

### Styleguides

#### Git Commit Messages

- Use the present tense ("Add feature" not "Added feature")
- Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit the first line to 72 characters or less
- Reference issues and pull requests liberally after the first line
- When only changing documentation, include `[ci skip]` in the commit title
- Consider starting the commit message with an applicable emoji:
  - :art: `:art:` when improving the format/structure of the code
  - :racehorse: `:racehorse:` when improving performance
  - :non-potable_water: `:non-potable_water:` when plugging memory leaks
  - :memo: `:memo:` when writing docs
  - :penguin: `:penguin:` when fixing something on Linux
  - :apple: `:apple:` when fixing something on macOS
  - :checkered_flag: `:checkered_flag:` when fixing something on Windows
  - :bug: `:bug:` when fixing a bug
  - :fire: `:fire:` when removing code or files
  - :green_heart: `:green_heart:` when fixing the CI build
  - :white_check_mark: `:white_check_mark:` when adding tests
  - :lock: `:lock:` when dealing with security
  - :arrow_up: `:arrow_up:` when upgrading dependencies
  - :arrow_down: `:arrow_down:` when downgrading dependencies
  - :shirt: `:shirt:` when removing linter warnings

#### C++ Style Guide

All C++ code must adhere to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with the following exceptions:

- Line length: 100 characters instead of 80
- Naming: Use snake_case for variables and functions, PascalCase for classes and structs
- File extensions: Use `.cpp` and `.h` (not `.cc` and `.hpp`)

#### Documentation Style Guide

- Use [Markdown](https://daringfireball.net/projects/markdown) for documentation
- Reference functions and classes using backticks: `function_name()`
- Follow the existing documentation style in the codebase

### Additional Notes

#### Issue and Pull Request Labels

This section lists the labels we use to help us track and manage issues and pull requests.

**Type of Issue and Issue State**
- `bug` - Issues that are bugs
- `enhancement` - Issues that are feature requests
- `documentation` - Issues that are documentation related
- `performance` - Issues related to performance
- `security` - Issues related to security
- `beginner` - Issues that are good for beginners
- `help-wanted` - Issues that need assistance
- `duplicate` - Issues that are duplicates of other issues
- `wontfix` - Issues that won't be fixed
- `invalid` - Issues that are invalid
- `question` - Issues that are questions

**Pull Request Labels**
- `work-in-progress` - Pull requests that are not ready for review
- `needs-review` - Pull requests that need review
- `ready-to-merge` - Pull requests that are ready to merge

#### Development Workflow

1. **Create a branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**
   - Write code following the style guide
   - Add tests for new functionality
   - Update documentation as needed

3. **Commit your changes**
   ```bash
   git add .
   git commit -m ":emoji: Brief description of changes"
   ```

4. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

5. **Create a pull request**
   - Go to the original repository on GitHub
   - Click "New pull request"
   - Select your branch and create the PR

#### Testing

Before submitting a pull request, ensure all tests pass:

```bash
# Build tests
cmake .. -DENABLE_TESTS=ON
make

# Run all tests
ctest

# Run specific test
ctest -R test_name

# Run with verbose output
ctest -V
```

#### Performance Testing

For performance-critical changes, run benchmarks:

```bash
# Build benchmarks
cmake .. -DENABLE_BENCHMARKS=ON
make

# Run benchmarks
./bin/bench_hash
./bin/bench_io
./bin/bench_phash
```

Compare results with baseline to ensure no performance regressions.

#### Code Review Process

All submissions require review. We use GitHub pull requests for this purpose. Consult [GitHub Help](https://help.github.com/articles/about-pull-requests/) for more information on using pull requests.

**Review Process:**
1. Maintainers review the code
2. Automated checks run (CI, tests, linting)
3. Feedback is provided
4. Changes are made based on feedback
5. Final approval and merge

**Merge Requirements:**
- All tests must pass
- Code must follow style guidelines
- Documentation must be updated
- At least one maintainer must approve
- No unresolved conversations

#### Community

You can chat with the community and developers in these places:

- [GitHub Discussions](https://github.com/yourusername/disksense64/discussions)
- [Discord Server](https://discord.gg/disksense64) (if available)
- [Reddit Community](https://reddit.com/r/disksense64) (if available)

We also have a monthly developer meeting. Check the [community calendar](https://calendar.google.com/calendar/embed?src=disksense64.developer.meetings@gmail.com) for details.

### Recognition

Contributions to DiskSense64 are recognized in several ways:

1. **Contributors List**: All contributors are listed in [CONTRIBUTORS.md](CONTRIBUTORS.md)
2. **GitHub Recognition**: Contributors receive credit in GitHub
3. **Release Notes**: Major contributions are highlighted in release notes
4. **Social Media**: Significant contributions may be featured on social media
5. **Swag Rewards**: Active contributors may receive project merchandise

Thank you for contributing to DiskSense64!