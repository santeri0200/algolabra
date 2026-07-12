Coverage report
- [![codecov](https://codecov.io/gh/santeri0200/algolabra/graph/badge.svg?token=D3VWSBDKM7)](https://codecov.io/gh/santeri0200/algolabra)
- I could not make this only include one function/method tests, so it includes coverage for all of the tests

What was tested and how?
- QOI inputs produce expected pixel arrays
  - Running `decode` functionality of the QOI file
  - The resulting pixel arrays for each QOI_OP codes were compared to expected results
- Pixel arrays produce expected QOI_OPs
  - Running `encode` functionality of the QOI file
  - The resulting data arrays for each QOI_OP codes were compared to expected results
- QOI and PNG implementations can encode BMP files to them selves and decoded back to the original pixel arrays
  - The BMP file decoded to pixel data
  - The pixel data is encoded to both QOI and PNG
  - Both encoding results and decoded and compared to the original pixel data
- Basic entry functionality works for intended commands
  - Functions are ran and tested for non-zero return values
