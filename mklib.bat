cd bin
del exports*
del *def
del *lib
del *exp

dumpbin /exports libtxtquad.dll > exports.txt
echo LIBRARY LIBTXTQUAD > libtxtquad.def
echo EXPORTS >> libtxtquad.def
for /f "skip=19 tokens=4" %%A in (exports.txt) do echo %%A >> libtxtquad.def
lib /def:libtxtquad.def /out:libtxtquad.lib /machine:x64
