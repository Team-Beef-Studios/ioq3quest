@echo off

setlocal

set BUILD_TYPE=release
set VERSION=1.1.3

@REM Define the following environment variables to sign a release build
@REM set KEYSTORE=
@REM set KEYSTORE_PASS=


set ANDROID_SDK_ROOT=%AppData%\..\Local\Android\Sdk
set adb="%ANDROID_SDK_ROOT%\platform-tools\adb.exe"
set make="%ANDROID_SDK_ROOT%\ndk\21.1.6352462\prebuilt\windows-x86_64\bin\make.exe"
set apksigner="%ANDROID_SDK_ROOT%\build-tools\29.0.2\apksigner.bat"
set JAVA_HOME=C:\Program Files\Android\Android Studio\jre\jre

if "%1"=="clean" (
	rm -rf .\build
	rm -rf .\android\build
	rm -rf .\android\app\src\main\jniLibs\arm64-v8a
)

if %BUILD_TYPE%==release (
	set GRADLE_BUILD_TYPE=:app:assembleRelease
)
if %BUILD_TYPE%==debug (
	set GRADLE_BUILD_TYPE=:app:assembleDebug
)

REM package our special pk3
pushd android\app\src\main
call make_pakQ3Q.bat
popd

echo #define Q3QVERSION  "%VERSION%" > .\android\app\src\main\cpp\code\vr\vr_version.h

pushd %~dp0\..
%make% -j %NUMBER_OF_PROCESSORS% %BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
	popd
	echo "Failed to build ioq3"
	exit /b 1
)

pushd android

set GRADLE_EXIT_CONSOLE=1
call gradlew.bat %GRADLE_BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
	popd
	popd
	echo "Failed to build android project"
	exit /b 1
)

set PACKAGE_NAME=com.drbeef.ioq3quest
set ANDROID_STORAGE_LOCATION=/sdcard/ioquake3quest/
set APK_LOCATION=.\app\build\outputs\apk\%BUILD_TYPE%\ioq3quest-%BUILD_TYPE%-%VERSION%.apk

if %BUILD_TYPE%==release (
	echo "Signing Release APK"
	call %apksigner% sign --ks ../%KEYSTORE% --out %APK_LOCATION% --v2-signing-enabled true --ks-pass pass:%KEYSTORE_PASS% .\app\build\outputs\apk\%BUILD_TYPE%\app-%BUILD_TYPE%-unsigned.apk
)

if %BUILD_TYPE%==debug (
	echo "Copying Debug APK"
	copy .\app\build\outputs\apk\%BUILD_TYPE%\app-%BUILD_TYPE%.apk %APK_LOCATION%
)

if "%1"=="nodeploy" (
	goto :END
)

%adb% install -r %APK_LOCATION%
if %ERRORLEVEL% NEQ 0 (
	%adb% uninstall %PACKAGE_NAME%
	%adb% install %APK_LOCATION%
	if %ERRORLEVEL% NEQ 0 (
		popd
		popd
		echo "Failed to install apk."
		exit /b 1
	)
)

@REM %adb% shell mkdir -p %ANDROID_STORAGE_LOCATION%
@REM %adb% push --sync "D:\Program Files (x86)\Steam\steamapps\common\Quake 3 Arena\baseq3" %ANDROID_STORAGE_LOCATION%
@REM if %ERRORLEVEL% NEQ 0 (
@REM 	popd
@REM 	popd
@REM 	echo "Failed to transfer files."
@REM 	exit /b 1
@REM )

@REM %adb% push --sync ..\code\renderergl2\glsl %ANDROID_STORAGE_LOCATION%/baseq3/
@REM if %ERRORLEVEL% NEQ 0 (
@REM 	popd
@REM 	popd
@REM 	echo "Failed to transfer shaders."
@REM 	exit /b 1
@REM )

@REM %adb% push --sync autoexec.cfg %ANDROID_STORAGE_LOCATION%/baseq3/
@REM if %ERRORLEVEL% NEQ 0 (
@REM 	popd
@REM 	popd
@REM 	echo "Failed to transfer autoexec."
@REM 	exit /b 1
@REM )

%adb% logcat -c
%adb% shell am start -n %PACKAGE_NAME%/.MainActivity
if %ERRORLEVEL% NEQ 0 (
	popd
	popd
	echo "Failed to start application."
	exit 1
)
%adb% logcat *:S Quake3:V SDL:V DEBUG:V

:END
endlocal
