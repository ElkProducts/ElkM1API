# ElkM1API - A simple API to wrap the ASCII protocol

ElkM1API is a wrapper for the many functions that the Elk ASCII protocol (and eventually the Elk Refresh Protocol) that allows it to request information in a familiar environment, with everything wrapped down to simple calls. 

It's designed to be accesable using SWIG, which allows it to be deployed to anywhere that supports C++11 compiled libraries and a target language. (Java with JNI, C#, Android with NDK, iPhone).

## Setup

To use this project, you will (currently) need SWIG for Windows to be installed to C:/Data/swigwin, and Microsoft Visual Studio 2013. 

## Usage

When you click "Rebuild" on the ElkM1API project, it deletes the SWIG generated wrappers and regenerates them, as well as the swig_wrap_csharp.cpp file (as it's only currently set up to wrap to C#). Delete the 'csharp' directory in one of the example projects and alt-click the generated 'csharp' folder to link it in and experiment with it. Altering the Properties>Pre-Build Steps should make generating it for Java or other windows platforms quite simple. As it's all STL C++11, pulling the C++ files into a Makefile and generating for Android or Linux should 'just work', but please open an issue if you encounter any issues compiling for those platforms. (CMake would be a more ideal make system, but for this case we stuck to more familiar tools)

## Structure

The API takes whatever command you give it and generates an ASCII packet, passing it to the underlying connection you and it. These connections are bi-directional per language, so it's possible (and quite easy) to implement the transport in the native language, rather than relying on a hopefully decent socket implementation in C++. (an example of this is in ElkM1DesktopApp/M1ConnectionWrappers.cs) 

For the most part, everything is kept synchronous by blocking for each call and synchronizing when the 'Monitor' thread returns. For some calls, such as getArmStatus(), there is also a callback available to let you react whenever you recieve a packet of that kind, which happens whenever the arming status of the system changes (regardless of if you asked for it or not). The list of functions which have callback equivelents is not currently exhaustive, so if you need more feel free to add them. (should take a couple lines of code max, but it wasn't certain from the outset which calls would be useful as callbacks)

There also exists some smart convienience functions, such as getConfiguredZones/Keypads/Areas (which try to deduce which of the possible devices are actually in use, and returns an array of indexes) and collectNames (which uses the above functions to asynchronously, but deterministically, collect the names to work around the slow ASCII protocol name retrieval method, and then fills undefined names with ""). You are suggested to use these as much as you can, as the Refresh protocol will probably rely quite heavily on them to determine which are actually defined.
