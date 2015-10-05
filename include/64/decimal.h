/***************************************************************************
 *
 *			   INFORMIX SOFTWARE, INC.
 *
 *			      PROPRIETARY DATA
 *
 *	THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *	INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *	CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *	DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN AGREEMENT 
 *	SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *	THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *	SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *	UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *
 *  Title:	decimal.h
 *  Sccsid:	@(#)decimal.h	9.2	1/12/92  12:06:37
 *		(created from isam/decimal.h version 6.1)
 *  Description:
 *		Header file for decimal data type.
 *
 ***************************************************************************
 */

#ifndef _DECIMAL_H
#define _DECIMAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Unpacked Format (format for program usage)
 *
 *    Signed exponent "dec_exp" ranging from  -64 to +63
 *    Separate sign of mantissa "dec_pos"
 *    Base 100 digits (range 0 - 99) with decimal point
 *	immediately to the left of first digit.
 */

#define DECSIZE 16
#define DECUNKNOWN -2

struct decimal
    {
    short dec_exp;		/* exponent base 100		*/
    short dec_pos;		/* sign: 1=pos, 0=neg, -1=null	*/
    short dec_ndgts;		/* number of significant digits	*/
    char  dec_dgts[DECSIZE];	/* actual digits base 100	*/
    };
#ifdef OSF_MLS
typedef struct decimal ix_dec_t;
#else
typedef struct decimal dec_t;
#endif


/*
 *  A decimal null will be represented internally by setting dec_pos
 *  equal to DECPOSNULL
 */

#define DECPOSNULL	(-1)

/*
 * DECLEN calculates minumum number of bytes
 * necessary to hold a decimal(m,n)
 * where m = total # significant digits and
 *	 n = significant digits to right of decimal
 */

#define DECLEN(m,n)	(((m)+((n)&1)+3)/2)
#define DECLENGTH(len)	DECLEN(PRECTOT(len),PRECDEC(len))

/*
 * DECPREC calculates a default precision given
 * number of bytes used to store number
 */

#define DECPREC(size)	(((size-1)<<9)+2)

/* macros to look at and make encoded decimal precision
 *
 *  PRECTOT(x)		return total precision (digits total)
 *  PRECDEC(x) 		return decimal precision (digits to right)
 *  PRECMAKE(x,y)	make precision from total and decimal
 */

#define PRECTOT(x)	(((x)>>8) & 0xff)
#define PRECDEC(x)	((x) & 0xff)
#define PRECMAKE(x,y)	(((x)<<8) + (y))

/*
 * Packed Format  (format in records in files)
 *
 *    First byte =
 *	  top 1 bit = sign 0=neg, 1=pos
 *	  low 7 bits = Exponent in excess 64 format
 *    Rest of bytes = base 100 digits in 100 complement format
 *    Notes --	This format sorts numerically with just a
 *		simple byte by byte unsigned comparison.
 *		Zero is represented as 80,00,00,... (hex).
 *		Negative numbers have the exponent complemented
 *		and the base 100 digits in 100's complement
 */

#ifdef __STDC__
/* 
** DECIMALTYPE Functions
*/
extern int decadd(struct decimal *n1, struct decimal *n2, struct decimal *n3);
extern int decsub(struct decimal *n1, struct decimal *n2, struct decimal *n3);
extern int decmul(struct decimal *n1, struct decimal *n2, struct decimal *n3);
extern int decdiv(struct decimal *n1, struct decimal *n2, struct decimal *n3);
extern int deccmp(struct decimal *n1, struct decimal *n2);
extern void deccopy(struct decimal *n1, struct decimal *n2);
extern int deccvasc(char *cp, int len, struct decimal *np);
extern int deccvdbl(double dbl, struct decimal *np);
extern int deccvint(int in, struct decimal *np);
extern int deccvlong(int  lng, struct decimal *np);
extern char *dececvt(struct decimal *np, int ndigit, int *decpt, int *sign);
extern char *decfcvt(struct decimal *np, int ndigit, int *decpt, int *sign);
extern void decround(struct decimal *np, int dec_round);
extern int dectoasc(struct decimal *np, char *cp, int len, int right);
extern int dectodbl(struct decimal *np, double *dblp);
extern int dectoint(struct decimal *np, int *ip);
extern int dectolong(struct decimal *np, int  *lngp);
extern void dectrunc(struct decimal *np, int trunc);
extern int deccvflt(double source, struct decimal *destination);
extern int dectoflt(struct decimal *source, float *destination);
#else
extern int decadd();
extern int decsub();
extern int decmul();
extern int decdiv();
extern int deccmp();
extern void deccopy();
extern int deccvasc();
extern int deccvdbl();
extern int deccvint();
extern int deccvlong();
extern char *dececvt();
extern char *decfcvt();
extern void decround();
extern int dectoasc();
extern int dectodbl();
extern int dectoint();
extern int dectolong();
extern void dectrunc();
extern int deccvflt();
extern int dectoflt();
#endif /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* _DECIMAL_H */
