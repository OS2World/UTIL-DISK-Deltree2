/************************************************************************
   DELTREE2 1.0 1993 Russ Herman <rwh@gov.on.ca>

   This program is pretty much a clone of DOS6 DELTREE for MDOS and OS/2,
   including HPFS filesystems. However, DELTREE2 may remove files
   in different order and return different error codes for failure
   than DELTREE does.

   Compile with BC++ 1.0 for OS/2 and BC++ 3.1 for DOS; run as DELTREE
   under COMMAND.COM and DELTREE2 under CMD.EXE

   Added /f toggle for forcedelete in OS/2 version.

   It returns an error code (>0) if it fails, or 0 if everything goes
   well.
************************************************************************/

#ifdef __OS2__
#define INCL_DOSMISC
#define INCL_DOSFILEMGR
#include <os2.h>
#define INCL_ERRORS
#include <bseerr.h>
int CHMOD_NORMAL(char *, int, int);
#define FNAME find.achName[0]
#define FATTR find.attrFile
#define FILE_ABNORMAL (FILE_READONLY | FILE_SYSTEM | FILE_HIDDEN | FILE_ARCHIVED)
#define CHDIR(a) DosSetCurrentDir(a)
#define NOMOREFILES(x) (x == ERROR_NO_MORE_FILES)
#else
#include <dir.h>
#define FALSE 0
#define TRUE !FALSE
#define HDIR unsigned long
#define ULONG unsigned long
#define FNAME find.ff_name[0]
#define FATTR find.ff_attrib
#define FILE_ABNORMAL (FA_RDONLY | FA_SYSTEM | FA_HIDDEN | FA_ARCH)
#define CHDIR(a) chdir(a)
#define MUST_HAVE_DIRECTORY FA_DIREC
#define FILE_DIRECTORY FA_DIREC
#define HDIR_CREATE 0x0FFFF
#define DosFindFirst(a,b,c,d,e,f,g) findfirst(a, d, c)
#define DosFindNext(a,b,c,d) findnext(b)
#define DosFindClose(x) /* */
#define DosDeleteDir(x) rmdir(x)
#define DosDelete(x) unlink(x)
#define NOMOREFILES(x) ((x != 0) && (_doserrno == ENMFILE))
#define FILEFINDBUF3 struct ffblk
#endif
#include <dos.h>
#include <io.h>
#include <errno.h>
#include <direct.h>
#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#define DOSFINDBAD(x) assert((x == 0) || NOMOREFILES(x))

void main(int,char **);
int nextdir(int);
int getyn(void);
#ifdef DBG
int nextdirdepth = 0;
char *dbgdel[]={"","force"};
#endif
int findret, forcedel = 0;
#ifdef STKLEN
extern unsigned _stklen=STKLEN;
#endif

