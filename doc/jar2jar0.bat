@echo off
rem
rem usage: jar2jar0 archiv.jar
rem
rem jar2jar0 - convert a compressed jar archive into uncompressed
rem           so that it can be used by tools like xref.
rem
rem (c) 2001 by Xref-Tech, Bratislava
rem

set J2J0_TMP_DIR_NAME=%TEMP%\jar2jar0_tmp_directory

if .%OS% == .Windows_NT goto :ntwin
deltree /Y %J2J0_TMP_DIR_NAME%
goto :jend
:ntwin
rd /s /q %J2J0_TMP_DIR_NAME%
:jend

mkdir %J2J0_TMP_DIR_NAME%
cd %J2J0_TMP_DIR_NAME%

echo "Extracting %1 to %J2J0_TMP_DIR_NAME%"
jar -xf %1

echo "Updating archive"
jar -uf0 %1 -C %J2J0_TMP_DIR_NAME% .

echo "Cleaning"

if .%OS% == .Windows_NT goto :ntwin2
deltree /Y %J2J0_TMP_DIR_NAME%
goto :jend2
:ntwin2
rd /s /q %J2J0_TMP_DIR_NAME%
:jend2

cd %TEMP%
echo "ALL DONE."
