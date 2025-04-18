/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2010 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: reader_gms.c,v 1.46 2010/09/27 17:20:23 bzfheinz Exp $"

/**@file   reader_gms.c
 * @ingroup FILEReaders 
 * @brief  GAMS file reader
 * @author Ambros Gleixner
 * @author Stefan Vigerske
 *
 * @todo Test for uniqueness of variable names (after cutting down).
 * @todo Check for words reserved for GAMS.
 * @todo Routines for reading.
 * @todo Indicator constraints.
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#else
#include <strings.h>
#endif

#include "scip/reader_gms.h"
#include "scip/cons_knapsack.h"
#include "scip/cons_linear.h"
#include "scip/cons_logicor.h"
#include "scip/cons_quadratic.h"
#include "scip/cons_soc.h"
#include "scip/cons_setppc.h"
#include "scip/cons_varbound.h"
#ifdef WITH_SIGNPOWER
#include "cons_signpower.h"
#endif
#include "scip/pub_misc.h"

#define READER_NAME             "gmsreader"
#define READER_DESC             "file reader for MI(NL)(SOC)Ps in GAMS file format"
#define READER_EXTENSION        "gms"


/*
 * Data structures
 */
#define GMS_MAX_LINELEN      256
#define GMS_MAX_PRINTLEN     256       /**< the maximum length of any line is 255 + '\\0' = 256*/
#define GMS_MAX_NAMELEN      64        /**< the maximum length for any name is 63 + '\\0' = 64 */
#define GMS_PRINTLEN         100



/*
 * Local methods (for writing)
 */

static const char badchars[] = "#*+/-@";

/** transforms given variables, scalars, and constant to the corresponding active variables, scalars, and constant */
static
SCIP_RETCODE getActiveVariables(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR**            vars,               /**< vars array to get active variables for */
   SCIP_Real*            scalars,            /**< scalars a_1, ..., a_n in linear sum a_1*x_1 + ... + a_n*x_n + c */
   int*                  nvars,              /**< pointer to number of variables and values in vars and vals array */
   SCIP_Real*            constant,           /**< pointer to constant c in linear sum a_1*x_1 + ... + a_n*x_n + c  */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   int requiredsize;
   int v;

   assert( scip != NULL );
   assert( vars != NULL );
   assert( scalars != NULL );
   assert( nvars != NULL );
   assert( constant != NULL );

   if( transformed )
   {
      SCIP_CALL( SCIPgetProbvarLinearSum(scip, vars, scalars, nvars, *nvars, constant, &requiredsize, TRUE) );

      if( requiredsize > *nvars )
      {
         SCIP_CALL( SCIPreallocBufferArray(scip, &vars, requiredsize) );
         SCIP_CALL( SCIPreallocBufferArray(scip, &scalars, requiredsize) );
         
         SCIP_CALL( SCIPgetProbvarLinearSum(scip, vars, scalars, nvars, requiredsize, constant, &requiredsize, TRUE) );
         assert( requiredsize <= *nvars );
      }
   }
   else
   {
      for( v = 0; v < *nvars; ++v )
      {
         SCIP_CALL( SCIPvarGetOrigvarSum(&vars[v], &scalars[v], constant) );
      }
   }
   return SCIP_OKAY;
}

/** clears the given line buffer */
static
void clearLine(
   char*                 linebuffer,         /**< line */
   int*                  linecnt             /**< number of charaters in line */
   )
{
   assert( linebuffer != NULL );
   assert( linecnt != NULL );

   (*linecnt) = 0;
   linebuffer[0] = '\0';
}

/** ends the given line with '\\0' and prints it to the given file stream */
static
void endLine(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   char*                 linebuffer,         /**< line */
   int*                  linecnt             /**< number of charaters in line */
   )
{
   assert( scip != NULL );
   assert( linebuffer != NULL );
   assert( linecnt != NULL );

   if( (*linecnt) > 0 )
   {
      linebuffer[(*linecnt)] = '\0';
      SCIPinfoMessage(scip, file, "%s\n", linebuffer);
      clearLine(linebuffer, linecnt);
   }
}

/** appends extension to line and prints it to the give file stream if the
 *  line exceeded the length given in the define GMS_PRINTLEN */
static
void appendLine(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   char*                 linebuffer,         /**< line */
   int*                  linecnt,            /**< number of characters in line */
   const char*           extension           /**< string to extend the line */
   )
{
   size_t len;
   assert( scip != NULL );
   assert( linebuffer != NULL );
   assert( linecnt != NULL );
   assert( extension != NULL );
   assert( strlen(linebuffer) + strlen(extension) < GMS_MAX_PRINTLEN );

   /* NOTE: avoid
    *   sprintf(linebuffer, "%s%s", linebuffer, extension); 
    * because of overlapping memory areas in memcpy used in sprintf.
    */
   len = strlen(linebuffer);
   strncat(linebuffer, extension, GMS_MAX_PRINTLEN - len);

   (*linecnt) += (int) strlen(extension);

   SCIPdebugMessage("linebuffer <%s>, length = %zu\n", linebuffer, len);

   if( (*linecnt) > GMS_PRINTLEN )
      endLine(scip, file, linebuffer, linecnt);
}

/** checks string for occurences of '#', '*', '+', '/', and '-' and replaces those by '_' */
static
void conformName(
   char*                      name                /**< string to adjust */
   )
{
   const char* badchar;
   
   assert( name != NULL );
   
   for( badchar = badchars; *badchar; ++badchar )
   {
      char* c = strchr(name, *badchar);

      while( c != NULL )
      {
         assert( *c == *badchar );

         *c = '_';
         c = strchr(c, *badchar);
      }
   }
}

/* print first len-1 characters of name to string s and replace '#', '*', '+', '/', and '-' by '_' if necessary */
static
SCIP_RETCODE printConformName(
   SCIP*                 scip,               /**< SCIP data structure */
   char*                 t,                  /**< target string */
   int                   len,                /**< length of t */
   const char*           name                /**< source string or format string */
   )
{
   SCIP_Bool replaceforbiddenchars;

   assert( t != NULL );
   assert( len > 0 );

   SCIP_CALL( SCIPgetBoolParam(scip, "reading/gmsreader/replaceforbiddenchars", &replaceforbiddenchars) );

   (void) SCIPsnprintf(t, len, "%s", name);

   if( replaceforbiddenchars )
      conformName(t);

   return SCIP_OKAY;
}



