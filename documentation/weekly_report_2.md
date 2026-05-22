Time used so far: approx. 6 hours (in total 9 hours)

What did I do this week?
- Produced base framework for QOI encoder
  - The decoder should be as easy to build
- Added testing framework
  - Applied coverage checking

How has the program progressed?
- Adding test harness took too long

What did I learn this week/today?
- That C++ was a mistake as a language, not just for this project, but in general.

What remains unclear or has been challenging?
- The TA for the course clarified that the DEFLATE implementation should be for PNG
  - Applying PNG as-is is going to take a while
- I'm not sure if the tests are going to be too good before next DL
  - I added some tests that test basic stuff
  - Most of the tests require me to implment both algorithms, which is the target for the next week

What will I do next?
- Add decoding for QOI
- Add decoding for PNG
  - This allows me to use PNGs as a source for raw/test data
- Add real tests
