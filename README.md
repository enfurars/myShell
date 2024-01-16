# myShell
Detailed project descrtiption can be found [here](https://github.com/enfurars/myShell/blob/main/Project-Description.pdf).

## Manuals to Run myShell in Linux
myShell can be started by running:
- make
- ./myshell

## Implementation Details
- The executables in the path (including aliases) are executed in fork exec manner and exit, bello, redirections and backgrounding are executed via functions. 
- Commands can have arguments.
- Exit command is implemented together with checking for background processes. If any, exit prevented. 
- Background processing (&), and redirection operators (">" and "> >") exist and achieve the same task as in bash or zsh. 
- Users are able to create alias by entering the command this way:
  - alias alias\_name = "command\_name command\_args"
  - Aliases are saved and valid even after a restart of the shell.
- A new re-redirection operator ("> > >") (no spaces) exists. This operator acts exactly as ("> >"), but the order of all letters in the output will be invested. 
- The "bello" command is displaying eight information items about the user, respectively:
  1. Username
  2. 2. Hostname 
  3. Last Executed Command : last input from user is shown
  4. TTY
  5. Current Shell Name (not myshell)
  6. Home Location
  7. Current Time and Date 
  8. Current number of proccesses being executed: processes under myshell are count. Bello, ps and myshell is included.