/* retransform to active variables and print in GAMS format to file stream with surrounding bracket, pre- and suffix */
static
SCIP_RETCODE printActiveVariables(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   char*                 linebuffer,         /**< line */
   int*                  linecnt,            /**< number of charaters in line */
   const char*           prefix,             /**< prefix (maybe NULL) */
   const char*           suffix,             /**< suffix (maybe NULL) */
   int                   nvars,              /**< number of variables */
   SCIP_VAR**            vars,               /**< array of variables */
   SCIP_Real*            vals,               /**< array of values (or NULL if all ones) */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   int v;
   int closingbracket;

   SCIP_VAR* var;
   char varname[GMS_MAX_NAMELEN];
   char buffer[GMS_MAX_PRINTLEN];
   char ext[GMS_MAX_PRINTLEN];

   SCIP_VAR** activevars = NULL;
   SCIP_Real* activevals = NULL;
   int nactivevars;
   SCIP_Real activeconstant = 0.0;

   assert( scip != NULL );
   assert( nvars == 0 || vars != NULL );
   assert( nvars > 0 || (vars == NULL && vals == NULL) );

   if( *linecnt == 0 )
      /* we start a new line; therefore we tab this line */
      appendLine(scip, file, linebuffer, linecnt, "     ");

   if( nvars == 0 )
   {
      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s(0)%s", prefix ? prefix : "", suffix ? suffix : "");

      appendLine(scip, file, linebuffer, linecnt, buffer);
   }
   else
   {
      nactivevars = nvars;

      /* duplicate variable and value array */
      SCIP_CALL( SCIPduplicateBufferArray(scip, &activevars, vars, nactivevars) );
      if( vals != NULL )
      {
         SCIP_CALL( SCIPduplicateBufferArray(scip, &activevals, vals, nactivevars) );
      }
      else
      {
         SCIP_CALL( SCIPallocBufferArray(scip, &activevals, nactivevars) );
         
         for( v = 0; v < nactivevars; ++v )
            activevals[v] = 1.0;
      }

      /* retransform given variables to active variables */
      SCIP_CALL( getActiveVariables(scip, activevars, activevals, &nactivevars, &activeconstant, transformed) );

      assert( nactivevars == 0 || activevals != NULL );

      if( nactivevars == 0 && SCIPisZero(scip, activeconstant) )
      {
         if( *linecnt == 0 )
            /* we start a new line; therefore we tab this line */
            appendLine(scip, file, linebuffer, linecnt, "     ");

         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s(0)%s", prefix ? prefix : "", suffix ? suffix : "");

         appendLine(scip, file, linebuffer, linecnt, buffer);
      }
      else
      {
         /* buffer prefix */
         (void) SCIPsnprintf(ext, GMS_MAX_PRINTLEN, "%s(", prefix);

         /* find position of closing bracket */
         closingbracket = nactivevars;
         if( SCIPisZero(scip, activeconstant) )
         {
            do
               --closingbracket;
            while( SCIPisZero(scip, activevals[closingbracket]) && closingbracket > 0 );
         }

         /* print active variables */
         for( v = 0; v < nactivevars; ++v )
         {
            var = activevars[v];
            assert( var != NULL );

            if( !SCIPisZero(scip, activevals[v]) )
            {
               if( *linecnt == 0 )
                  /* we start a new line; therefore we tab this line */
                  appendLine(scip, file, linebuffer, linecnt, "     ");

               SCIP_CALL( printConformName(scip, varname, GMS_MAX_NAMELEN, SCIPvarGetName(var)) );

               if( SCIPisEQ(scip, activevals[v], 1.0) )
                  (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s%s%s%s%s", ext, strchr(ext, '(') == NULL ? "+" : "",
                        varname, (v == closingbracket) ? ")" : "", (v == closingbracket && suffix) ? suffix : "");
               else if( SCIPisEQ(scip, activevals[v], -1.0) )
                  (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s-%s%s%s", ext,
                        varname, (v == closingbracket) ? ")" : "", (v == closingbracket && suffix) ? suffix : "");
               else if( strchr(ext, '(') != NULL )
                  (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s%.15g*%s%s%s", ext,
                        activevals[v], varname, (v == closingbracket) ? ")" : "", (v == closingbracket && suffix) ? suffix : "");
               else
                  (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s%+.15g*%s%s%s", ext,
                        activevals[v], varname, (v == closingbracket) ? ")" : "", (v == closingbracket && suffix) ? suffix : "");

               appendLine(scip, file, linebuffer, linecnt, buffer);

               (void) SCIPsnprintf(ext, GMS_MAX_PRINTLEN, (*linecnt == 0) ? "" : " ");
            }
         }

         /* print active constant */
         if( !SCIPisZero(scip, activeconstant) )
         {
            if( *linecnt == 0 )
               /* we start a new line; therefore we tab this line */
               appendLine(scip, file, linebuffer, linecnt, "     ");

            (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s%+.15g)%s", ext, activeconstant, suffix ? suffix : "");

            appendLine(scip, file, linebuffer, linecnt, buffer);
         }
         /* nothing has been printed, yet */
         else if( strchr(ext, '(') != NULL )
         {
            if( *linecnt == 0 )
               /* we start a new line; therefore we tab this line */
               appendLine(scip, file, linebuffer, linecnt, "     ");

            (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s(0)%s", prefix ? prefix : "", suffix ? suffix : "");

            appendLine(scip, file, linebuffer, linecnt, buffer);
         }
      }

      /* free buffer arrays */
      SCIPfreeBufferArray(scip, &activevars);
      SCIPfreeBufferArray(scip, &activevals);
   }

   return SCIP_OKAY;
}



/* print linear row in GAMS format to file stream (without retransformation to active variables) */
static
SCIP_RETCODE printLinearRow(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           rowname,            /**< row name */
   const char*           rownameextension,   /**< row name extension */
   const char*           type,               /**< row type ("=e=", "=l=", or "=g=") */
   int                   nvars,              /**< number of variables */
   SCIP_VAR**            vars,               /**< array of variables */
   SCIP_Real*            vals,               /**< array of values */
   SCIP_Real             rhs                 /**< right hand side */
   )
{
   int v;
   char linebuffer[GMS_MAX_PRINTLEN] = { '\0' };
   int linecnt;

   SCIP_VAR* var;
   char varname[GMS_MAX_NAMELEN];
   char consname[GMS_MAX_NAMELEN + 3]; /* four extra characters for ' ..' */
   char buffer[GMS_MAX_PRINTLEN];

   assert( scip != NULL );
   assert( strcmp(type, "=e=") == 0 || strcmp(type, "=l=") == 0 || strcmp(type, "=g=") == 0);
   assert( nvars == 0 || (vars != NULL && vals != NULL) );

   clearLine(linebuffer, &linecnt);

   /* start each line with a space */
   appendLine(scip, file, linebuffer, &linecnt, " ");

   /* print row name */
   if( strlen(rowname) > 0 || strlen(rownameextension) > 0 )
   {
      (void) SCIPsnprintf(buffer, GMS_MAX_NAMELEN + 3, "%s%s ..", rowname, rownameextension);
      SCIP_CALL( printConformName(scip, consname, GMS_MAX_NAMELEN + 3, buffer) );
      appendLine(scip, file, linebuffer, &linecnt, consname);
   }

   /* print coefficients */
   if( nvars == 0 )
   {
      /* we start a new line; therefore we tab this line */
      if( linecnt == 0 )
         appendLine(scip, file, linebuffer, &linecnt, "     ");

      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " 0");
      
      appendLine(scip, file, linebuffer, &linecnt, buffer);
   }

   for( v = 0; v < nvars; ++v )
   {
      var = vars[v];
      assert( var != NULL );

      /* we start a new line; therefore we tab this line */
      if( linecnt == 0 )
         appendLine(scip, file, linebuffer, &linecnt, "     ");

      SCIP_CALL( printConformName(scip, varname, GMS_MAX_NAMELEN, SCIPvarGetName(var)) );
      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " %+.15g*%s", vals[v], varname);
      
      appendLine(scip, file, linebuffer, &linecnt, buffer);
   }

   /* print right hand side */
   if( SCIPisZero(scip, rhs) )
      rhs = 0.0;

   (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " %s %.15g;", type, rhs);

   /* we start a new line; therefore we tab this line */
   if( linecnt == 0 )
      appendLine(scip, file, linebuffer, &linecnt, "     ");
   appendLine(scip, file, linebuffer, &linecnt, buffer);

   endLine(scip, file, linebuffer, &linecnt);

   return SCIP_OKAY;
}



/** prints given linear constraint information in GAMS format to file stream */
static
SCIP_RETCODE printLinearCons(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           rowname,            /**< name of the row */
   int                   nvars,              /**< number of variables */
   SCIP_VAR**            vars,               /**< array of variables */
   SCIP_Real*            vals,               /**< array of coefficients values (or NULL if all coefficient values are 1) */
   SCIP_Real             lhs,                /**< left hand side */
   SCIP_Real             rhs,                /**< right hand side */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   int v;
   SCIP_VAR** activevars = NULL;
   SCIP_Real* activevals = NULL;
   int nactivevars;
   SCIP_Real activeconstant = 0.0;

   assert( scip != NULL );
   assert( rowname != NULL );

   /* The GAMS format does not forbid that the variable array is empty */
   assert( nvars == 0 || vars != NULL );

   assert( lhs <= rhs );
   
   if( SCIPisInfinity(scip, -lhs) && SCIPisInfinity(scip, rhs) )
      return SCIP_OKAY;

   nactivevars = nvars;
   if( nvars > 0 ) 
   {
      /* duplicate variable and value array */
      SCIP_CALL( SCIPduplicateBufferArray(scip, &activevars, vars, nactivevars) );
      if( vals != NULL )
      {
         SCIP_CALL( SCIPduplicateBufferArray(scip, &activevals, vals, nactivevars) );
      }
      else
      {
         SCIP_CALL( SCIPallocBufferArray(scip, &activevals, nactivevars) );
         
         for( v = 0; v < nactivevars; ++v )
            activevals[v] = 1.0;
      }

      /* retransform given variables to active variables */
      SCIP_CALL( getActiveVariables(scip, activevars, activevals, &nactivevars, &activeconstant, transformed) );
   }
   
   /* print row(s) in GAMS format */
   if( SCIPisEQ(scip, lhs, rhs) )
   {
      assert( !SCIPisInfinity(scip, rhs) );

      /* print equality constraint */
      SCIP_CALL( printLinearRow(scip, file, rowname, "", "=e=", 
            nactivevars, activevars, activevals, rhs - activeconstant) );
   }
   else
   {
      if( !SCIPisInfinity(scip, -lhs) )
      {
         /* print inequality ">=" */
         SCIP_CALL( printLinearRow(scip, file, rowname, SCIPisInfinity(scip, rhs) ? "" : "_lhs", "=g=",
               nactivevars, activevars, activevals, lhs - activeconstant) );
      }
      if( !SCIPisInfinity(scip, rhs) )
      {
         /* print inequality "<=" */
         SCIP_CALL( printLinearRow(scip, file, rowname, SCIPisInfinity(scip, -lhs) ? "" : "_rhs", "=l=",
               nactivevars, activevars, activevals, rhs - activeconstant) );
      }
   }

   if( nvars > 0 )
   {
      /* free buffer arrays */
      SCIPfreeBufferArray(scip, &activevars);
      SCIPfreeBufferArray(scip, &activevals);
   }
   
   return SCIP_OKAY;
}



