# licensing information

dj64dev project is distributed under the terms of the
GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Sources in `src/libc/*` and `src/include/*` that refer to "copying.dj",
are distributed under the terms of the GNU Library General Public License,
either version 2.1, or (at your option) any later version.
See
[this](https://www.delorie.com/archives/browse.cgi?p=djgpp/2024/06/29/04:14:46)
discussion for details. See [copying.dj](copying.dj/copying.dj) for
the detailed licensing information.

`src/makemake.c` and `src/mkstubs.c` are distributed under
the terms of the GNU General Public License, either version 2, or
(at your option) any later version (GPLv2+).
See [copying.dj](copying.dj/copying.dj) for more information.

All files in `src/libc/*` and `src/include/*` are using the
LGPL-compatible licenses.
Files ported from fdpp or comcom32 projects and originally distributed
under GPLv3+, were donated to dj64dev project by their author (@stsp)
and re-licensed to LGPLv3+ by himself.
`libc/posix/wchar/wchar.c` is ported from tass64 project, and is
[allowed](https://sourceforge.net/p/tass64/feature-requests/25/)
to be distributed under LGPLv3+ by its author.
Files ported from musl are under MIT license.

Sources in `src/djdev64/*` are distributed under the terms of the
GNU General Public License, either version 3, or (at your option)
any later version (GPLv3+). That includes the sources without an
explicit licensing information within.

All sources used to build `crt0.elf`, `libc_s.a` and `libdj64*`
are distributed under the terms of the GNU LGPL, with the particular
version of LGPL either written in the source file itself, or, in case
of the reference to "copying.dj", is LGPLv2.1+. That allows you to
apply the terms of an LGPL when building your source code with dj64
tool-chain.

All sources used to build `libdjdev64*` and `libdjstub64*` libraries
are distributed under the GNU General Public License, either version 3,
or (at your option) any later version (GPLv3+).

Files in `demos/*` are distributed under the GNU Lesser General Public
License, either version 3, or (at your option) any later version (LGPLv3+).

Files in the top dir and in `src/libc/*` that have no explicit licensing
information within (small scripts, pkg-config files, makefile rules etc)
are distributed under the terms of the GNU Lesser General Public License,
either version 3, or (at your option) any later version (LGPLv3+).

A copy of the files "LICENSE" and "COPYING.LESSER" are included with this
document. If you did not receive a copy of these files, you may
obtain them from the dj64dev project home page, or from an FSF site.
