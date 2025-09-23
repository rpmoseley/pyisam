/*
 * This source forces the inclusion of both the IFISAM libraries, but it not actually called by any otherthing.
 * It avoids the warning given by the meson build system about no sources being used.
 */

extern void stchar(char *, char *, int);      /* Defined in libifisamx.so */
extern int  isopen(char *, int);              /* Defined in libifisam.so */

void _dummy(void)
{
  char rec[2];
  stchar("_dummy", rec, 1);
  isopen("_dummy", 0);
}
