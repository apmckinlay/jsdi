jsuneido-jsdi
=============

This is the C++ part of the DLL, COM, and APP interface which allows jSuneido to run the Suneido win32 gui.

Building
--------
Building with Visual Studio 2013 requires the November 2013 CTP Platform Toolset

You must set environment variables `JAVA_HOME_AMD64` and `JAVA_HOME_X86`
(or else you will get errors that jni.h is not found) For example:

```
JAVA_HOME_AMD64=C:\Program Files\Java\jdk1.8.0_20
JAVA_HOME_X86=C:\Program Files (x86)\Java\jdk1.8.0_20
```

You will probably need to restart Visual Studio after setting these
