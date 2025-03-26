# fex
a fast, minimalistic File EXplorer for linux written in C.

# Why
I wanted to experiment with a custom software for a fast, reliable, no-nonsense navigation between folders.
The seamless integration with vim makes the experience especially satisfying for me, as I use vim quite often.
A lot of times I navigate folders with files that I'm not familiar with, so the integration with the `file` utility on top of the screen makes it trivial to find out the type of file we are currently looking at without further intervention by the user.

# How
The software is entirely written in C for extreme simplicity and minimalism, the experience is kept as barebones as possible intentionally.
The goal is to create a program that allows for a very quick and easy access of the file system. 
The code itself is (or at least should) be ANSI/ISO C compliant, except for the OS-dependent calls in the file system interaction. In the current implementation all calls are made with POSIX systems in mind. The goal is to make as easy as possible to make a "drop-in replacement" set of functions for other systems. I will make a serious attempt at keeping the code versatile for anybody to modify as they please.

Like I said, this software is inspired and is also integrated with vim, so navigation with cursor keys is also available with the classic "hjkl" keys. As I make progress in developement (which currently has no definitive plan), I will integrate more of the features provided by vim to enhance the experience without bloating the overall package.
