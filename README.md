# Algolabra

## Documentation
[Implementation documentation](documentation/implementation.md)

### Setup
- Clone the repo with `git clone --recurse-submodules`
  - If you manage to clone the repo without the submodules and want to run test, run `git submodule update --init --recursive`
- Install test vendor and run tests
  - `cd vendor/googletest && cmake -S . -B build && cmake --build build`
  - `make test`
- Run the project
  - `sh compile.sh`

### Weekly reports
[Week 1](documentation/weekly_report_1.md)
