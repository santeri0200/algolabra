Time used so far: approx. 12 hours (in total 37 hours)

What did I do this week?
- Made BMP implementation with BITMAPINFOHEADER support for 1bpp and 24bpp
  - This was because the software I used to make the BMP exported my 1bpp test image as such
  - The non-palette images (without alpha channel) are set to 24bpp

How has the program progressed?
- The "raw data" can now be read from a file ... give that it uses specific BMP format
- Data-to-_encoding_ pipeline is now set up
  - You are able to decode BMP and encode it to either QOI or PNG

What did I learn this week/today?
- BMP in all of it's simplicity is a great way to losslessly pack relatively small images

What remains unclear or has been challenging?
- How do I implement _real_ filters
  - I probably won't
- Do I have time for a GUI or is it just a terminal app with few print lines?

What will I do next?
- Fix test
- Rest of the entry point
  - [x] Command chaining to have the demo available before the course ends
  - [ ] Comparison feature

