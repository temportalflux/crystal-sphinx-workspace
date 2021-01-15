#!/bin/bash

PlatformArchitecture=$1
msbuild="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe"
binaries="$PWD/Binaries/Build/Debug/x$PlatformArchitecture/Assimp"

cd "Core/TemportalEngine/libs/assimp"

rm -rf CMakeCache.txt
cmake -G "Visual Studio 15 2017 Win64"
"$msbuild" "Assimp.sln"

mkdir -p "$binaries"
cp "lib/Debug/assimp-vc141-mtd.lib" "$binaries/"
cp "bin/Debug/assimp-vc141-mtd.pdb" "$binaries/"
cp "bin/Debug/assimp-vc141-mtd.dll" "$binaries/"