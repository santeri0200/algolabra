## Implamentation docs

The general structure of the program
- The project is ran under one entry function
  - The entry function can be accessed through main or test main, allowing a bit of flexibility in development
- The entry point has option parser, mainly for mode and input file
- The modes are:
  - decode
  - encode
  Both of which support QOI and PNG. For ease of use, the input file can be a BMP as decoding support for it has been added.
- Decoding flag runs the file through decoding step
  - This mode esentially only includes a check for _compatibility_
- Encoding flag runs the file through appropriate decoding function and then encodes the resulting pixel array in both QOI and PNG
  - The results for size are included in standard out
  - With an optional _out_ flag, the results can be added to an existing folder


Achieved time and space complexities (e.g., Big-O analysis based on pseudocode)
- Both seem to be O(n) for both algorithms. Again with some constant. DEFLATE also has additional multilpiers, but they reduce down to O(n) anyways...

Performance and Big-O complexity comparison (if relevant to the project)
- Performance differnce under `dice.bmp` is with-in the margin of error (I'd say). This is probably because of the subpar C++ code and some optional PNG features. These features WILL make PNG slower but the achieved compression ratio would probably be a bit closer to QOI. 
- As the original proposal was to compare QOI and DEFLATE, the PNG not being fully implemented should have been the end goal from the start.

Possible shortcomings and suggestions for improvement
- PNG is imlemented barebones in the 1.0 spec. This is not the newest. DEFLATE algorithm as described by our TA is very time consuming, so the results are once again skewed by the limitations in the implementation wrapping our underlying algorithm.

Use of large language models (ChatGPT, etc.). Mention which model you used and how. If you did not use any, explicitly state that. This is important!
- Tried using CurreChat with it's various models, but the platform did not respond most queries
- Ended up using ChatGPT 5.5 a caveman prompt for PNG documentation, so that it would only produce smaller outputs hopefully only including what was stated in it...

List of the sources you have used, only those relevant to your work.
- https://qoiformat.org/qoi-specification.pdf
- https://en.wikipedia.org/wiki/Deflate
- https://www.rfc-editor.org/rfc/rfc1951
- https://libpng.org/pub/png/spec/1.0/
