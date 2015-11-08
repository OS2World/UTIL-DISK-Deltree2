
		       DELTREE2 v1.0 for OS/2 2.x

		    Copyright (c) 1993, Russ Herman
			  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of sections 1, 2, 3, 4, 5, 8, and 9 of Larry Wall's
    "Artistic License" which comes with this kit.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    the Artistic License for more details.

    You should have received a copy of the Artistic License with this
    Kit, in the file named "Artistic".

-------------------------------------------------------------------------------

This program is a workalike of the MS-DOS 6.0 deltree command. This means
that it will delete the specified directory and all the files and
subdirectories contained within. This includes system, hidden, and readonly
files. OS/2 invocation is

	deltree2 [/y] [/f] [drive:]path [[drive:]path]...

/y suppresses a prompt asking if you really want to do this action.
/f will completely purge the files regardless of the DELDIR status of that
drive.

Exact error handling procedures and errorlevel returns are undocumented in
the MS-DOS6 Technical Reference Manual. The only thing you can count on
is that if the returned errorlevel is zero, everything went smoothly.
One difference between MS and this version is that if the specified
directory is empty, their version just goes ahead and deletes it. This
version will seek confirmation unless the /y toggle is in use. There may
be other differences I'm unaware of. Don't ask about wildcard specifications.
I'm not sure what either version will do. Just be careful.

This program was developed with Borland C++ for OS/2 to produce
the OS/2 version. It may also be compiled with BCC++ 3.1 to produce
a DOS version for the benefit of non-MS-DOS6 users.

						    Russ Herman
						    Nov. 7, 1993