void main(int argc,char *argv[])
{
    char *basedir, *p;
    FILEFINDBUF3 find; HDIR dirhandle; ULONG srch_cnt;
    int newdrive, urdrive, rootdir, dotdir, dotdotdir;
    int asking=TRUE, retcode=TRUE;

    while (--argc)  {
	++argv;
	if ((argv[0][0] == '/') || (argv[0][0] == '-')) {
	    switch(toupper(argv[0][1])) {
		case 'Y': asking = FALSE; break;

#ifdef __OS2__
		case 'F': forcedel = 1; break;
#endif
		default : printf("Invalid switch - %c\n", argv[0][1]); abort();
		}
	    }
	else
	    break;
	}
    if (!argc) {
	puts("DELTREE2 v1.0\nRequired parameter missing");
	abort();
	}
    urdrive = getdisk();
#ifdef __OS2__
    DosError(FERR_DISABLEHARDERR);
#endif

    /* Process each command line parameter */
    while (argc--) {
	basedir = *argv++;
	if (asking) {
	    printf("Delete directory \"%s\" and all its subdirectories? [yn] ",
		   basedir);
	    if (!getyn())
		continue;
	    }
	setdisk(urdrive);
	srch_cnt = 1; dirhandle = HDIR_CREATE;
	findret = DosFindFirst(basedir, &dirhandle, MUST_HAVE_DIRECTORY,
			       &find, sizeof(find), &srch_cnt,
			       FIL_STANDARD);
	if (findret == 0) {
#ifndef __OS2__
	dirhandle = 1;
#endif
	    DosFindClose(dirhandle);
	    }
	else
	    continue;  /* no such directory, goto next argument */
	if (basedir[1] == ':') {
	    /* Need to change drives */
	    newdrive = toupper(basedir[0])-'A';
	    if (newdrive != urdrive) {
		setdisk(newdrive);
		if (newdrive != getdisk())
		    continue;  /* cannot switch drives, goto next argument */
	       }
	    basedir+=2;
	    }
	assert (NULL != (p=getcwd(NULL, MAXPATH)));
	dotdir = !strcmp(basedir, ".") ||
		 !strcmp(basedir, p+2);
	free(p);
	dotdotdir = !strcmp(basedir, "..");
	rootdir = (!strcmp(basedir,"\\") ||
		   !strcmp(basedir,"/"));
	printf("Deleting %s...\n", basedir);
	if (rootdir) {
	    CHDIR("\\");
	    retcode &= nextdir(FALSE);
	    }
	else if (dotdir) {
	    retcode &= nextdir(FALSE);
	    }
	else if (dotdotdir) {
	    CHDIR("..");
	    retcode &= nextdir(FALSE);
	    }
	else {
	    if (CHDIR(basedir))
		continue /* cannot change directory - goto next argument */;
	    retcode &= nextdir(FALSE);
	    CHDIR("..");
#ifndef DBG
	    retcode &= (0 == DosDeleteDir(basedir));
#endif
	    }
	/* goto next argument */
	}
    setdisk(urdrive);
    exit(retcode ? 0 : 1);
    }

