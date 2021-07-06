$ErrorActionPreference = "Stop"

$VK_SDK="C:\VulkanSDK\1.2.*"
$GLFW_URL="https://github.com/glfw/glfw/releases/download/3.3.2/glfw-3.3.2.zip"
$FONT_URL="https://github.com/acgaudette/kufont-ascii.git"
$HEAD_URL="https://github.com/acgaudette/acg.git"
 $ALG_URL="https://github.com/acgaudette/alg.git"

mkdir -Force assets
cd assets

if (-Not (Test-Path "kufont*")) {
	git clone $FONT_URL
}

if (-Not (Test-Path "font.pbm")) {
	New-Item -Path font.pbm -ItemType SymbolicLink -Value kufont*/*.pbm
}

cd ..

mkdir -Force ext/src
mkdir -Force ext/lib
mkdir -Force ext/include
cd ext

cd include

if (-Not (Test-Path "acg")) {
	git clone $HEAD_URL
}

if (-Not (Test-Path "alg")) {
	git clone $ALG_URL
}

cd ..

if (-Not (Test-Path "lib/vulkan.lib")) {
	New-Item -Path "lib/vulkan.lib" -ItemType SymbolicLink -Value "$VK_SDK/Lib/vulkan-1.lib"
}

if (-Not (Test-Path "include/vulkan")) {
	New-Item -Path "include/vulkan" -ItemType SymbolicLink -Value "$VK_SDK/Include/vulkan"
}

cd src

if (-Not (Test-Path "glfw*.zip")) {
	Invoke-WebRequest $GLFW_URL -OutFile glfw.zip
}

if (-Not (Test-Path -PathType Container "glfw-*")) {
	Expand-Archive glfw.zip .
}

cd glfw*
cmake                           `
    -DBUILD_SHARED_LIBS=OFF .   `
    -DGLFW_VULKAN_STATIC=OFF .  `
    -DGLFW_BUILD_EXAMPLES=OFF . `
    -DGLFW_BUILD_DOCS=OFF .     `
    -DCMAKE_INSTALL_PREFIX="../.."

MSBUILD.exe ALL_BUILD.vcxproj
MSBUILD.exe INSTALL.vcxproj

cd ../../..
