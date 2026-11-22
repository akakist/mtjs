# Telnet

Telnet is a built-in telnet server that allows easy process management by defining various console commands.

The functionality is simplified to the maximum - you need to specify the port to listen on and register commands.

When registering a command, you can specify the directory in which the command will be executed.

## Commands

`telnet.listen(addr, deviceName);` - addr in the format "ip:port", deviceName - will be displayed in the command line prompt.

`register_command(directory, regexp, help);` - directory - the directory in which the command is executed. regexp - the command itself, help - the string that will be displayed in the telnet help.

Example:
`telnet.register_command("t1","^([0-9\\.]+)([\\+-\\*\\/])([0-9\\.]+)$","calculator");`

The regexp syntax follows standard C++ regex. It does not support things like \d.

There is an example in src, `t_telnet.ts`, which implements a calculator.

```
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
prod1:$
```

Enter `ls`:

```
prod1:$ ls
Commands in '/'

help - display this page
quit - quit console
.. - go to parent directory
t1 - Directory
prod1:$
```

Enter `t1` - the directory name is entered without `cd`.

```
prod1:$ t1
prod1:/t1$
```

Next, enter arithmetic operations:

```
prod1:/t1$ 5*5
result: 25
```

The calculator example is somewhat excessive but demonstrates the full power of telnet.


