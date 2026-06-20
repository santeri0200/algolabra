# Algolabra
[![codecov](https://codecov.io/gh/santeri0200/algolabra/graph/badge.svg?token=D3VWSBDKM7)](https://codecov.io/gh/santeri0200/algolabra)

## Documentation
[Implementation documentation](documentation/implementation.md)
[Testing documentation](documentation/testing.md)

### Setup
- Clone the repo with `git clone --recurse-submodules`
  - If you manage to clone the repo without the submodules and want to run test, run `git submodule update --init --recursive`
- Install test vendor and run tests
  - `cd vendor/googletest && cmake -S . -B build && cmake --build build`
  - `make test`
- Run the project
  - `sh compile.sh`

### Running the project
- Run the comparison feature `out/main %target% compare input.bmp`
  - Where `%target%` is either `qoi`, `png` or `both`
- Encode BMP images to either PNG or QOI `out/main %target% encode input output`
  - Where `%target%` is either `qoi` or `png`
  - This feature allows inspection of the outputs

### Weekly reports
[Week 1](documentation/weekly_report_1.md)
[Week 2](documentation/weekly_report_2.md)
[Week 3](documentation/weekly_report_3.md)
[Week 4](documentation/weekly_report_4.md)
[Week 5](documentation/weekly_report_5.md)
