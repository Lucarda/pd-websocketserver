#!/bin/sh

# This script is for use with foo~.dll as argument

# On msys2 and install the ntldd package:

#	pacman -S mingw32/mingw-w64-i686-ntldd-git

#	pacman -S mingw64/mingw-w64-x86_64-ntldd-git

arch="none"

error() {
   echo "$@" 1>&2
}

NTLDD=$(which ntldd 2>/dev/null)

if [ "x${NTLDD}" = "x" ]; then
  error "no 'ntldd' binary found"
  exit 0
fi

arch() {
  if file "$1" | grep -w "PE32+" >/dev/null; then
    arch="w64"
    return
  fi
  if file "$1" | grep -w "PE32" >/dev/null; then
    arch="w32"
    return
  fi
}

arch "$1"


./localdeps.win.sh "$1"

# run again the script on each dep.

for filename in *."${arch}"; do
	./localdeps.win.sh "$filename"
done

# Change extension of $1.

FILE="$1"
FILEFOO="${FILE%%.*}"

if [ "${arch}" = "w64" ]; then
    mv "$1" "${FILEFOO}.m_amd64"
  fi
  
if [ "${arch}" = "w32" ]; then
    mv "$1" "${FILEFOO}.m_i386"
  fi


# Remove scripts from install dir.
rm ./localdeps.win.sh
rm ./windep.sh