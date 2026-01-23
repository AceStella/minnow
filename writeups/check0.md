Checkpoint 0 Writeup
====================

My name: Ace

My SUNet ID: None

I collaborated with: Gemini

I would like to credit/thank these classmates for their help: Gemini

This lab took me about 12 hours to do. I did not attend the lab session.

My secret code from section 2.1 was: 540368

I was surprised by or edified to learn that: I can use deque of string to save data.

Describe ByteStream implementation. I use deque of string to save data.
If just use deque of char, its easy to implement, but may only peek one bit frequently. Use deque of string can peek the remain part of string one time.
At first I used is_closed() function in Reader, but got error when compile. And I fogot to add deque head file.
As for asymptotic performance, push O(1), pop O(1), peek O(1), space O(N).
ByteStream throughput (pop length 4096):  3.97 Gbit/s
  ByteStream throughput (pop length 128):   2.92 Gbit/s
  ByteStream throughput (pop length 32):    2.18 Gbit/s
Implementation Time: about 5h, and Difficulty, mid.
Other measurement: safety, use string view to make sure read only.

Implementation Challenges:
The pop and push logic with deque of string.

Remaining Bugs:
None for now.

- If applicable: I received help from a former student in this class,
  another expert, or a chatbot or other AI system (e.g. ChatGPT,
  Gemini, Claude, etc.), with the following questions or prompts:
  [please list questions/prompts]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I contributed a new test case that catches a plausible bug
  not otherwise caught: [provide Pull Request URL]

