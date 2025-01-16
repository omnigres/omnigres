Welcome to **Omnigres**! This document provides a quick overview on how to prepare your contributions.

---

## Commit Message Guidelines

We follow a “Problem: / Solution:” format, inspired
by [Solving problems one commit at a time](https://yrashk.com/blog/2017/09/04/solving-problems-one-commit-at-a-time/).
Each commit message looks like this:

```
Problem: short problem description

[Optional extra lines for context]

Solution: short solution description

[Optional extra lines for rationale and alternatives considered]
```

### Example

```
Problem: worker crashes on unexpected input

This started happening after the new data loader in v0.2.0.

Solution: validate input before init

We considered ignoring malformed inputs, but failing fast is safer.
```

> [!TIP]
> **Problem** Think of answering the W-questions? What is the problem?
> What were the expectations? Why is this a problem?
> Who is affected? When is this going to be important? and so on.

> [!TIP]
> **Solution** Consider _paths not taken_: mention other approaches you considered. This helps future maintainers
> understand why you chose your solution and prevent duplicate dead-end work.

---

### Housekeeping

1. **Version Bump**: In `versions.txt`, increment the version using [Semantic Versioning](https://semver.org/).
2. **Changelog Entry**: Add an appropriate section to CHANGELOG.md,
   referencing the PR number (specify it when you create the PR)

---

## Contribution Workflow

1. **Where To Start?**
   - **Important/urgent** changes: branch off `master`
   - Next-release scheduling: branch off `next/<ext_name>`

1. **Changes & Commits**
   - Keep changes small and focused
   - Format commit messages with `Problem:` followed by `Solution:` (see above)

3. **Open PR**
   - Target `master` for urgent fixes.
   - Target `next/<ext_name>` for longer release cycles.
   - If work is not reasonably complete or otherwise tentative in nature,
     make it a Draft PR.

4. **Review & Merge**
   - Maintainers review the PR and provide feedback
   - Please provide timely responses when appropriate
   - Prefer to add new commits as opposed to force-pushes (_unless well motivated to do so_).
     This will help the reviewers to track progress.
   - Once approved, the maintainers will merge the PR.

## Version and Changelog Updates

### When to Update

1. **Urgent fixes or important updates for other upcoming contributions**
   - Bump the extension version in `versions.txt`.
   - Update the `CHANGELOG.md`.
   - Commit these changes alongside your code in a PR to `master`.

2. **Longer release cycles**
   - Use a branch named `next/<ext_name>` (e.g., `next/omni_myext`).
   - Reach out to maintainers if such a branch doesn’t exist.
   - Changes go into `next/<ext_name>` until maintainers open a PR from that branch into `master`.

---

## Summary of Best Practices

- **Keep commits atomic**: One logical fix or feature per commit.
- **Use “Problem:” and “Solution:”**: This clarifies the purpose of each commit.
- **Preserve “paths not taken”**: Documenting alternatives avoids re-inventing old ideas.
- **Always update versions and changelogs**: Reflect your changes accurately.
- **Specify your PR number** in `CHANGELOG.md`.
- **Release Cycles**: Maintainers open a PR from `next/<ext_name>` into `master` to prepare release cycles.

By following these guidelines, you help keep Omnigres maintainable, well-documented, and easy to collaborate on.

**Thanks for contributing and happy hacking!**