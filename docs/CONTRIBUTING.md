# Contributing to DiskSense64

First off, thank you for considering contributing to DiskSense64! It's people like you that make DiskSense64 such a great tool.

## Code of Conduct

This project and everyone participating in it is governed by the [DiskSense64 Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to [project-email@example.com].

## How Can I Contribute?

### Reporting Bugs

This section guides you through submitting a bug report for DiskSense64. Following these guidelines helps maintainers and the community understand your report, reproduce the behavior, and find related reports.

Before creating bug reports, please check [this list](#before-submitting-a-bug-report) as you might find out that you don't need to create one. When you are creating a bug report, please [include as many details as possible](#how-do-i-submit-a-good-bug-report). Fill out [the required template](https://github.com/yourusername/disksense64/blob/master/.github/ISSUE_TEMPLATE/bug_report.md), the information it asks for helps us resolve issues faster.

> **Note:** If you find a **Closed** issue that seems like it is the same thing that you're experiencing, open a new issue and include a link to the original issue in the body of your new one.

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for DiskSense64, including completely new features and minor improvements to existing functionality. Following these guidelines helps maintainers and the community understand your suggestion and find related suggestions.

Before creating enhancement suggestions, please check [this list](#before-submitting-an-enhancement-suggestion) as you might find out that you don't need to create one. When you are creating an enhancement suggestion, please [include as many details as possible](#how-do-i-submit-a-good-enhancement-suggestion). Fill in [the template](https://github.com/yourusername/disksense64/blob/master/.github/ISSUE_TEMPLATE/feature_request.md), including the steps that you imagine you would take if the feature you're requesting existed.

### Your First Code Contribution

Unsure where to begin contributing to DiskSense64? You can start by looking through these `beginner` and `help-wanted` issues:

* [Beginner issues][beginner] - issues which should only require a few lines of code, and a test or two.
* [Help wanted issues][help-wanted] - issues which should be a bit more involved than `beginner` issues.

Both issue lists are sorted by total number of comments. While not perfect, number of comments is a reasonable proxy for impact a given change will have.

### Pull Requests

The process described here has several goals:

- Maintain DiskSense64's quality
- Fix problems that are important to users
- Engage the community in working toward the best possible DiskSense64
- Enable a sustainable system for DiskSense64's maintainers to review contributions

Please follow these steps to have your contribution considered by the maintainers:

1. Follow all instructions in [the template](PULL_REQUEST_TEMPLATE.md)
2. Follow the [styleguides](#styleguides)
3. After you submit your pull request, verify that all [status checks](https://help.github.com/articles/about-status-checks/) are passing

While the prerequisites above must be satisfied prior to having your pull request reviewed, the reviewer(s) may ask you to complete additional design work, tests, or other changes before your pull request can be ultimately accepted.

## Styleguides

### Git Commit Messages

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally after the first line
* When only changing documentation, include `[ci skip]` in the commit title

### C++ Styleguide

All C++ code must adhere to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with the following exceptions:

* Line length: 100 characters instead of 80
* Naming: Use snake_case for variables and functions, PascalCase for classes and structs

### Documentation Styleguide

* Use [Markdown](https://daringfireball.net/projects/markdown) for documentation
* Reference functions and classes using backticks: `function_name()`
* Follow the existing documentation style in the codebase

## Additional Notes

### Issue and Pull Request Labels

This section lists the labels we use to help us track and manage issues and pull requests.

[GitHub search](https://help.github.com/articles/searching-issues/) makes it easy to use labels for finding groups of issues or pull requests you're interested in.

The labels are loosely grouped by their purpose, but it's not required that every issue have a label from every group or that an issue can't have more than one label from the same group.

#### Type of Issue and Issue State

* `bug` - Issues that are bugs
* `enhancement` - Issues that are feature requests
* `documentation` - Issues that are documentation related
* `performance` - Issues related to performance
* `security` - Issues related to security
* `beginner` - Issues that are good for beginners
* `help-wanted` - Issues that need assistance
* `duplicate` - Issues that are duplicates of other issues
* `wontfix` - Issues that won't be fixed
* `invalid` - Issues that are invalid
* `question` - Issues that are questions

#### Pull Request Labels

* `work-in-progress` - Pull requests that are not ready for review
* `needs-review` - Pull requests that need review
* `ready-to-merge` - Pull requests that are ready to merge

## Development Setup

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/disksense64.git`
3. Create a feature branch: `git checkout -b feature/AmazingFeature`
4. Make your changes
5. Commit your changes: `git commit -m 'Add some AmazingFeature'`
6. Push to the branch: `git push origin feature/AmazingFeature`
7. Open a pull request

## Testing

Before submitting a pull request, please make sure your changes pass all tests:

1. Run unit tests: `./build_and_test.sh test`
2. Run performance benchmarks: `./build_and_test.sh bench`
3. Verify build on all platforms: `./build_and_test.sh all`

## Code Review Process

All submissions require review. We use GitHub pull requests for this purpose. Consult [GitHub Help](https://help.github.com/articles/about-pull-requests/) for more information on using pull requests.

The review process:
1. At least one maintainer must approve the PR
2. All CI checks must pass
3. Code must follow style guidelines
4. Tests must pass
5. Documentation must be updated if needed

## Community

You can chat with the community and developers in these places:

- [GitHub Discussions](https://github.com/yourusername/disksense64/discussions)
- [Discord Server](https://discord.gg/disksense64) (if available)
- [Reddit Community](https://reddit.com/r/disksense64) (if available)

We also have a monthly developer meeting. Check the [community calendar](https://calendar.google.com/calendar/embed?src=disksense64.developer.meetings@gmail.com) for details.