/* print quadratic row in GAMS format to file stream (performing retransformation to active variables) */
static
SCIP_RETCODE printQuadraticRow(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           rowname,            /**< row name */
   const char*           rownameextension,   /**< row name extension */
   const char*           type,               /**< row type ("=e=", "=l=", or "=g=") */
   int                   nlinvars,           /**< number of linear terms */
   SCIP_VAR**            linvars,            /**< variables in linear part */ 
   SCIP_Real*            lincoeffs,          /**< coefficients of variables in linear part */ 
   int                   nquadvarterms,      /**< number of quadratic variable terms */
   SCIP_QUADVARTERM*     quadvarterms,       /**< quadratic variable terms */
   int                   nbilinterms,        /**< number of bilinear terms */
   SCIP_BILINTERM*       bilinterms,         /**< bilinear terms */
   SCIP_Real             rhs,                /**< right hand side */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   int t;
   char linebuffer[GMS_MAX_PRINTLEN] = { '\0' };
   int linecnt;

   SCIP_VAR* var;
   char consname[GMS_MAX_NAMELEN + 3]; /* four extra characters for ' ..' */
   char buffer[GMS_MAX_PRINTLEN];

   assert( scip != NULL );
   assert( strlen(rowname) > 0 || strlen(rownameextension) > 0 );
   assert( strcmp(type, "=e=") == 0 || strcmp(type, "=l=") == 0 || strcmp(type, "=g=") == 0 );
   assert( nlinvars == 0 || (linvars != NULL && lincoeffs != NULL) );
   assert( nquadvarterms == 0 || quadvarterms != NULL );
   assert( nbilinterms == 0 || bilinterms != NULL );
   assert( nquadvarterms > 0 || nbilinterms == 0 );

   clearLine(linebuffer, &linecnt);

   /* start each line with a space */
   appendLine(scip, file, linebuffer, &linecnt, " ");

   /* print row name */
   (void) SCIPsnprintf(buffer, GMS_MAX_NAMELEN + 3, "%s%s ..", rowname, rownameextension);
   SCIP_CALL( printConformName(scip, consname, GMS_MAX_NAMELEN + 3, buffer) );

   appendLine(scip, file, linebuffer, &linecnt, consname);

   /* print linear terms */
   if( nlinvars > 0 )
   {
      SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", " ", nlinvars, linvars, lincoeffs, transformed) );
   }
   
   /* print linear coefficients of quadratic terms */
   for( t = 0; t < nquadvarterms; ++t )
   {
      var = quadvarterms[t].var;
      assert( var != NULL );

      if( !SCIPisZero(scip, quadvarterms[t].sqrcoef) )
      {
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%+.15g*", quadvarterms[t].lincoef);

         SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, buffer, NULL, 1, &var, NULL, transformed) );
      }
   }

   /* print square coefficients of quadratic terms */
   for( t = 0; t < nquadvarterms; ++t )
   {
      var = quadvarterms[t].var;
      assert( var != NULL );

      if( !SCIPisZero(scip, quadvarterms[t].sqrcoef) )
      {
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%+.15g*sqr", quadvarterms[t].sqrcoef);

         SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, buffer, NULL, 1, &var, NULL, transformed) );
      }
   }

   /* print bilinear terms */
   for( t = 0; t < nbilinterms; ++t )
   {
      if( !SCIPisZero(scip, bilinterms[t].coef) )
      {
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%+.15g*", bilinterms[t].coef);

         /* print first variable (retransformed to active variables) */
         var = bilinterms[t].var1;
         assert( var != NULL );

         SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, buffer, "", 1, &var, NULL, transformed) );

         /* print second variable (retransformed to active variables) */
         var = bilinterms[t].var2;
         assert( var != NULL );

         SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "*", " ", 1, &var, NULL, transformed) );
      }
   }

   /* print right hand side */
   if( linecnt == 0 )
      /* we start a new line; therefore we tab this line */
      appendLine(scip, file, linebuffer, &linecnt, "     ");

   if( SCIPisZero(scip, rhs) )
      rhs = 0.0;

   (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s%s %.15g;", (nlinvars == 0 && nquadvarterms == 0) ? "0 " : "", type, rhs);

   appendLine(scip, file, linebuffer, &linecnt, buffer);

   endLine(scip, file, linebuffer, &linecnt);

   return SCIP_OKAY;
}



/** prints given quadratic constraint information in GAMS format to file stream */
static
SCIP_RETCODE printQuadraticCons(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           rowname,            /**< name of the row */
   int                   nlinvars,           /**< number of linear terms */
   SCIP_VAR**            linvars,            /**< variables in linear part */ 
   SCIP_Real*            lincoeffs,          /**< coefficients of variables in linear part */ 
   int                   nquadvarterms,      /**< number of quadratic variable terms */
   SCIP_QUADVARTERM*     quadvarterms,       /**< quadratic variable terms */
   int                   nbilinterms,        /**< number of bilinear terms */
   SCIP_BILINTERM*       bilinterms,         /**< bilinear terms */
   SCIP_Real             lhs,                /**< left hand side */
   SCIP_Real             rhs,                /**< right hand side */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   assert( scip != NULL );
   assert( rowname != NULL );
   assert( nlinvars == 0 || (linvars != NULL && lincoeffs != NULL) );
   assert( nquadvarterms == 0 || quadvarterms != NULL );
   assert( nbilinterms == 0 || bilinterms != NULL );
   assert( nquadvarterms > 0 || nbilinterms == 0 );
   assert( lhs <= rhs );
   
   if( SCIPisInfinity(scip, -lhs) && SCIPisInfinity(scip, rhs) )
      return SCIP_OKAY;

   /* print row(s) in GAMS format */
   if( SCIPisEQ(scip, lhs, rhs) )
   {
      assert( !SCIPisInfinity(scip, rhs) );

      /* print equality constraint */
      SCIP_CALL( printQuadraticRow(scip, file, rowname, "", "=e=",
         nlinvars, linvars, lincoeffs,
         nquadvarterms, quadvarterms,
         nbilinterms, bilinterms, rhs, transformed) );
   }
   else
   {
      if( !SCIPisInfinity(scip, -lhs) )
      {
         /* print inequality ">=" */
         SCIP_CALL( printQuadraticRow(scip, file, rowname, SCIPisInfinity(scip, rhs) ? "" : "_lhs", "=g=",
            nlinvars, linvars, lincoeffs,
            nquadvarterms, quadvarterms,
            nbilinterms, bilinterms, lhs, transformed) );
      }
      if( !SCIPisInfinity(scip, rhs) )
      {
         /* print inequality "<=" */
         SCIP_CALL( printQuadraticRow(scip, file, rowname, SCIPisInfinity(scip, -lhs) ? "" : "_rhs", "=l=",
            nlinvars, linvars, lincoeffs,
            nquadvarterms, quadvarterms,
            nbilinterms, bilinterms, rhs, transformed) );
      }
   }
   
   return SCIP_OKAY;
}

