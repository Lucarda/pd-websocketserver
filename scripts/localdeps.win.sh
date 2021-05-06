#!/bin/sh

## puts dependencies besides the binary

## usage: $0 <binary> [<binary2>...]


###########################################
# WARNING
#
# this uses an ugly hack to allow side-by-side installation of 32bit and 64bit
# dependencies:
# embedded dependencies are renamed from "libfoo.dll" to "libfoo.w32" resp.
# "libfoo.w64", and the files are modified (using 'sed') to reflext this
# renaming.
# this is somewhat brittle and likely to break!

error() {
   echo "$@" 1>&2
}

NTLDD=$(which ntldd 2>/dev/null)

if [ "x${NTLDD}" = "x" ]; then
  error "no 'ntldd' binary found"
  exit 0
fi


list_deps() {
  ${NTLDD} -R "$1" \
	| grep -i mingw \
	| awk '{print $3}' \
	| grep -i mingw \
	| sed -e 's|\\|/|g'
}

file2arch() {
  if file "$1" | grep -w "PE32+" >/dev/null; then
    echo "w64"
    return
  fi
  if file "$1" | grep -w "PE32" >/dev/null; then
    echo "w32"
    return
  fi
}

install_deps () {
local outdir
local idepfile
local odepfile
local archext
local dep
error "DEP: ${INSTALLDEPS_INDENT}'$1' '$2'"

outdir=$2
if [ "x${outdir}" = "x" ]; then
  outdir=${1%/*}
fi
if [ ! -d "${outdir}" ]; then
  outdir=.
fi

list_deps "$1" | while read dep; do
  idepfile=$(basename "${dep}")
  odepfile=${idepfile}
  archext=$(file2arch "${dep}")
  if [ "x${archext}" != "x" ]; then
    odepfile=$(echo ${idepfile} | sed -e "s|\.dll|.${archext}|")
  fi
  if [ "x${idepfile}" = "x${odepfile}" ]; then
	archext=""
  fi
  if [ -e "${outdir}/${odepfile}" ]; then
    error "DEP: ${INSTALLDEPS_INDENT}  ${dep} SKIPPED"
  else
    error "DEP: ${INSTALLDEPS_INDENT}  ${dep} -> ${outdir}/${odepfile}"
    cp "${dep}" "${outdir}/${odepfile}"
    chmod a-x "${outdir}/${depfile}"
  fi

  if [ "x${archext}" != "x" ]; then
    sed -b -e "s|${idepfile}|${odepfile}|g" -i "${outdir}/${odepfile}" "${outdir}"/*."${archext}" "$1"
  fi
done
}


for f in "$@"; do
   if [ -e "${f}" ]; then
       install_deps "${f}"
   fi
done
