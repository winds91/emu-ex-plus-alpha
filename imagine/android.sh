#!/bin/bash
# Convenience script for Android that also builds & installs the AAR from the base module

set -e
scriptDir="$(dirname "$(readlink -f "$0")")"
cd "${scriptDir}"

source tools/androidFuncs.sh

parseCMakeActionArgs "$@"
executeCMakeActions

imagineV9SrcPath="${scriptDir}/src/base/android/imagine-v9"
imagineV9BuildLibBasePath="${imagineV9SrcPath}/build/outputs/aar"
sdkPath=${IMAGINE_SDK_PATH:="${HOME}/imagine-sdk"}
prefix="${sdkPath}/android-java"
imagineV9InstallLibPath="${prefix}/imagine-v9.aar"

case $action in
	build)
		bash "${imagineV9SrcPath}/gradlew" --project-dir "${imagineV9SrcPath}" assembleRelease
	;;
	install)
		mkdir -p "${prefix}"
		echo "Installing aar to ${prefix}"
		cp "${imagineV9BuildLibBasePath}"/imagine-v9*.aar ${imagineV9InstallLibPath}
	;;
	installLinks)
		mkdir -p "${prefix}"
		echo "Installing symlink aar to ${prefix}"
		ln -srf "${imagineV9BuildLibBasePath}"/imagine-v9*.aar ${imagineV9InstallLibPath}
	;;
esac
