# MinServer 3
By steambird1, 2020-2022

## Older Version
[MinServer 2](https://github.com/steambird1/MinServer)
[MinServer 1](https://github.com/steambird1/MinServer-Old)

This is a newer version of MinServer.
However, here are some differences:

## Differences between MinServer 2
1. Here requires library file, for `$library.txt` default to specify DLL for **ALL** paths.
Here is `DEFAULT.dll` for default file visit (and HTML with `mspara3`), and `Services.dll` for file operation and user operations.
2. Currently, main program doesn't support operation command line. It only supports:

| Info | Description |
| :--: | :---------: |
| --port | Specify the port to listen. |
| --lib | Specify the library file. |
| --forbidden | Specify the forbidden-prompting file. `$forbids.htm` for default. |
| --not-found | Specify the not-found-prompting file. `$not_found.htm` for default. |
| --buffer-size | Specify the size of initial buffer. Make sure it is not too small. |

3. Here is no old-style service like `/file_operate`. To use it, specify your path with `service.dll`, with also have to be specified in `mslib.js` (`service_head`).
The default value is `/services`. Use `?method=auth_workspace` after that; `upload` is not supported because now we have a `file_operate` with `open` only and It'll read entire file or write to file.
4. Some names in `mslib` and `mspara` are also changed. Also, the new DLL entry is:
```c++
extern "C" __declspec(dllexport) void ServerMain(ssocket::acceptor &s, dlldata &d)
```
You can read the source code to learn more about `ssocket` and its `acceptor`.
Also, make sure you are using **Debug** mode of Visual Studio. For some reason, It'll crash if we use Release mode.

5. And the most important thing is, **This is an multi-thread server**!