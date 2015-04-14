fbx-conv
========
fbx-conv can convert some middle format (such as fbx, blend, obj, etc) to .c3b or .c3t which are used by cocos2d-x.

Precompiled Binaries
====================
You can download the precompiled binaries from https://github.com/cocos2d/cocos2d-x-3rd-party-libs-bin/.

Building
========
You'll need an installation of the FBX SDK 2014. Once installed, set the
FBX_SDK_ROOT to the directory where you installed the FBX SDK. Then run the project which you want at "build/xxxx/"
. These we generate Visual Studio and XCode project.

Command-line USAGE:
========

*   Windows - `fbx-conv.exe [options] <input>`
*   Mac - `fbx-conv [options] <input>`

###Options/flags
*   **`-?`**				-Display help information.
*   **`-f`**				-Flip the V texture coordinates.
*   **`-m <size>`**			-The maximum amount of vertices or indices a mesh may contain (default: 32k)
*   **`-n <size>`**			-The maximum amount of bones a nodepart can contain (default: 40)
*   **`-v`**       			-Verbose: print additional progress information
*   **`-g`**       			-Whether you need to merge the same mesh which have the same vertex attribute
*   **`-a`**       			-Export c3b(binary) and c3t(text)
*   **`-b`**       			-Export c3b(binary)
*   **`-t`**       			-Export c3t(text)
*   **`-c <size>`**			-The compression level: 0 , 1 (default: 0)
*   **`-l`**       			-Export model data only.
*   **`-j`**       			-Export animation data only.

###Example
`fbx-conv -a boss.FBX`

Note:
========
1. Model need to have a material.
2. contain only one skeleton.

Thanks
========
Special thanks to libgdx fbx-conv, We used their code and do some change.