/** check GAMS limitations on SOC constraints
 * returns true of constraint can be written as conic equation in GAMS (using equation type =C=)
 */
static
SCIP_Bool isGAMSprintableSOC(
   int                   nlhsvars,           /**< number of variables on left hand side */
   SCIP_VAR**            lhsvars,            /**< variables on left hand side */
   SCIP_Real*            lhscoeffs,          /**< coefficients of variables on left hand side, or NULL if == 1.0 */
   SCIP_Real*            lhsoffsets,         /**< offsets of variables on left hand side, or NULL if == 0.0 */
   SCIP_Real             lhsconstant,        /**< constant on left hand side */
   SCIP_VAR*             rhsvar,             /**< variable on right hand side */
   SCIP_Real             rhscoef,            /**< coefficient of variable on right hand side */
   SCIP_Real             rhsoffset           /**< offset of variable on right hand side */
   )
{
   int i;

   assert(nlhsvars == 0 || lhsvars != NULL);

   if (rhscoef != 1.0)
      return FALSE;

   if (rhsoffset != 0.0)
      return FALSE;

   if( rhsvar == NULL )
      return FALSE;

   if( !SCIPvarIsActive(rhsvar) )
      return FALSE;

   if( lhsconstant != 0.0 )
      return FALSE;

   if( nlhsvars < 2 )
      return FALSE;

   for( i = 0; i < nlhsvars; ++i )
   {
      if( lhscoeffs [i] != 1.0 )
         return FALSE;

      if( lhsoffsets[i] != 0.0 )
         return FALSE;

      if( !SCIPvarIsActive(lhsvars[i]) )
         return FALSE;
   }

   return TRUE;
}

/* print second order cone row in GAMS format to file stream (performing retransformation to active variables)
 * The constraints are of the following form:
 * \f[
 *    \left\{ x \;:\; \sqrt{\gamma + \sum_{i=1}^{n} (\alpha_i\, (x_i + \beta_i))^2} \leq \alpha_{n+1}\, (x_{n+1}+\beta_{n+1}) \right\}.
 * \f]
 * */
static
SCIP_RETCODE printSOCCons(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           rowname,            /**< row name */
   int                   nlhsvars,           /**< number of variables on left hand side */
   SCIP_VAR**            lhsvars,            /**< variables on left hand side */
   SCIP_Real*            lhscoeffs,          /**< coefficients of variables on left hand side, or NULL if == 1.0 */
   SCIP_Real*            lhsoffsets,         /**< offsets of variables on left hand side, or NULL if == 0.0 */
   SCIP_Real             lhsconstant,        /**< constant on left hand side */
   SCIP_VAR*             rhsvar,             /**< variable on right hand side */
   SCIP_Real             rhscoef,            /**< coefficient of variable on right hand side */
   SCIP_Real             rhsoffset,          /**< offset of variable on right hand side */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   char linebuffer[GMS_MAX_PRINTLEN] = { '\0' };
   int linecnt;

   char consname[GMS_MAX_NAMELEN + 3]; /* four extra characters for ' ..' */
   char buffer[GMS_MAX_PRINTLEN];

   assert( scip != NULL );
   assert( strlen(rowname) > 0 );
   assert( nlhsvars == 0 || lhsvars != NULL );

   clearLine(linebuffer, &linecnt);

   /* start each line with a space */
   appendLine(scip, file, linebuffer, &linecnt, " ");

   /* print row name */
   (void) SCIPsnprintf(buffer, GMS_MAX_NAMELEN + 3, "%s ..", rowname);
   SCIP_CALL( printConformName(scip, consname, GMS_MAX_NAMELEN + 3, buffer) );

   appendLine(scip, file, linebuffer, &linecnt, consname);

   if( !isGAMSprintableSOC(nlhsvars, lhsvars, lhscoeffs, lhsoffsets, lhsconstant, rhsvar, rhscoef, rhsoffset) )
   {
      int t;

      /* print right-hand side on left */
      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "sqr(%.15g +", rhsoffset);

      SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, buffer, ")", 1, &rhsvar, &rhscoef, transformed) );

      appendLine(scip, file, linebuffer, &linecnt, " =g= ");

      /* print left-hand side on right */

      if( lhsconstant != 0.0 )
      {
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%.15g", lhsconstant);

         appendLine(scip, file, linebuffer, &linecnt, buffer);
      }

      for( t = 0; t < nlhsvars; ++t )
      {
         assert( lhsvars[t] != NULL );

         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "+ sqr(%.15g * (%.15g + ", lhscoeffs ? lhscoeffs[t] : 1.0, lhsoffsets ? lhsoffsets[t] : 0.0);

         SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, buffer, "))", 1, &lhsvars[t], NULL, transformed) );
      }
   }
   else
   {
      /* print right-hand side on left */
      SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", " ", 1, &rhsvar, &rhscoef, transformed) );

      appendLine(scip, file, linebuffer, &linecnt, " =c= ");

      /* print left-hand side on right */
      SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", " ", nlhsvars, lhsvars, lhscoeffs, transformed) );
   }

   appendLine(scip, file, linebuffer, &linecnt, ";");

   endLine(scip, file, linebuffer, &linecnt);

   return SCIP_OKAY;
}

#ifdef WITH_SIGNPOWER
/* print signpower row in GAMS format to file stream (performing retransformation to active variables) */
static
SCIP_RETCODE printSignpowerRow(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           rowname,            /**< row name */
   const char*           rownameextension,   /**< row name extension */
   const char*           type,               /**< row type ("=e=", "=l=", or "=g=") */
   SCIP_VAR*             nonlinvar,          /**< nonlinear variable */
   SCIP_VAR*             linvar,             /**< linear variable, may be NULL */
   SCIP_Real             exponent,           /**< exponent of nonlinear variable */
   SCIP_Real             offset,             /**< offset of nonlinear variable */
   SCIP_Real             coeflinear,         /**< coefficient of linear variable */
   SCIP_Real             lhs,                /**< left hand side */
   SCIP_Real             rhs,                /**< right hand side */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   char linebuffer[GMS_MAX_PRINTLEN] = { '\0' };
   int linecnt;
   SCIP_Bool nisoddint;

   char consname[GMS_MAX_NAMELEN + 3]; /* four extra characters for ' ..' */
   char buffer[GMS_MAX_PRINTLEN];

   assert( scip != NULL );
   assert( strlen(rowname) > 0 || strlen(rownameextension) > 0 );
   assert( strcmp(type, "=e=") == 0 || strcmp(type, "=l=") == 0 || strcmp(type, "=g=") == 0 );
   assert( nonlinvar != NULL );
   assert( exponent > 1.0 );

   clearLine(linebuffer, &linecnt);

   /* start each line with a space */
   appendLine(scip, file, linebuffer, &linecnt, " ");

   /* print row name */
   (void) SCIPsnprintf(buffer, GMS_MAX_NAMELEN + 3, "%s%s ..", rowname, rownameextension);
   SCIP_CALL( printConformName(scip, consname, GMS_MAX_NAMELEN + 3, buffer) );

   appendLine(scip, file, linebuffer, &linecnt, consname);

   /* print nonlinear term
    * signpow(x,n) is printed as x*abs(x) if n == 2, x*power(abs(x),n-1) if n is not 2 and not an odd integer, and as intpower(x,n) if n is an odd integer
    */
   nisoddint = SCIPisIntegral(scip, exponent) && ((int)SCIPfloor(scip, exponent+0.5))%2 == 1;
   if( !nisoddint )
   {
      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "(%g ", offset);
      appendLine(scip, file, linebuffer, &linecnt, buffer);
      SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", ") * ", 1, &nonlinvar, NULL, transformed) );

      if( exponent == 2.0)
      {
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "abs(%g ", offset);
         appendLine(scip, file, linebuffer, &linecnt, buffer);
         SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", ")", 1, &nonlinvar, NULL, transformed) );
      }
      else
      {
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "power(abs(%g ", offset);
         appendLine(scip, file, linebuffer, &linecnt, buffer);
         SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", ")", 1, &nonlinvar, NULL, transformed) );
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, ",%g)", exponent-1.0);
         appendLine(scip, file, linebuffer, &linecnt, buffer);
      }
   }
   else
   {
      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "intpower(%g ", offset);
      appendLine(scip, file, linebuffer, &linecnt, buffer);
      SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", ",", 1, &nonlinvar, NULL, transformed) );
      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%g)", exponent);
      appendLine(scip, file, linebuffer, &linecnt, buffer);
   }

   /* print linear term */
   if( linvar != NULL )
   {
      SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "+", "", 1, &linvar, &coeflinear, transformed) );
   }

   /* print right hand side */
   if( linecnt == 0 )
      /* we start a new line; therefore we tab this line */
      appendLine(scip, file, linebuffer, &linecnt, "     ");

   if( SCIPisZero(scip, rhs) )
      rhs = 0.0;

   (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s %.15g;", type, rhs);

   appendLine(scip, file, linebuffer, &linecnt, buffer);

   endLine(scip, file, linebuffer, &linecnt);

   return SCIP_OKAY;
}