int nextdir(int toplev)
{
#ifdef __OS2__
    FILEFINDBUF3 find;
    ULONG srch_cnt;
    HDIR dirhandle;
    APIRET findret;
#else
    struct ffblk find;
    ULONG srch_cnt;
    HDIR dirhandle;
    int findret;
#endif
    int retcode=TRUE, c, rc, nomore;

#ifdef DBG
    printf("entered nextdir %d\n", nextdirdepth++);
#endif

    srch_cnt = 1; dirhandle = HDIR_CREATE;
    findret =  DosFindFirst("*.*", &dirhandle, MUST_HAVE_DIRECTORY,
			     &find, sizeof(find), &srch_cnt,
			     FIL_STANDARD);
#ifdef __OS2__
    nomore = (findret == ERROR_NO_MORE_FILES);
#else
    nomore = ((findret == -1) && (_doserrno == ENMFILE));
    if (!nomore)
	dirhandle = 1;
#endif
#ifdef DBG
    printf("DosFindFirstDir returned %d(%d)\n", (int) findret, nomore);
    DOSFINDBAD(findret);
    assert(dirhandle != HDIR_CREATE);
#endif
    if (nomore)
	findret = 0;
    while (!nomore) {
	if ((findret == 0) && (FNAME != '.') && (FATTR & FILE_DIRECTORY)) {
	    /* descend next level */
	    if (toplev)
		printf("Deleting %s...\n", &FNAME);
#ifdef DBG
	    printf("descending to scan %s\n", &FNAME);
	    assert(0 == CHDIR(&FNAME));
	    retcode &= nextdir(FALSE);
	    /* ascend level and onto next directory */
	    assert(0 == CHDIR(".."));
	    printf("removing dir  %s\n", &FNAME);
#else
	    /* descend level */
	    if (CHDIR(&FNAME)) {
		/* descent impossible */
		retcode = FALSE;
		}
	    else {
		/* process next directory */
		retcode &= nextdir(FALSE);
		/* ascend level */
		if (CHDIR("..")) {
		    /* Something woefully wrong ... */
		    if (dirhandle != HDIR_CREATE) {
			DosFindClose(dirhandle);
			dirhandle = HDIR_CREATE;
			}
		    return(FALSE);
		    }
		else {
		    /* delete empty directory */
		    retcode &= (0 == DosDeleteDir(&FNAME));
		    }
		}
#endif
	    }
	/* Position to next entry */
	findret = DosFindNext(dirhandle, &find, sizeof(find), &srch_cnt);
#ifdef __OS2__
	nomore = (findret == ERROR_NO_MORE_FILES);
#else
	nomore = ((findret == -1) && (_doserrno == ENMFILE));
#endif
#ifdef DBG
	printf("DosFindNextDir returned %d(%d)\n", (int) findret, nomore);
	DOSFINDBAD(findret);
#endif
	if (nomore)
	    findret = 0;
	/* Process next entry */
	continue;
	}
    if (dirhandle != HDIR_CREATE) {
	DosFindClose(dirhandle);
	dirhandle = HDIR_CREATE;
	}
    srch_cnt = 1;

/* Now delete the files */
    findret = DosFindFirst("*.*",&dirhandle, FILE_ABNORMAL, &find,
			    sizeof(find), &srch_cnt, FIL_STANDARD);
#ifdef __OS2__
    nomore = (findret == ERROR_NO_MORE_FILES);
#else
    nomore = ((findret == -1) && (_doserrno == ENMFILE));
    if (!nomore)
	dirhandle = 1;
#endif
#ifdef DBG
    printf("DosFindFirstFile returned %d(%d)\n", (int) findret, nomore);
    DOSFINDBAD(findret);
#endif
    if (nomore)
	findret = 0;

     while (!nomore) {
	 if (findret == 0) {
	    /* Process if file OK */
	    if (toplev)
		printf("Deleting %s...\n", &FNAME);
#ifdef DBG
	    if (!findret && (FATTR & FILE_ABNORMAL))
		printf("making normal and %sdeleting %s\n",
		       dbgdel[forcedel], &FNAME);
	    else
		printf("%sdeleting %s\n",
		       dbgdel[forcedel], &FNAME);
#else
	    if (!findret && (FATTR & FILE_ABNORMAL))
		rc = _chmod(&FNAME, 1, 0);
	    if (!rc) {
#ifdef __OS2__
		if (forcedel)
		    retcode &= (0 == DosForceDelete(&FNAME));
		else
#endif
		    retcode &= (0 == DosDelete(&FNAME));
		}
	    else
		retcode = FALSE;
#endif
	    }
	/* Advance to next file */
	findret = DosFindNext(dirhandle, &find, sizeof(find), &srch_cnt);
#ifdef __OS2__
	nomore = (findret == ERROR_NO_MORE_FILES);
#else
	nomore = ((findret == -1) && (_doserrno == ENMFILE));
#endif
#ifdef DBG
	printf("DosFindNextFile returned %d(%d)\n", (int) findret, nomore);
	DOSFINDBAD(findret);
#endif
	if (nomore)
	    findret = 0;
	/* Process next file */
	continue;
	}
    if (dirhandle != HDIR_CREATE)
	DosFindClose(dirhandle);
#ifdef DBG
    printf("leaving nextdir %d\n", --nextdirdepth);
#endif
    return(retcode);
    }

int getyn()
{
    char response[2], c[2];

    /* Accept only y<ret>, Y<ret>, n<ret>, N<ret> */
    while (!((response[1] == '\r') &&
	    ((response[0] == 'N') ||
	     (response[0] == 'Y')))) {
	response[0] = response[1]; c[0] = c[1];
	response[1] = toupper(c[1]=getch());
	if (isprint(c[1])) {
	    putch(c[1]);
	    putch('\b');
	    }
	}
    putch('\r'); putch('\n');
    return(response[0] == 'Y');
}








