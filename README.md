# fex
a fast, minimalistic File EXplorer for linux written in C.

# How to use
The software, when opened, will display the current folder by default, but an arbitrary path can be specified as well.
The special character ':' will allow the user to enter a series of commands to navigate or trigger a specific event, here is a comprehensive list of all of the currently implemented keywords:
- gg + Enter: selects the first element of the directory
- G + Enter: selects the last element of the directory
- [0-9]+ + Enter* (any number n): will select the nth element, if available. If the selected number is too big, it will be interpreted as G. *Note that if the number inserted is sufficiently large, the program will not need the user to press Enter, and will simply jump to that selection
- vim: will open vim in the current directory
- w: will display the logo and a brief copyright notice
- To quit from ':' mode, press the Escape button

To exit the program, simply type 'q'. To navigate through folders, you can either use the arrow keys or the 'hjkl' keys in vim style. To open a directory, simply press Enter after having selected it. If you open a text file, it will be opened with vim, anything else will be opened with your default application. Fex has a way to differentiate between file types:
- Directories: [file]
- Symlinks: {file}
- Block devices: ..file..
- Character special: \_file\_
- Default: file

If you have a suggestion regarding further differentiation or ideas, feel free to send a pull request or raise an issue.

# Why
I wanted to experiment with a custom software for a fast, reliable, no-nonsense navigation between folders.
The seamless integration with vim makes the experience especially satisfying for me, as I use vim quite often.
A lot of times I navigate folders with files that I'm not familiar with, so the integration with the `file` utility on top of the screen makes it trivial to find out the type of file we are currently looking at without further intervention by the user.

# How
The software is entirely written in C for extreme simplicity and minimalism, the experience is kept as barebones as possible intentionally.
The goal is to create a program that allows for a very quick and easy access of the file system. 
The code itself is (or at least should) be ANSI/ISO C compliant, except for the OS-dependent calls in the file system interaction. In the current implementation all calls are made with POSIX systems in mind. The goal is to make as easy as possible to make a "drop-in replacement" set of functions for other systems. I will make a serious attempt at keeping the code versatile for anybody to modify as they please.

Like I said, this software is inspired and is also integrated with vim, so navigation with cursor keys is also available with the classic "hjkl" keys. As I make progress in developement (which currently has no definitive plan), I will integrate more of the features provided by vim to enhance the experience without bloating the overall package.

# Copyright notice

This software is distributed under the GNU GPL 3.0 license. Refer to LICENSE or visit <http://www.gnu.org/licenses/> for more information.
You are more than welcome to fork, modify or distribute this software.