/* print signpower row in GAMS format to file stream (performing retransformation to active variables)
 * */
static
SCIP_RETCODE printSignpowerCons(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           rowname,            /**< row name */
   SCIP_VAR*             nonlinvar,          /**< nonlinear variable */
   SCIP_VAR*             linvar,             /**< linear variable, may be NULL */
   SCIP_Real             exponent,           /**< exponent of nonlinear variable */
   SCIP_Real             offset,             /**< offset of nonlinear variable */
   SCIP_Real             coeflinear,         /**< coefficient of linear variable */
   SCIP_Real             lhs,                /**< left hand side */
   SCIP_Real             rhs,                /**< right hand side */
   SCIP_Bool             transformed         /**< transformed constraint? */
   )
{
   assert( scip != NULL );
   assert( strlen(rowname) > 0 );

   /* print row(s) in GAMS format */
   if( SCIPisEQ(scip, lhs, rhs) )
   {
      assert( !SCIPisInfinity(scip, rhs) );

      /* print equality constraint */
      SCIP_CALL( printSignpowerRow(scip, file, rowname, "", "=e=",
         nonlinvar, linvar, exponent, offset, coeflinear, lhs, rhs, transformed) );
   }
   else
   {
      if( !SCIPisInfinity(scip, -lhs) )
      {
         /* print inequality ">=" */
         SCIP_CALL( printSignpowerRow(scip, file, rowname, SCIPisInfinity(scip, rhs) ? "" : "_lhs", "=g=",
            nonlinvar, linvar, exponent, offset, coeflinear, lhs, rhs, transformed) );
      }
      if( !SCIPisInfinity(scip, rhs) )
      {
         /* print inequality "<=" */
         SCIP_CALL( printSignpowerRow(scip, file, rowname, SCIPisInfinity(scip, -lhs) ? "" : "_rhs", "=l=",
            nonlinvar, linvar, exponent, offset, coeflinear, lhs, rhs, transformed) );
      }
   }

   return SCIP_OKAY;
}
#endif

/** method check if the variable names are not longer than GMS_MAX_NAMELEN */
static
SCIP_RETCODE checkVarnames(
   SCIP*              scip,               /**< SCIP data structure */
   SCIP_VAR**         vars,               /**< array of variables */
   int                nvars               /**< number of variables */
   )
{
   int v;
   SCIP_VAR* var;
   SCIP_Bool replaceforbiddenchars;
   const char* badchar;

   assert( scip != NULL );
   assert( vars != NULL );

   SCIP_CALL( SCIPgetBoolParam(scip, "reading/gmsreader/replaceforbiddenchars", &replaceforbiddenchars) );

   /* check if the variable names contain the symbols '#', '*', '+', '/', '-', or '@' */
   for( badchar = badchars; *badchar; ++badchar )
   {
      for( v = 0; v < nvars; ++v )
      {
         var = vars[v];
         assert( var != NULL );

         if( strchr(SCIPvarGetName(var), *badchar) != NULL )
         {
            if( replaceforbiddenchars )
            {
               SCIPwarningMessage("there is a variable name with symbol '%c', not allowed in GAMS format; all '%c' replaced by '_' (consider using 'write genproblem'/'write gentransproblem').\n", *badchar, *badchar);
            }
            else
            {
               SCIPwarningMessage("there is a variable name with symbol '%c', not allowed in GAMS format; use 'write genproblem'/'write gentransproblem', or set 'reading/gmsreader/replaceforbiddenchars' to TRUE and risk duplicate variable names.\n", *badchar);
            }

            break;
         }
      }
   }

   /* check if the variable names are too long */
   for( v = 0; v < nvars; ++v )
   {
      var = vars[v];
      assert( var != NULL );

      if( strlen(SCIPvarGetName(var)) > GMS_MAX_NAMELEN )
      {
         SCIPwarningMessage("there is a variable name which has to be cut down to %d characters; GAMS model might be corrupted\n", 
            GMS_MAX_NAMELEN - 1);
         break;
      }
   }

   return SCIP_OKAY;
}

/** method check if the constraint names are not longer than GMS_MAX_NAMELEN */
static
SCIP_RETCODE checkConsnames(
   SCIP*              scip,               /**< SCIP data structure */
   SCIP_CONS**        conss,              /**< array of constraints */
   int                nconss,             /**< number of constraints */
   SCIP_Bool          transformed         /**< TRUE iff problem is the transformed problem */
   )
{
   int c;
   SCIP_CONS* cons;
   SCIP_CONSHDLR* conshdlr;
   const char* conshdlrname;
   SCIP_Bool replaceforbiddenchars;
   const char* badchar;

   assert( scip != NULL );
   assert( conss != NULL );

   SCIP_CALL( SCIPgetBoolParam(scip, "reading/gmsreader/replaceforbiddenchars", &replaceforbiddenchars) );

   /* check if the constraint names contain the symbol '#', '*', '+', '/', '-', or '@' */
   for( badchar = badchars; *badchar; ++badchar )
   {
      for( c = 0; c < nconss; ++c )
      {
         cons = conss[c];
         assert( cons != NULL );

         if( strchr(SCIPconsGetName(cons), *badchar) != NULL )
         {
            if( replaceforbiddenchars )
            {
               SCIPwarningMessage("there is a constraint name with symbol '%c', not allowed in GAMS format; all '%c' replaced by '_' (consider using 'write genproblem'/'write gentransproblem').\n", *badchar, *badchar);
            }
            else
            {
               SCIPwarningMessage("there is a constraint name with symbol '%c', not allowed in GAMS format; use 'write genproblem'/'write gentransproblem', or set 'reading/gmsreader/replaceforbiddenchars' to TRUE and risk duplicate variable names.\n", *badchar);
            }

            break;
         }
      }
   }

   /* check if the constraint names are too long */
   for( c = 0; c < nconss; ++c )
   {
      cons = conss[c];
      assert( cons != NULL );

      /* in case the transformed is written, only constraints are posted which are enabled in the current node */
      assert(!transformed || SCIPconsIsEnabled(cons));

      conshdlr = SCIPconsGetHdlr(cons);
      assert( conshdlr != NULL );

      conshdlrname = SCIPconshdlrGetName(conshdlr);
      assert( transformed == SCIPconsIsTransformed(cons) );

      if( strcmp(conshdlrname, "linear") == 0 || strcmp(conshdlrname, "quadratic") == 0 )
      {
         SCIP_Real lhs = strcmp(conshdlrname, "linear") == 0 ? SCIPgetLhsLinear(scip, cons) : SCIPgetLhsQuadratic(scip, cons);
         SCIP_Real rhs = strcmp(conshdlrname, "linear") == 0 ? SCIPgetLhsLinear(scip, cons) : SCIPgetRhsQuadratic(scip, cons);

         if( SCIPisEQ(scip, lhs, rhs) && strlen(SCIPconsGetName(conss[c])) > GMS_MAX_NAMELEN )
         {
            SCIPwarningMessage("there is a constraint name which has to be cut down to %d characters;\n",
               GMS_MAX_NAMELEN - 1);
            break;
         }
         else if( !SCIPisEQ(scip, lhs, rhs) && strlen(SCIPconsGetName(conss[c])) > GMS_MAX_NAMELEN - 4 )
         {
            SCIPwarningMessage("there is a constraint name which has to be cut down to %d characters;\n",
               GMS_MAX_NAMELEN - 5);
            break;
         }
      }
      else if( strlen(SCIPconsGetName(conss[c])) > GMS_MAX_NAMELEN )
      {
         SCIPwarningMessage("there is a constraint name which has to be cut down to %d characters;\n",
            GMS_MAX_NAMELEN - 1);
         break;
      }
   }
   return SCIP_OKAY;
}



