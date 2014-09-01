fbx-conv
========

Command line utility using the FBX SDK to convert FBX/Collada/Obj files
to c3t or c3b format.

Precompiled Binaries
====================
You can download the precompiled binaries from https://github.com/cocos2d/cocos2d-x-3rd-party-libs-bin/.

Building
========
You'll need an installation of the FBX SDK 2014. Once installed, set the
FBX_SDK_ROOT to the directory where you installed the FBX SDK. Then run the project which you want at "build/xxxx/"
. These we generate Visual Studio and XCode project.

Example
========
Export both .c3b and .c3t file command line:
xx/xx/fbx-conv(full path) -a xxx.fbx

Export .c3b command line:
xx/xx/fbx-conv(full path) -b xxx.fbx

Export .c3t command line:
xx/xx/fbx-conv(full path) -t xxx.fbx

