# Adding Codecov to a Zephyr Project

This guide documents the steps to integrate [Codecov](https://about.codecov.io/) into a Zephyr RTOS project that uses Twister for unit tests and GitHub Actions for CI.

## Prerequisites

- Unit tests running on `native_sim` via Twister in CI
- GitHub repository with Actions enabled

---

## Step 1 — Sign up and add your repository to Codecov

1. Go to [codecov.io](https://about.codecov.io/) and sign in with your GitHub account.
2. Click **Add new repository** and select your repo.
3. Copy the **CODECOV_TOKEN** shown on the setup page.

---

## Step 2 — Add the token as a GitHub Actions secret

1. In your GitHub repository, go to **Settings → Secrets and variables → Actions**.
2. Click **New repository secret**.
3. Name: `CODECOV_TOKEN`, Value: paste the token from Step 1.

---

## Step 3 — Install gcovr in your CI workflow

Twister uses `gcovr` to collect coverage data. Add an install step before the test run in `.github/workflows/build.yml`:

```yaml
- name: Install gcovr
  run: pip install gcovr
```

---

## Step 4 — Enable coverage collection in Twister

Modify the **Run unit tests** step to add `--coverage` and `--coverage-tool gcovr`:

**Before:**
```yaml
- name: Run unit tests
  working-directory: app
  run: ../zephyr/scripts/twister -T tests -p native_sim
```

**After:**
```yaml
- name: Run unit tests
  working-directory: app
  run: ../zephyr/scripts/twister -T tests -p native_sim --coverage --coverage-tool gcovr
```

Twister will now produce a coverage JSON file at `twister-out/coverage.json`.

---

## Step 5 — Convert coverage to Cobertura XML

Codecov accepts Cobertura XML format. Add a conversion step after the test step:

```yaml
- name: Convert coverage to Cobertura XML
  working-directory: app
  run: |
    gcovr --add-tracefile twister-out/coverage.json \
          --cobertura twister-out/coverage.xml
```

---

## Step 6 — Upload coverage to Codecov

Add the upload step after the conversion:

```yaml
- name: Upload coverage to Codecov
  uses: codecov/codecov-action@v5
  with:
    token: ${{ secrets.CODECOV_TOKEN }}
    files: app/twister-out/coverage.xml
    fail_ci_if_error: false
```

> Note: the `files` path is relative to the workspace root, while previous steps used `working-directory: app`. Adjust the path if your checkout path differs.

---

## Complete modified workflow

```yaml
name: Build

on:
  push:

permissions:
  contents: read

jobs:
  build:
    name: Build app
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          path: app
          persist-credentials: false

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: 3.12

      - name: Setup Zephyr project
        uses: zephyrproject-rtos/action-zephyr-setup@v1
        with:
          app-path: app
          toolchains: arm-zephyr-eabi

      - name: Build firmware
        working-directory: app
        run: west build -b stm32h747i_disco/stm32h747xx/m7

      - name: Install gcovr
        run: pip install gcovr

      - name: Run unit tests
        working-directory: app
        run: ../zephyr/scripts/twister -T tests -p native_sim --coverage --coverage-tool gcovr

      - name: Convert coverage to Cobertura XML
        working-directory: app
        run: |
          gcovr --add-tracefile twister-out/coverage.json \
                --cobertura twister-out/coverage.xml

      - name: Upload coverage to Codecov
        uses: codecov/codecov-action@v5
        with:
          token: ${{ secrets.CODECOV_TOKEN }}
          files: app/twister-out/coverage.xml
          fail_ci_if_error: false
```

---

## Step 7 — Filter coverage to your own source files (optional)

By default, gcovr includes all compiled files — Zephyr kernel code, drivers, and third-party libraries. To limit coverage to only your own source directories, create a `gcovr.cfg` file in your app root (next to `west.yml`):

```
filter = ^src/
filter = ^lib/
```

Each `filter` is a regex matched against the **relative** path from `--coverage-basedir`. The `^` anchor ensures only paths that start with `src/` or `lib/` are included, preventing accidental matches against Zephyr's own `../zephyr/lib/` paths.

Add or adjust entries to match your project layout:
- `filter = ^src/` — covers everything under `app/src/`
- `filter = ^lib/` — covers everything under `app/lib/` and all its subdirectories

**Important notes on `gcovr.cfg` syntax:**
- Do **not** add an INI section header like `[gcovr]` — gcovr 8.x expects plain `key = value` pairs.
- gcovr automatically discovers the file when it is placed in the working directory or any parent directory.

---

## Step 8 — Add a Codecov badge to your README (optional)

After your first successful upload, Codecov generates a badge URL. Find it under **Settings → Badge** in the Codecov dashboard for your repo, then add it to your `README.md`:

```markdown
[![codecov](https://codecov.io/gh/<owner>/<repo>/branch/main/graph/badge.svg?token=<TOKEN>)](https://codecov.io/gh/<owner>/<repo>)
```

---

## Notes

- Coverage only works for tests running on `native_sim` (host-executable). Tests targeting real hardware are not instrumented.
- The `--coverage-tool gcovr` flag requires `gcovr >= 5.0`. The `pip install gcovr` step fetches the latest version.
- If you have multiple test suites or platforms, run Twister once per platform with `--coverage`, then merge the JSON files with `gcovr --add-tracefile a.json -a b.json --cobertura merged.xml` before uploading.