/*
 * Callback methods of reader
 */

/** copy method for reader plugins (called when SCIP copies plugins) */
static
SCIP_DECL_READERCOPY(readerCopyGms)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(reader != NULL);
   assert(strcmp(SCIPreaderGetName(reader), READER_NAME) == 0);

   /* call inclusion method of reader */
   SCIP_CALL( SCIPincludeReaderGms(scip) );
 
   return SCIP_OKAY;
}


/** destructor of reader to free user data (called when SCIP is exiting) */
#define readerFreeGms NULL

/** problem reading method of reader */
#define readerReadGms NULL

/** problem writing method of reader */
static
SCIP_DECL_READERWRITE(readerWriteGms)
{  /*lint --e{715}*/
   SCIP_CALL( SCIPwriteGms(scip, file, name, transformed, objsense, objscale, objoffset, vars,
         nvars, nbinvars, nintvars, nimplvars, ncontvars, conss, nconss, result) );

   return SCIP_OKAY;
}



/*
 * reader specific interface methods
 */

/** includes the gms file reader in SCIP */
SCIP_RETCODE SCIPincludeReaderGms(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_READERDATA* readerdata;

   /* create gms reader data */
   readerdata = NULL;

   /* include gms reader */
   SCIP_CALL( SCIPincludeReader(scip, READER_NAME, READER_DESC, READER_EXTENSION,
         readerCopyGms,
         readerFreeGms, readerReadGms, readerWriteGms, 
         readerdata) );

   /* add gms reader parameters for writing routines*/
   SCIP_CALL( SCIPaddBoolParam(scip,
         "reading/gmsreader/freeints", "have integer variables no upper bound by default (depending on GAMS version)?",
         NULL, FALSE, FALSE, NULL, NULL) );

   SCIP_CALL( SCIPaddBoolParam(scip,
         "reading/gmsreader/replaceforbiddenchars", "shall characters '#', '*', '+', '/', and '-' in variable and constraint names be replaced by '_'?",
         NULL, FALSE, FALSE, NULL, NULL) );

   return SCIP_OKAY;
}



