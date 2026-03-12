Checkpoint 1 Writeup
====================

My name: Ace

My SUNet ID: None

I collaborated with: Gemini

I would like to thank/reward these classmates for their help: Gemini

This lab took me about 8 hours to do. I did not attend the lab session.

I was surprised by or edified to learn that: 
The test even consider the integer overflow at near 2^64.

Report from the hands-on component of the lab checkpoint: [include
information from 2.1(4), and report on your experience in 2.2]

Describe Reassembler structure and design. 
I use map of first index as key and string as value to save data in capacity.
By using lower_bound(), I can find the key equals or greater than the first index in O(log N) time.
If I just go through the keys, it may take O(N) time to find the key.

Implementation Challenges:
1.Handling empty/EOF.
2.Integer overflow.
3.Clang-Tidy.

Remaining Bugs:
None for now with 18 tests.

- If applicable: I received help from a former student in this class,
  another expert, or a chatbot or other AI system Gemini, with the following questions or prompts:
  - "How to efficiently merge overlapping intervals using std::map and lower_bound in C++?"
  - "How to fix integer overflow issues when calculating substring bounds with gigantic indices?"
  - "How to resolve clang-tidy warnings regarding performance-inefficient-string-concatenation and readability-function-cognitive-complexity?"

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]
