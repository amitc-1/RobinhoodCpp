# RobinhoodCpp
<<<<<<< HEAD
A C++ framework for trading/autotrading using Robinhood (includes support for two-factor authentication required by Robinhood).
=======
A cross platform C++ framework for trading/autotrading using Robinhood (includes support for two-factor authentication required by Robinhood).
>>>>>>> change path to headers

## Prerequisites
This C++ framework depends on the following libraries
- nlohmann json https://github.com/nlohmann/json
- zlib https://zlib.net/ or https://github.com/madler/zlib
- curl https://curl.haxx.se or https://github.com/curl/curl

## Quick Start
We can use package manager vcpkg to install the above libraries.
1) Download & install vcpkg as given at https://github.com/Microsoft/vcpkg/. (If you already had vcpkg, make sure 
   you upgrade to the latest version as old version may give problems). 

2) Use vcpkg to install nlohmann-json  
   PS> .\vcpkg install nlohmann-json  
   Linux:~/$ ./vcpkg install nlohmann-json
   
3) Use vcpkg to install zlib  (Install zlib before you install curl)  
   PS> .\vcpkg install zlib  
   Linux:~/$ ./vcpkg install zlib 

4) Use vcpkg to install curl  
   PS> .\vcpkg install curl  
   Linux:~/$ ./vcpkg install curl 
   
5) Download RobinhoodCpp from github. 

6) Build RobinhoodCpp  
   Assuming you are in the directory which has CMakeLists.txt.  
   mkdir build; cd build  
   cmake .. "-DCMAKE_TOOLCHAIN_FILE=<PATH_TO_VCPKG>/vcpkg/scripts/buildsystems/vcpkg.cmake"  
   PS> msbuild RobinhoodCpp.sln. (This will create RobinhoodCpp.exe in build/Debug as can be seen in output logs on the screen)  
   Linux:~/$ make  
  