/* writes problem to gms file */
SCIP_RETCODE SCIPwriteGms(
   SCIP*              scip,               /**< SCIP data structure */
   FILE*              file,               /**< output file, or NULL if standard output should be used */
   const char*        name,               /**< problem name */
   SCIP_Bool          transformed,        /**< TRUE iff problem is the transformed problem */
   SCIP_OBJSENSE      objsense,           /**< objective sense */
   SCIP_Real          objscale,           /**< scalar applied to objective function; external objective value is
                                           *   extobj = objsense * objscale * (intobj + objoffset)
                                           */
   SCIP_Real          objoffset,          /**< objective offset from bound shifting and fixing */
   SCIP_VAR**         vars,               /**< array with active variables ordered binary, integer, implicit, continuous */
   int                nvars,              /**< number of mutable variables in the problem */
   int                nbinvars,           /**< number of binary variables */
   int                nintvars,           /**< number of general integer variables */
   int                nimplvars,          /**< number of implicit integer variables */
   int                ncontvars,          /**< number of continuous variables */
   SCIP_CONS**        conss,              /**< array with constraints of the problem */
   int                nconss,             /**< number of constraints in the problem */
   SCIP_RESULT*       result              /**< pointer to store the result of the file writing call */
   )
{
   int c,v;

   int linecnt;
   char linebuffer[GMS_MAX_PRINTLEN];

   char varname[GMS_MAX_NAMELEN];
   char buffer[GMS_MAX_PRINTLEN];

   SCIP_Real* objcoeffs;

   SCIP_CONSHDLR* conshdlr;
   const char* conshdlrname;
   SCIP_CONS* cons;

   char consname[GMS_MAX_NAMELEN];

   SCIP_VAR** consvars;
   SCIP_Real* consvals;
   int nconsvars;

   SCIP_VAR* var;
   SCIP_Real lb;
   SCIP_Real ub;
   SCIP_Bool freeints;
   SCIP_Bool nondefbounds;
   SCIP_Bool nlcons;
   SCIP_Bool nqcons;
   SCIP_Bool rangedrow;

   assert( scip != NULL );
   assert( nvars > 0 );

   /* check if the variable names are not too long */
   SCIP_CALL( checkVarnames(scip, vars, nvars) );
   /* check if the constraint names are too long */
   SCIP_CALL( checkConsnames(scip, conss, nconss, transformed) );

   /* print statistics as comment to file */
   SCIPinfoMessage(scip, file, "* SCIP STATISTICS\n");
   SCIPinfoMessage(scip, file, "*   Problem name     : %s\n", name);
   SCIPinfoMessage(scip, file, "*   Variables        : %d (%d binary, %d integer, %d implicit integer, %d continuous)\n",
      nvars, nbinvars, nintvars, nimplvars, ncontvars);
   SCIPinfoMessage(scip, file, "*   Constraints      : %d\n", nconss);
   SCIPinfoMessage(scip, file, "*   Obj. scale       : %.15g\n", objscale);
   SCIPinfoMessage(scip, file, "*   Obj. offset      : %.15g\n\n", objoffset);

   /* print flags */
   SCIPinfoMessage(scip, file, "$MAXCOL %d\n", GMS_MAX_LINELEN - 1);
   SCIPinfoMessage(scip, file, "$OFFDIGIT\n\n");

   /* print variable section */
   SCIPinfoMessage(scip, file, "Variables\n");
   clearLine(linebuffer, &linecnt);

   /* auxiliary objective variable */
   SCIPinfoMessage(scip, file, " objvar,\n");

   /* "model" variables */
   for( v = 0; v < nvars; ++v )
   {
      var = vars[v];
      assert( var != NULL );

      SCIP_CALL( printConformName(scip, varname, GMS_MAX_NAMELEN, SCIPvarGetName(var)) );
      (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " %s%s", varname, (v < nvars - 1) ? "," : ";");
      appendLine(scip, file, linebuffer, &linecnt, buffer);

      if( (linecnt > 0 && (v == nbinvars - 1 || v == nbinvars + nintvars - 1 ||
            v == nbinvars + nintvars + nimplvars - 1)) || v == nvars - 1 )
      {
         endLine(scip, file, linebuffer, &linecnt);
         clearLine(linebuffer, &linecnt);
      }
   }

   SCIPinfoMessage(scip, file, "\n");

   /* declare binary variables if present */
   if( nbinvars > 0 )
   {
      SCIPinfoMessage(scip, file, "Binary variables\n");
      clearLine(linebuffer, &linecnt);
   
      for( v = 0; v < nbinvars; ++v )
      {
         var = vars[v];

         SCIP_CALL( printConformName(scip, varname, GMS_MAX_NAMELEN, SCIPvarGetName(var)) );
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " %s%s", varname, (v < nbinvars - 1) ? "," : ";");

         appendLine(scip, file, linebuffer, &linecnt, buffer);
      }

      endLine(scip, file, linebuffer, &linecnt);
      SCIPinfoMessage(scip, file, "\n");
   }

   /* declare integer variables if present */
   if( nintvars > 0 )
   {
      SCIPinfoMessage(scip, file, "Integer variables\n");
      clearLine(linebuffer, &linecnt);
   
      for( v = 0; v < nintvars; ++v )
      {
         var = vars[nbinvars + v];

         SCIP_CALL( printConformName(scip, varname, GMS_MAX_NAMELEN, SCIPvarGetName(var)) );
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " %s%s", varname, (v < nintvars - 1) ? "," : ";");

         appendLine(scip, file, linebuffer, &linecnt, buffer);
      }

      endLine(scip, file, linebuffer, &linecnt);
      SCIPinfoMessage(scip, file, "\n");
   }

   /* print variable bounds */
   SCIPinfoMessage(scip, file, "* Variable bounds\n");
   SCIP_CALL( SCIPgetBoolParam(scip, "reading/gmsreader/freeints", &freeints) );
   nondefbounds = FALSE;

   for( v = 0; v < nvars; ++v )
   {
      var = vars[v];
      assert( var != NULL );

      SCIP_CALL( printConformName(scip, varname, GMS_MAX_NAMELEN, SCIPvarGetName(var)) );

      if( transformed )
      {
         /* in case the transformed is written only local bounds are posted which are valid in the current node */
         lb = SCIPvarGetLbLocal(var);
         ub = SCIPvarGetUbLocal(var);
      }
      else
      {
         lb = SCIPvarGetLbOriginal(var);
         ub = SCIPvarGetUbOriginal(var);
      }
      assert( lb <= ub );

      /* fixed */
      if( SCIPisEQ(scip, lb, ub) )
      {
         if( v < nintvars )
            SCIPinfoMessage(scip, file, " %s.fx = %g;\n", varname, SCIPfloor(scip, lb + 0.5));
         else
            SCIPinfoMessage(scip, file, " %s.fx = %.15g;\n", varname, lb);
         nondefbounds = TRUE;

         /* no need to write lower and upper bounds additionally */
         continue;
      }

      /* lower bound */
      if( v < nbinvars + nintvars )
      {
         /* default lower bound of binaries and integers is 0 (also in recent gams versions if pf4=0 is given) */
         if( !SCIPisZero(scip, lb) )
         {
            if( !SCIPisInfinity(scip, -lb) )
               SCIPinfoMessage(scip, file, " %s.lo = %g;\n", varname, SCIPceil(scip, lb));
            else
               SCIPinfoMessage(scip, file, " %s.lo = %g;\n", varname, -SCIPinfinity(scip)); /* sorry, -inf not allowed in gams file here */
            nondefbounds = TRUE;
         }
      }
      else if( v >= nbinvars + nintvars && !SCIPisInfinity(scip, -lb) )
      {
         /* continuous variables are free by default */
         SCIPinfoMessage(scip, file, " %s.lo = %.15g;\n", varname, lb);
         nondefbounds = TRUE;
      }

      /* upper bound */
      if( v < nbinvars )
      {
         if( !SCIPisEQ(scip, ub, 1.0) )
         {
            SCIPinfoMessage(scip, file, " %s.up = %g;\n", varname, SCIPfloor(scip, ub));
            nondefbounds = TRUE;
         }
      }
      else if( v < nbinvars + nintvars && !freeints )
      {
         /* freeints == FALSE: integer variables have upper bound 100 by default */
         if( !SCIPisEQ(scip, ub, 100.0) )
         {
            if( !SCIPisInfinity(scip, ub) )
               SCIPinfoMessage(scip, file, " %s.up = %g;\n", varname, SCIPfloor(scip, ub));
            else
               SCIPinfoMessage(scip, file, " %s.up = %g;\n", varname, SCIPinfinity(scip)); /* sorry, +inf not allowed in gams file here */
            nondefbounds = TRUE;
         }
      }
      else if( v < nbinvars + nintvars && !SCIPisInfinity(scip, ub) )
      {
         /* freeints == TRUE: integer variables have no upper bound by default */
         SCIPinfoMessage(scip, file, " %s.up = %g;\n", varname, SCIPfloor(scip, ub));
         nondefbounds = TRUE;
      }
      else if( v >= nbinvars + nintvars && !SCIPisInfinity(scip, ub) )
      {
         /* continuous variables are free by default */
         SCIPinfoMessage(scip, file, " %s.up = %.15g;\n", varname, ub);
         nondefbounds = TRUE;
      }
   }

   if( !nondefbounds )
      SCIPinfoMessage(scip, file, "* (All other bounds at default value: binary [0,1], integer [%s], continuous [-inf,+inf].)\n", freeints ? "0,+inf" : "0,100");
   SCIPinfoMessage(scip, file, "\n");

   /* print equations section */
   SCIPinfoMessage(scip, file, "Equations\n");
   clearLine(linebuffer, &linecnt);

   SCIPinfoMessage(scip, file, " objequ%s\n", (nconss > 0) ? "," : ";");

   /* declare equations */
   for( c = 0; c < nconss; ++c )
   {
      cons = conss[c];
      assert( cons != NULL );

      conshdlr = SCIPconsGetHdlr(cons);
      assert( conshdlr != NULL );

      SCIP_CALL( printConformName(scip, consname, GMS_MAX_NAMELEN, SCIPconsGetName(cons)) );
      conshdlrname = SCIPconshdlrGetName(conshdlr);
      assert( transformed == SCIPconsIsTransformed(cons) );

      rangedrow = strcmp(conshdlrname, "linear") == 0
         && !SCIPisInfinity(scip, -SCIPgetLhsLinear(scip, cons)) && !SCIPisInfinity(scip, SCIPgetRhsLinear(scip, cons))
         && !SCIPisEQ(scip, SCIPgetLhsLinear(scip, cons), SCIPgetRhsLinear(scip, cons));
      rangedrow = rangedrow || (strcmp(conshdlrname, "quadratic") == 0
         && !SCIPisInfinity(scip, -SCIPgetLhsQuadratic(scip, cons)) && !SCIPisInfinity(scip, SCIPgetRhsQuadratic(scip, cons))
         && !SCIPisEQ(scip, SCIPgetLhsQuadratic(scip, cons), SCIPgetRhsQuadratic(scip, cons)));
#ifdef WITH_SIGNPOWER
      rangedrow = rangedrow || (strcmp(conshdlrname, "signpower") == 0
         && !SCIPisInfinity(scip, -SCIPgetLhsSignpower(scip, cons)) && !SCIPisInfinity(scip, SCIPgetRhsSignpower(scip, cons))
         && !SCIPisEQ(scip, SCIPgetLhsSignpower(scip, cons), SCIPgetRhsSignpower(scip, cons)));
#endif
      rangedrow = rangedrow || (strcmp(conshdlrname, "varbound") == 0
         && !SCIPisInfinity(scip, -SCIPgetLhsVarbound(scip, cons)) && !SCIPisInfinity(scip, SCIPgetRhsVarbound(scip, cons))
         && !SCIPisEQ(scip, SCIPgetLhsVarbound(scip, cons), SCIPgetRhsVarbound(scip, cons)));

      /* we declare only those constraints which we can print in GAMS format */
      SCIP_CALL( printConformName(scip, consname, GMS_MAX_NAMELEN, SCIPconsGetName(cons)) );
      if( rangedrow )
      {
         assert( strcmp(conshdlrname, "linear") == 0 || strcmp(conshdlrname, "knapsack") == 0 || strcmp(conshdlrname, "varbound") == 0 );

         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " %s%s%s%s%s", consname, "_lhs, ", consname, "_rhs", (c < nconss - 1) ? "," : ";");
         appendLine(scip, file, linebuffer, &linecnt, buffer);
      }
      else if( strcmp(conshdlrname, "knapsack") == 0 || strcmp(conshdlrname, "logicor") == 0 || strcmp(conshdlrname, "setppc") == 0
            || strcmp(conshdlrname, "linear") == 0 || strcmp(conshdlrname, "quadratic") == 0 || strcmp(conshdlrname, "varbound") == 0
            || strcmp(conshdlrname, "soc") == 0 || strcmp(conshdlrname, "signpower") == 0 )
      {
         (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " %s%s", consname, (c < nconss - 1) ? "," : ";");
         appendLine(scip, file, linebuffer, &linecnt, buffer);
      }
      else
      {
         SCIPwarningMessage("Constraint type <%s> not supported. Skip writing constraint <%s>.\n", conshdlrname, consname);
         if( c == nconss - 1 )
            appendLine(scip, file, linebuffer, &linecnt, ";");
      }
   }
      
   endLine(scip, file, linebuffer, &linecnt);
   SCIPinfoMessage(scip, file, "\n");
   
   /* print objective function equation */
   clearLine(linebuffer, &linecnt);
   (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, " objequ .. objvar =e= ");
   appendLine(scip, file, linebuffer, &linecnt, buffer);

   SCIP_CALL( SCIPallocBufferArray(scip, &objcoeffs, nvars) );

   for( v = 0; v < nvars; ++v )
   {
      var = vars[v];
      assert( var != NULL );

#ifndef NDEBUG
      /* in case the original problem has to be posted the variables have to be either "original" or "negated" */
      if( !transformed )
         assert( SCIPvarGetStatus(var) == SCIP_VARSTATUS_ORIGINAL ||
            SCIPvarGetStatus(var) == SCIP_VARSTATUS_NEGATED );
#endif

      objcoeffs[v] = SCIPisZero(scip, SCIPvarGetObj(var)) ? 0.0 : SCIPvarGetObj(var);
   }

   SCIP_CALL( printActiveVariables(scip, file, linebuffer, &linecnt, "", ";", nvars, vars, objcoeffs, transformed) );

   SCIPfreeBufferArray(scip, &objcoeffs);
   endLine(scip, file, linebuffer, &linecnt);
   SCIPinfoMessage(scip, file, "\n");

   /* print constraints */
   nlcons = FALSE;
   nqcons = FALSE;
   for( c = 0; c < nconss; ++c )
   {
      cons = conss[c];
      assert( cons != NULL );

      /* in case the transformed is written, only constraints are posted which are enabled in the current node */
      assert(!transformed || SCIPconsIsEnabled(cons));
      
      conshdlr = SCIPconsGetHdlr(cons);
      assert( conshdlr != NULL );

      SCIP_CALL( printConformName(scip, consname, GMS_MAX_NAMELEN, SCIPconsGetName(cons)) );
      conshdlrname = SCIPconshdlrGetName(conshdlr);
      assert( transformed == SCIPconsIsTransformed(cons) );

      if( strcmp(conshdlrname, "knapsack") == 0 )
      {
         SCIP_Longint* weights;

         consvars = SCIPgetVarsKnapsack(scip, cons);
         nconsvars = SCIPgetNVarsKnapsack(scip, cons);

         /* copy Longint array to SCIP_Real array */
         weights = SCIPgetWeightsKnapsack(scip, cons);
         SCIP_CALL( SCIPallocBufferArray(scip, &consvals, nconsvars) );
         for( v = 0; v < nconsvars; ++v )
            consvals[v] = weights[v];

         SCIP_CALL( printLinearCons(scip, file, consname, nconsvars, consvars, consvals,
               -SCIPinfinity(scip), (SCIP_Real) SCIPgetCapacityKnapsack(scip, cons), transformed) );

         SCIPfreeBufferArray(scip, &consvals);
      }
      else if( strcmp(conshdlrname, "linear") == 0 )
      {
         SCIP_CALL( printLinearCons(scip, file, consname,
               SCIPgetNVarsLinear(scip, cons), SCIPgetVarsLinear(scip, cons), SCIPgetValsLinear(scip, cons),
               SCIPgetLhsLinear(scip, cons),  SCIPgetRhsLinear(scip, cons), transformed) );
      }
      else if( strcmp(conshdlrname, "logicor") == 0 )
      {
         SCIP_CALL( printLinearCons(scip, file, consname,
               SCIPgetNVarsLogicor(scip, cons), SCIPgetVarsLogicor(scip, cons), NULL,
               1.0, SCIPinfinity(scip), transformed) );
      }
      else if( strcmp(conshdlrname, "quadratic") == 0 )
      {
         SCIP_CALL( printQuadraticCons(scip, file, consname,
               SCIPgetNLinearVarsQuadratic(scip, cons), SCIPgetLinearVarsQuadratic(scip, cons), SCIPgetCoefsLinearVarsQuadratic(scip, cons),
               SCIPgetNQuadVarTermsQuadratic(scip, cons), SCIPgetQuadVarTermsQuadratic(scip, cons),
               SCIPgetNBilinTermsQuadratic(scip, cons), SCIPgetBilinTermsQuadratic(scip, cons),
               SCIPgetLhsQuadratic(scip, cons),  SCIPgetRhsQuadratic(scip, cons), transformed) );

         nlcons = TRUE;
      }
      else if( strcmp(conshdlrname, "setppc") == 0 )
      {
         consvars = SCIPgetVarsSetppc(scip, cons);
         nconsvars = SCIPgetNVarsSetppc(scip, cons);

         switch ( SCIPgetTypeSetppc(scip, cons) )
         {
         case SCIP_SETPPCTYPE_PARTITIONING :
            SCIP_CALL( printLinearCons(scip, file, consname,
                  nconsvars, consvars, NULL, 1.0, 1.0, transformed) );
            break;
         case SCIP_SETPPCTYPE_PACKING :
            SCIP_CALL( printLinearCons(scip, file, consname,
                  nconsvars, consvars, NULL, -SCIPinfinity(scip), 1.0, transformed) );
            break;
         case SCIP_SETPPCTYPE_COVERING :
            SCIP_CALL( printLinearCons(scip, file, consname,
                  nconsvars, consvars, NULL, 1.0, SCIPinfinity(scip), transformed) );
            break;
         }
      }
      else if( strcmp(conshdlrname, "varbound") == 0 )
      {
         SCIP_CALL( SCIPallocBufferArray(scip, &consvars, 2) );
         SCIP_CALL( SCIPallocBufferArray(scip, &consvals, 2) );

         consvars[0] = SCIPgetVarVarbound(scip, cons);
         consvars[1] = SCIPgetVbdvarVarbound(scip, cons);

         consvals[0] = 1.0;
         consvals[1] = SCIPgetVbdcoefVarbound(scip, cons);

         SCIP_CALL( printLinearCons(scip, file, consname,
               2, consvars, consvals,
               SCIPgetLhsVarbound(scip, cons), SCIPgetRhsVarbound(scip, cons), transformed) );

         SCIPfreeBufferArray(scip, &consvars);
         SCIPfreeBufferArray(scip, &consvals);
      }
      else if( strcmp(conshdlrname, "soc") == 0 )
      {
         SCIP_CALL( printSOCCons(scip, file, consname,
            SCIPgetNLhsVarsSOC(scip, cons), SCIPgetLhsVarsSOC(scip, cons), SCIPgetLhsCoefsSOC(scip, cons), SCIPgetLhsOffsetsSOC(scip, cons), SCIPgetLhsConstantSOC(scip, cons),
            SCIPgetRhsVarSOC(scip, cons), SCIPgetRhsCoefSOC(scip, cons), SCIPgetRhsOffsetSOC(scip, cons), transformed) );
         
         nlcons = nlcons || !isGAMSprintableSOC(SCIPgetNLhsVarsSOC(scip, cons), SCIPgetLhsVarsSOC(scip, cons), SCIPgetLhsCoefsSOC(scip, cons), SCIPgetLhsOffsetsSOC(scip, cons), SCIPgetLhsConstantSOC(scip, cons),
            SCIPgetRhsVarSOC(scip, cons), SCIPgetRhsCoefSOC(scip, cons), SCIPgetRhsOffsetSOC(scip, cons));
      }
#ifdef WITH_SIGNPOWER
      else if( strcmp(conshdlrname, "signpower") == 0 )
      {
         SCIP_CALL( printSignpowerCons(scip, file, consname,
            SCIPgetNonlinearVarSignpower(scip, cons), SCIPgetLinearVarSignpower(scip, cons),
            SCIPgetExponentSignpower(scip, cons), SCIPgetOffsetSignpower(scip, cons), SCIPgetCoefLinearSignpower(scip, cons),
            SCIPgetLhsSignpower(scip, cons),  SCIPgetRhsSignpower(scip, cons), transformed) );

         nlcons = TRUE;
         nqcons = TRUE;
      }
#endif
      else
      {
         SCIPwarningMessage("constraint handler <%s> can not print requested format\n", conshdlrname );
         SCIPinfoMessage(scip, file, "* ");
         SCIP_CALL( SCIPprintCons(scip, cons, file) );
         SCIPinfoMessage(scip, file, "\n");
      }

      SCIPinfoMessage(scip, file, "\n");
   }

   /* print model creation */
   SCIPinfoMessage(scip, file, "Model m / all /;\n\n");

   /* print solve command */
   (void) SCIPsnprintf(buffer, GMS_MAX_PRINTLEN, "%s%s",
         nbinvars + nintvars > 0 ? "MI" : "", nlcons ? (nqcons ? "NLP" : "QCP") : (nbinvars + nintvars > 0 ? "P" : "LP"));

   SCIPinfoMessage(scip, file, "$if not set %s $set %s %s\n", buffer, buffer, buffer);
   SCIPinfoMessage(scip, file, "Solve m using %%%s%% %simizing objvar;\n",
         buffer, objsense == SCIP_OBJSENSE_MINIMIZE ? "min" : "max");

   *result = SCIP_SUCCESS;

   return SCIP_OKAY;
}
