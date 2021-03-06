/*****
 ** ** Module Header ******************************************************* **
 ** 									     **
 **   Modules Revision 3.0						     **
 **   Providing a flexible user environment				     **
 ** 									     **
 **   File:		error.c						     **
 **   First Edition:	1991/10/23					     **
 ** 									     **
 **   Authors:	Jens Hamisch, jens@Strawberry.COM			     **
 **		R.K. Owen, <rk@owen.sj.ca.us> or <rkowen@nersc.gov>	     **
 ** 									     **
 **   Description:	The modules error logger			     **
 ** 									     **
 **   Exports:		Module_Error					     **
 **			GetFacilityPtr					     **
 **			CheckFacility					     **
 **			Enable_Error					     **
 **			Disable_Error					     **
 **			Restore_Error					     **
 **									     **
 **   Notes:								     **
 ** 									     **
 ** ************************************************************************ **
 ****/

/** ** Copyright *********************************************************** **
 ** 									     **
 ** Copyright 1991-1994 by John L. Furlan.                      	     **
 ** see LICENSE.GPL, which must be provided, for details		     **
 ** 									     ** 
 ** ************************************************************************ **/

static char Id[] = "@(#)$Id$";
static void *UseId[] = { &UseId, Id };

/** ************************************************************************ **/
/** 				      HEADERS				     **/
/** ************************************************************************ **/

#include "modules_def.h"
#if HAVE_STDARG_H
#  include <stdarg.h>
#else
#  error "You need an ANSI C compiler"
#endif

#if defined(HAVE_SYSLOG) && defined(WITH_LOGGING)
#  include <syslog.h>
#endif

#include <pwd.h>
#include <grp.h>

/** ************************************************************************ **/
/** 				  LOCAL DATATYPES			     **/
/** ************************************************************************ **/

/**
 **  Error weights
 **/

typedef enum _err_weights	{
    WGHT_NONE=0,			/** Dummy value: No error	     **/
    WGHT_VERBOSE,			/** Verbose messages		     **/
    WGHT_INFO=3,			/** Informal message	 	     **/
    WGHT_DEBUG=5,			/** Debugger output		     **/
    WGHT_WARN=10,			/** Warning: Prog. flow not affected **/
    WGHT_PROB=20,			/** Problem: Prog. flow cond. aff.   **/
    WGHT_ERROR=25,			/** Error and Fatal: Prog. flow aff. **/
    WGHT_FATAL=27,			/**   Return to the caller	     **/
    WGHT_PANIC=29			/** Panic: Exit program immediatelly **/
} ErrWeights;

/**
 **  Log facilities
 **/

typedef	struct	_err_facility	{
    ErrWeights	 Weight;		/** Error weight 		     **/
    char	*facility;		/** Log facility		     **/
    char	*def_facility;		/** Default facility (facility undef)**/
} ErrFacilities;

typedef struct	_facil_names	{
    char 	*name;			/** Name of a facility		     **/
    int		 token;			/** Assigned token		     **/
} FacilityNames;

/**
 **  Error measurement table
 **/

typedef	struct	{
    ErrWeights	 error_weight;		/** The weight itself		     **/
    char	*message;		/** Message to be printed	     **/
    ErrCode	 ret_nov,		/** Return code			     **/
		 ret_adv,
		 ret_exp;
} ErrMeasr;

/**
 **  Error code translation table
 **/

typedef struct  { 
    ErrType	  error_type;		/** The error type as specified by   **/
					/** the caller			     **/
    ErrWeights    error_weight;		/** The weight of this error         **/
    char	 *messages;		/** List of messages to be printed   **/
} ErrTransTab;

/** ************************************************************************ **/
/** 				     CONSTANTS				     **/
/** ************************************************************************ **/

#define	ARGLIST_SIZE	10
#define	ERR_LINELEN	80		/** internal buffer size	     **/
#define ERR_BUFSIZE	4096		/** buffer for the whole error msg.  **/

/** ************************************************************************ **/
/**				      MACROS				     **/
/** ************************************************************************ **/

/** not applicable **/

/** ************************************************************************ **/
/** 				    LOCAL DATA				     **/
/** ************************************************************************ **/

static	char	module_name[] = __FILE__;

/**
 **  Local flags
 **/
static	int	quiet_on_error = 0;

/**
 **  Local strings
 **/

static	char	 buffer[ ERR_LINELEN];		/** Internal string buffer   **/
	char	*error_line = NULL;

/**
 **  Log facility table
 **/

static	char	_stderr[] = "stderr";
static	char	_stdout[] = "stdout";
static	char	_null[] = "null";
static	char	_none[] = "none";

static	ErrFacilities	Facilities[] = {
    { WGHT_NONE,	NULL,	NULL },
    { WGHT_VERBOSE,	NULL,	DEF_FACILITY_VERBOSE },
    { WGHT_INFO,	NULL,	DEF_FACILITY_INFO },
    { WGHT_DEBUG,	NULL,	DEF_FACILITY_DEBUG },
    { WGHT_WARN,	NULL,	DEF_FACILITY_WARN },
    { WGHT_PROB,	NULL,	DEF_FACILITY_PROB },
    { WGHT_ERROR,	NULL,	DEF_FACILITY_ERROR },
    { WGHT_FATAL,	NULL,	DEF_FACILITY_FATAL },
    { WGHT_PANIC,	NULL,	DEF_FACILITY_PANIC }
};

/**
 **  Syslog facility names
 **/

/* must be in alphabetic order */
static	FacilityNames	facility_names[] = {
#if defined(HAVE_SYSLOG) && defined(WITH_LOGGING)
    { "auth",		LOG_AUTHPRIV },
    { "cron",		LOG_CRON },
    { "daemon",		LOG_DAEMON },
    { "ftp",		LOG_FTP },
    { "kern",		LOG_KERN },
    { "local0",		LOG_LOCAL0 },
    { "local1",		LOG_LOCAL1 },
    { "local2",		LOG_LOCAL2 },
    { "local3",		LOG_LOCAL3 },
    { "local4",		LOG_LOCAL4 },
    { "local5",		LOG_LOCAL5 },
    { "local6",		LOG_LOCAL6 },
    { "local7",		LOG_LOCAL7 },
    { "lpr",		LOG_LPR },
    { "mail",		LOG_MAIL },
    { "news",		LOG_NEWS },
    { "user",		LOG_USER },
    { "uucp",		LOG_UUCP }
#else
    { "none",		0 }
#endif
};

/**
 **  Syslog level names
 **/

/* must be in alphabetic order */
static	FacilityNames	level_names[] = {
#if defined(HAVE_SYSLOG) && defined(WITH_LOGGING)
    { "alert",		LOG_ALERT },
    { "crit",		LOG_CRIT },
    { "debug",		LOG_DEBUG },
    { "emerg",		LOG_EMERG },
    { "err",		LOG_ERR },
    { "info",		LOG_INFO },
    { "notice",		LOG_NOTICE },
    { "warning",	LOG_WARNING },
#else
    { "none",		0 }
#endif
};

/**
 **  Error measurement table
 **  Take care that this list is sorted in ascending order concerning the error
 **  weights
 **/

#define	MEAS_VERB_NDX	1	/** Index of the 'VERBOSE' entry	    **/

/*					novice		advanced	expert*/
static	ErrMeasr	Measurements[] = {
	{ WGHT_NONE,	"",		OK,		OK,		OK },	
	{ WGHT_VERBOSE,	"VERB",		OK,		OK,		OK },
	{ WGHT_INFO,	"INFO",		OK,		OK,		OK },
	{ WGHT_DEBUG,	"DEBUG",	OK,		OK,		OK },
	{ WGHT_WARN,	"WARN",		PROBLEM,	WARN,		OK },
	{ WGHT_PROB,	"PROB",		ERROR,		PROBLEM,	OK },
	{ WGHT_ERROR,	"ERROR",	ERROR,		ERROR,		ERROR },
	{ WGHT_FATAL,	"FATAL",	FATAL,		FATAL,		FATAL },
	{ WGHT_PANIC,	"PANIC",	PANIC,		PANIC,		PANIC },
};

/**
 **  Error code translation table
 **  Take care that this list is sorted in ascending order concerning the error
 **  types.
 **  Error weights (WGHT_*)
 **	WARN	- warnings
 **	PROB	- problems (normally the modulecmd may be completed)
 **	ERROR	- errors (which normally leads to an unsuccessful end)
 **	FATAL	- fatal system errors
 **	PANIC	- very fatal system errors (internal program inconsistencies)
 **/

static	ErrTransTab	TransTab[] = {
    { NO_ERR,		WGHT_NONE,  NULL },
    { NO_ERR_VERBOSE,	WGHT_VERBOSE,  NULL },
    { ERR_PARAM,	WGHT_ERROR, N_("Parameter error concerning '$1'") },
    { ERR_USAGE,	WGHT_ERROR, N_("Usage is '$1+'") },
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_ARGSTOLONG,	WGHT_ERROR,
	N_("'$1': Arguments too long. Max. is '$2'") },
    { ERR_OPT_AMBIG,	WGHT_ERROR, N_("Option '$1' is ambiguous") },
    { ERR_OPT_NOARG,	WGHT_ERROR, N_("Option '$1' allows no argument") },
    { ERR_OPT_REQARG,	WGHT_ERROR, N_("Option '$1' requires an argument") },
    { ERR_OPT_UNKNOWN,	WGHT_ERROR, N_("Unrecognized option '$1'") },
    { ERR_OPT_ILL,	WGHT_ERROR, N_("Illegal option '$1'") },
    { ERR_OPT_INV,	WGHT_ERROR, N_("Invalid option '$1'") },
    { ERR_USERLVL,	WGHT_ERROR, N_("Undefined userlevel '$1'") },
    { ERR_GETOPT,	WGHT_FATAL, N_("getopt() failed") },
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_OPEN,		WGHT_ERROR, N_("Cannot open file '$1' for '$2'") },
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_POPEN,	WGHT_ERROR, N_("Cannot open pipe '$1' for '$2'") },
    { ERR_OPENDIR,	WGHT_ERROR,
	N_("Cannot open directory '$1' for reading") },
    { ERR_CHDIR,	WGHT_WARN,  N_("Cannot chdir to '$1' for '$2'") },
    { ERR_CLOSE,	WGHT_WARN,  N_("Cannot close file '$1'") },
    { ERR_PCLOSE,	WGHT_WARN,  N_("Cannot close pipe '$1'") },
    { ERR_CLOSEDIR,	WGHT_WARN,  N_("Cannot close directory '$1'") },
    { ERR_READ,		WGHT_ERROR, N_("Error while reading file '$1'") },
    { ERR_READDIR,	WGHT_ERROR, N_("Error while reading directory '$1'") },
    { ERR_WRITE,	WGHT_ERROR, N_("Error while writing file '$1'") },
    { ERR_SEEK,		WGHT_ERROR, N_("Seek error on file '$1'") },
    { ERR_FLUSH,	WGHT_WARN,  N_("Flush error on file '$1'") },
    { ERR_DUP,		WGHT_WARN,  N_("Cannot duplicate handle of file '$1'")},
    { ERR_DIRNAME,	WGHT_ERROR, N_("Cannot build directory name") },
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_NAMETOLONG,	WGHT_ERROR,
	N_("Requested directory name to long: dir='$1',file='$2'") },
    { ERR_DIRNOTFOUND,	WGHT_ERROR, N_("Directory '$1' not found") },
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_FILEINDIR,	WGHT_ERROR,
	N_("File '$1' not found in directory '$2'") },
    { ERR_NODIR,	WGHT_ERROR, N_("'$1' is not a directory") },
    { ERR_UNLINK,	WGHT_WARN,  N_("Cannot unlink '$1'") },
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_RENAME,	WGHT_PROB,  N_("Cannot rename '$1' to '$2'") },
    { ERR_ALLOC,	WGHT_FATAL, N_("Out of memory.") },
    { ERR_UVEC,		WGHT_FATAL, N_("General uvec error.") },
    { ERR_MHASH,	WGHT_FATAL, N_("General MHash error.") },
    { ERR_SOURCE,	WGHT_WARN,  N_("Error sourcing file '$1'") },
    { ERR_UNAME,	WGHT_FATAL, N_("'uname (2)' failed.") },
    { ERR_GETHOSTNAME,	WGHT_FATAL, N_("'gethostname (2)' failed.") },
    { ERR_GETDOMAINNAME,WGHT_FATAL, N_("'getdomainname (2)' failed.") },
    { ERR_STRING,	WGHT_FATAL, N_("string error") },
    { ERR_DISPLAY,	WGHT_ERROR, N_("Cannot open display") },
    { ERR_PARSE,	WGHT_ERROR, N_("Parse error") },
    { ERR_EXEC,		WGHT_ERROR, N_("Tcl command execution failed: $1") },
    { ERR_EXTRACT,	WGHT_ERROR, N_("Cannot extract X11 resources") },
    { ERR_COMMAND,	WGHT_ERROR, N_("'$1' is an unrecognized subcommand") },
    { ERR_LOCATE,	WGHT_ERROR,
	N_("Unable to locate a modulefile for '$1'") },
    { ERR_MAGIC,	WGHT_ERROR,
	N_("Magic cookie '#%Module' missing in '$1'") },
    { ERR_MODULE_PATH,	WGHT_ERROR, N_("'MODULEPATH' not set") },
    { ERR_HOME,		WGHT_ERROR, N_("'HOME' not set") },
    { ERR_SHELL,	WGHT_ERROR, N_("Unknown shell type '$1'") },
    { ERR_DERELICT,	WGHT_ERROR, N_("Unknown shell derelict '$1'") },
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_CONFLICT,	WGHT_ERROR,
	N_("Module '$1' conflicts with the currently loaded module(s) '$2+'")},
/* TRANSLATORS: do not exchange order of arguments */
    { ERR_PREREQ,	WGHT_ERROR,
	N_("Module '$1' depends on one of the module(s) '$2+'") },
    { ERR_NOTLOADED,	WGHT_ERROR, N_("Module '$1' is currently not loaded") },
    { ERR_DUP_SYMVERS,	WGHT_PROB,  N_("Duplicate version symbol '$1' found") },
    { ERR_SYMLOOP,	WGHT_ERROR, N_("Version symbol '$1' loops") },
    { ERR_BADMODNAM,	WGHT_PROB,  N_("Invalid modulename '$1' found") },
    { ERR_DUP_ALIAS,	WGHT_WARN,  N_("Duplicate alias '$1' found") },
    { ERR_BEGINENV,	WGHT_WARN,
#ifdef BEGINENV
#  if BEGINENV == 99
	N_("Invalid update subcommand - No _MODULESBEGINENV_ file")
#  else
	N_("Invalid update subcommand - No .modulesbeginenv file")
#  endif
#else
	N_("Invalid update subcommand - .modulesbeginenv not supported")
#endif
	},
    { ERR_BEGINENVX,	WGHT_WARN,
N_("Invalid update subcommand - No MODULESBEGINENV - hence not supported")},
    { ERR_INIT_TCL,	WGHT_ERROR, N_("Cannot initialize TCL") },
    { ERR_INIT_TCLX,	WGHT_WARN,
N_("Cannot initialize TCLX - modules using extended commands might fail") },
    { ERR_INIT_ALPATH,	WGHT_WARN,
N_("Could not extend auto_path variable - modules using autoloadable functions might fail") },
    { ERR_INIT_STUP,	WGHT_WARN,
N_("Cannot find a 'module load' command in any of the '$1' startup files") },
    { ERR_SET_VAR,	WGHT_WARN,  N_("Cannot set TCL variable '$1'") },
    { ERR_INFO_DESCR,	WGHT_ERROR,
	N_("Unrecognized module info descriptor '$1'") },
    { ERR_INVWGHT_WARN,	WGHT_WARN,  N_("Invalid error weight '$1' found") },
    { ERR_INVFAC_WARN,	WGHT_WARN,  N_("Invalid log facility '$1'") },
    { ERR_INTERAL,	WGHT_PANIC, N_("Internal error in the alias handler") },
    { ERR_INTERRL,	WGHT_PANIC, N_("Internal error in the error logger") },
    { ERR_INVAL,	WGHT_PANIC, N_("Invalid error type '$1' found") },
    { ERR_INVWGHT,	WGHT_PANIC, N_("Invalid error weight '$1' found") },
    { ERR_INVFAC,	WGHT_PANIC, N_("Invalid log facility '$1'") },
    { ERR_ENVVAR,       WGHT_FATAL,
N_("The environment variables LOADMODULES and _LMFILES_ have inconsistent lengths.") }
};

/** ************************************************************************ **/
/**				    PROTOTYPES				     **/
/** ************************************************************************ **/

static	ErrTransTab	*ErrorLookup(	ErrType error_type );

static	ErrMeasr	*MeasLookup(	ErrWeights weigth );

static	int	FlushError(	ErrType		  Type,
				char		 *module,
				int		  lineno,
				ErrWeights	  Weight,
				char		 *WeightMsg,
				char		 *ErrMsgs,
				int		  argc,
				char		**argv);

static	int	PrintError(	char		 *buffer,
				ErrType		  Type,
				char		 *module,
				int		  lineno,
				ErrWeights	  Weight,
				char		 *WeightMsg,
				char		 *ErrMsgs,
				int		  argc,
				char		**argv);

static	char	*ErrorString(	char		 *ErrMsgs,
				int		  argc,
				char		**argv);

static	int	scan_facility(	char		 *s,
				FacilityNames	 *table,
				int		  size);

static	char	*GetFacility(	ErrWeights	  Weight);
static	ErrFacilities	*GetFacility_sub( ErrWeights Weight);


/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		Module_Verbosity				     **
 ** 									     **
 **   Description:	Display a verbose message			     **
 ** 									     **
 **   First Edition:	1995/12/27					     **
 ** 									     **
 **   Parameters:	int	result	Result code of the module command    **
 **			int	argc	Number of arguments to the module    **
 **					command				     **
 **			char	**argv	Argument array			     **
 **			char	*buffer	Print buffer			     **
 ** 									     **
 **   Result:		-						     **
 ** 									     **
 **   Attached Globals:	g_current_module	The module which is handled  **
 **						by the current command	     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

void Module_Verbosity(
	int argc,
	char **argv
) {
	if (sw_verbose && argc && *argv)
		if (!FlushError
		    (NO_ERR_VERBOSE, g_current_module, linenum, WGHT_VERBOSE,
		     Measurements[MEAS_VERB_NDX].message, *argv, argc, argv)) {
			ErrorLogger(ERR_INTERRL, LOC, NULL);
		}

} /** End of 'Module_Verbosity' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		Enable_Error, Disable_Error, Restore_Error	     **
 ** 									     **
 **   Description:	Enables, disables, or restores error logging	     **
 ** 			Sometimes an error isn't really an error	     **
 ** 									     **
 **   First Edition:	1999/11/11					     **
 ** 									     **
 **   Parameters:	none						     **
 ** 									     **
 **   Result:		none						     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

static void save_error_state(int reset) {
	static int in_effect = 0;
	static int saved_state = 0;

	if (reset && in_effect ) {
		quiet_on_error = saved_state;
		in_effect = 0;
	} else if (!reset && !in_effect ) {
		saved_state = quiet_on_error;
		in_effect = 1;
	}
}


void Enable_Error(void) {
	save_error_state(0);
	quiet_on_error = 0;
}

void Disable_Error(void) {
	save_error_state(0);
	quiet_on_error = 1;
}

void Restore_Error(void) {
	quiet_on_error = 0;	/* the default is to output errors */
	save_error_state(1);
}

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		Module_Error					     **
 ** 									     **
 **   Description:	Error handling for the modules package		     **
 ** 									     **
 **   First Edition:	1995/08/06					     **
 ** 									     **
 **   Parameters:	ErrType		 error_type	Type of the error    **
 **			char		*module		Affected module	     **
 **			int		 lineo		Line number	     **
 **			...				Argument list	     **
 ** 									     **
 **   Result:		ErrCode		OK		No error	     **
 **					PROBLEM		Problem. Program may **
 **							continue running     **
 **					ERROR		Caller should try to **
 **							exit gracefully      **
 ** 									     **
 **   Attached Globals:							     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

int Module_Error(
	ErrType error_type,
	char *module,
	int lineno,
	...
) {
	uvec           *uv;			/** Argv vector		     **/
	char	       *arg;			/** current argument	     **/
	va_list         parptr;			/** Varargs scan pointer     **/
	ErrTransTab    *TransPtr;		/** Error transtab entry     **/
	ErrMeasr       *MeasPtr;		/** Measurement pointer      **/
	ErrCode         ret_code;
	int             NoArgs = (error_type == ERR_ALLOC);

	if (quiet_on_error)
		return OK;	/* do nothing - just OK */
    /**
     **  Argument check
     **/
	if (NO_ERR_VERBOSE == error_type && !sw_verbose)
		return (OK);

	if (!module)
		module = _(em_unknown);
    /**
     **  Construct argument array
     **/
	if (!(uv = uvec_ctor(ARGLIST_SIZE))) {
		module = module_name;
		error_type = ERR_ALLOC;
		NoArgs = 1;
	}

	va_start(parptr, lineno);
	while (!NoArgs && (arg = va_arg(parptr, char *))) {
		if (uvec_add(uv, arg)) {
			module = module_name;
			error_type = ERR_ALLOC;
			NoArgs = 1;
			break;
		}
	} /** while **/

	if (NO_ERR_VERBOSE == error_type && !uvec_number(uv))
		return (OK);
    /**
     **  Locate the error translation table entry according to the 
     **  passed error type
     **/
	if (!(TransPtr = ErrorLookup(error_type))) {
		if (uv)
			uvec_dtor(&uv);
		return (ErrorLogger(ERR_INVAL, LOC, (sprintf(buffer, "%d",
			error_type), buffer), NULL));
	}
    /**
     **  Now locate the assigned error weight ...
     **/
	if (!(MeasPtr = MeasLookup(TransPtr->error_weight))) {
		if (uv)
			uvec_dtor(&uv);
		return (ErrorLogger(ERR_INVWGHT, LOC, (
			sprintf(buffer, "%d", TransPtr->error_weight),
						       buffer), NULL));
	}
    /**
     **  Use the return code as defined for the current user level
     **/
	switch (sw_userlvl) {
	case UL_NOVICE:
		ret_code = MeasPtr->ret_nov;
		break;
	case UL_ADVANCED:
		ret_code = MeasPtr->ret_adv;
		break;
	case UL_EXPERT:
		ret_code = MeasPtr->ret_exp;
		break;
	}
    /**
     **  Print the error message
     **/
	if (TransPtr->error_weight <= WGHT_DEBUG || ret_code != OK)
		if (!FlushError(error_type, module, lineno,
		     TransPtr->error_weight, MeasPtr->message,
		     TransPtr->messages, uvec_number(uv), uvec_vector(uv))) {
			if (uv)
				uvec_dtor(&uv);
			ret_code = ErrorLogger(ERR_INTERRL, LOC, NULL);
		}
    /**
     **  Return to the caller ... conditionally ...
     **/
	if (WARN == ret_code)
		ret_code = OK;

	if (uv)
		uvec_dtor(&uv);

	if (ret_code > ERROR)
		exit(ret_code);

	return ret_code;

} /** End of 'Module_Error' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		ErrorLookup					     **
 ** 									     **
 **   Description:	Look up the passed error type in the translation tab.**
 ** 									     **
 **   First Edition:	1995/08/06					     **
 ** 									     **
 **   Parameters:	ErrType		 error_type	Type of the error    **
 ** 									     **
 **   Result:		ErrTransTab*	NULL	Not found		     **
 **					else	Pointer to the acc. entry    **
 ** 									     **
 **   Attached Globals:							     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

static ErrTransTab *ErrorLookup(
	ErrType error_type
) {
	ErrTransTab    *low, *mid, *high, *save;/** Search pointers	     **/
    /**
     **  Init search pointers
     **/
	low = TransTab;
	high = TransTab + (sizeof(TransTab) / sizeof(TransTab[0]) - 1);
	mid = (ErrTransTab *) NULL;
    /**
     **  Search loop
     **/
	while (low < high) {
		save = mid;
		mid = low + ((high - low) / 2);
		if (save == mid)
			low = mid = high;	/** Just for safety ...	     **/
		if (mid->error_type < error_type) {
			low = mid;
		} else if (mid->error_type > error_type) {
			high = mid;
		} else
			return (mid);		/** Yep! Got it!	     **/
	} /** while **/
    /**
     **  Maybe the loop has been finished before the comparison took place
     **  (low == high) and hit!
     **/
	if (mid->error_type == error_type)
		return (mid);			/** Yep! Got it!	     **/
    /**
     **  If this point is reached, nothing has been found ...
     **/
	return (NULL);

} /** End of 'ErrorLookup' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		MeasLookup					     **
 ** 									     **
 **   Description:	Look up the passed error weight in the measurement   **
 **			table						     **
 ** 									     **
 **   First Edition:	1995/08/06					     **
 ** 									     **
 **   Parameters:	ErrWeights	weigth	Weight of the error	     **
 ** 									     **
 **   Result:		ErrMeasr*	NULL	Not found		     **
 **					else	Pointer to the acc. entry    **
 ** 									     **
 **   Attached Globals:							     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

static ErrMeasr *MeasLookup(
	ErrWeights weigth
) {
	ErrMeasr       *low, *mid, *high, *save;/** Search pointers	     **/
    /**
     **  Init search pointers
     **/
	low = Measurements;
	high = Measurements + (sizeof(Measurements)/sizeof(Measurements[0])-1);
	save = (ErrMeasr *) NULL;
	mid = (ErrMeasr *) NULL;
    /**
     **  Search loop
     **/
	while (low < high) {
		save = mid;
		mid = low + ((high - low) / 2);
		if (save == mid)
			low = mid = high;	/** Just to be sure ...      **/
		if (mid->error_weight < weigth) {
			low = mid;
		} else if (mid->error_weight > weigth) {
			high = mid;
		} else
			return (mid);		/** Yep! Got it!	     **/
	} /** while **/
    /**
     **  Maybe the loop has been finished before the comparison took place
     **  (low == high) and hit!
     **/
	if (mid->error_weight == weigth)
		return (mid);			/** Yep! Got it!	     **/
    /**
     **  If this point is reached, nothing has been found ...
     **/
	return (NULL);

} /** End of 'MeasLookup' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		FlushError					     **
 ** 									     **
 **   Description:	Print the error message. Decide which facility to    **
 **			use and schedule the according logger routine	     **
 ** 									     **
 **   First Edition:	1995/12/21					     **
 ** 									     **
 **   Parameters:	ErrType		  Type		Error type as passed **
 **			char		 *module	Module name	     **
 **			int		  lineno	Line number	     **
 **		  	ErrWeights	  Weight	Error Weight	     **
 **			char		 *WeightMsg	Printable Weight     **
 **			char		 *ErrMsgs	Error message	     **
 **			int		  argc		Number of arguments  **
 **			char		**argv		Argument array	     **
 ** 									     **
 **   Result:		int	1		Everything OK		     **
 **				0		Error occured while printing **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

static int FlushError(
	ErrType Type,
	char *module,
	int lineno,
	ErrWeights Weight,
	char *WeightMsg,
	char *ErrMsgs,
	int argc,
	char **argv
) {
	char           *facilities,
		       *fac,
		       *errmsg_buffer,
		       *fac_buffer;
	FILE           *facfp;

    /**
     **  get the error facilities at first. If there isn't any, we may
     **  return on success immediatelly.
     **/
	if (!(facilities = GetFacility(Weight)))
		goto success0;
    /**
     **  set psep if it hasn't been done yet (initialization failure)
     **  ... assume '/'
     **/
	if (!psep)
		psep = "/";
    /**
     **  copy the facilities string so we can do some changes in it
     **/
	if (!(fac_buffer = stringer(NULL, 0, facilities, NULL))) {
		ErrorLogger(ERR_ALLOC, LOC, NULL);
		goto unwind0;
	}
    /**
     **  Print the error message into the buffer
     **/
	if (!(errmsg_buffer = stringer(NULL, ERR_BUFSIZE, NULL))) {
		ErrorLogger(ERR_ALLOC, LOC, NULL);
		goto unwind1;
	}
    /**
     **  In case of verbosity, the first argument is the ErrMsgs format string
     **/
	if (WGHT_VERBOSE == Weight) {
		ErrMsgs = *argv++;
		--argc;
	}
	if (!PrintError(errmsg_buffer, Type, module, lineno, Weight,
			WeightMsg, ErrMsgs, argc, argv))
		goto unwind2;
    /**
     **  Now tokenize the facilities string and schedule the error message
     **  for every single facility
     **/
	for (fac = xstrtok(fac_buffer, ":");
	     fac;
	     fac = xstrtok((char *)NULL, ":")) {
	/**
	 **  Check for filenames. Two specials are defined: stderr and stdout
	 **  Otherwise filenames are expected to begin on '.' or '/'.
	 **  Everthing not recognized as a filename is assumed to be a
	 **  syslog facility
	 **  'null' and 'none' are known as 'no logging'
	 **/
		if (!strcmp(fac, _null) || !strcmp(fac, _none))
			continue;
		else if (!strcmp(fac, _stderr))
			fprintf(stderr, "%s", errmsg_buffer);
		else if (!strcmp(fac, _stdout))
			fprintf(stdout, "%s", errmsg_buffer);
	/**
	 **  Syslog
	 **/
		else if ('.' != *fac && *psep != *fac) {
#if defined(HAVE_SYSLOG) && defined(WITH_LOGGING)
			int             syslog_fac, syslog_lvl;
			/* now send to syslog */
			if (CheckFacility(fac, &syslog_fac, &syslog_lvl)) {
				openlog("modulecmd", LOG_PID, syslog_fac);
				setlogmask(LOG_UPTO(syslog_lvl));
				syslog((syslog_fac | syslog_lvl), "%s",
				       errmsg_buffer);
				closelog();
	    /**
	     **  Invalid facilities ... take care not to end up in 
	     **  infinite loops
	     **/
			} else if (Type == ERR_INVFAC ||
				   OK == ErrorLogger(ERR_INVFAC,LOC,fac,NULL))
				continue;
#endif
		}
	/**
	 **  Custom files ...
	 **  This may result from the syslog part above
	 **/
		if ('.' == *fac || *psep == *fac) {
			if (!(facfp = fopen(fac, "a"))) {
				if (WGHT_PANIC == Weight) /** Avoid loops!**/
					goto unwind2;
		/**
		 **  Invalid facilities ... take care not to end up in 
		 **  infinite loops
		 **/
				if (Type == ERR_INVFAC ||
				    OK == ErrorLogger(ERR_INVFAC,LOC,fac,NULL))
					continue;
				else
					goto unwind2;
			}
			fprintf(facfp, "%s", errmsg_buffer);

			if (EOF == fclose(facfp))
				if (OK != ErrorLogger(ERR_CLOSE,LOC,fac,NULL))
					goto unwind2;
		}
	} /** for **/
    /**
     **  Return on success
     **/
	null_free((void *)&errmsg_buffer);
	null_free((void *)&fac_buffer);
success0:
	return (1);			/** -------- EXIT (SUCCESS) -------> **/

unwind2:
	null_free((void *)&errmsg_buffer);
unwind1:
	null_free((void *)&fac_buffer);
unwind0:
	return (0);			/** -------- EXIT (FAILURE) -------> **/

} /** End of 'FlushError' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		GetFacility					     **
 ** 									     **
 **   Description:	Get the log facility according to the passed error   **
 **			weight						     **
 ** 									     **
 **   First Edition:	1995/12/21					     **
 ** 									     **
 **   Parameters:	ErrWeights	  Weight	Error Weight	     **
 ** 									     **
 **   Result:		char*	NULL		No facility found	     **
 **				Otherwise	Pointer to the colon separa- **
 **						ted facility string	     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

static char    *GetFacility(
	ErrWeights Weight
) {
	ErrFacilities  *facility;

    /**
     **  Get the facility table entry at first
     **/
	if (!(facility = GetFacility_sub(Weight)))
		return ((char *)NULL);
    /**
     **  Now we've got two possibilities:
     **      First of all there may be a custom facilitiy defined
     **	     Otherwise the default facility is to be returned now
     **/
	return (facility->facility ? facility->facility : facility->
		def_facility);
} /** End of 'GetFacility' **/

static ErrFacilities *GetFacility_sub(
	ErrWeights Weight
) {
	ErrFacilities  *low, *mid, *high, *save;
	char            buffer[20];
    /**
     **  Binary search for the passed facility in the Facilities table.
     **  This requires the table to be sorted on ascending error weight
     **/
	low = Facilities;
	high = Facilities + (sizeof(Facilities) / sizeof(Facilities[0]));
	save = (ErrFacilities *) NULL;
	mid = (ErrFacilities *) NULL;

	while (low < high) {
		save = mid;
		mid = low + ((high - low) / 2);
		if (save == mid)
			low = mid = high;	/** Just to be sure ...	     **/
		if (mid->Weight > Weight)
			high = mid;
		else if (mid->Weight < Weight)
			low = mid;
		else
			break;			/** found! **/
	}
    /**
     **  We have to check, if we've found something or if there's an internal
     **  error (wrong weight)
     **/
	if (mid->Weight != Weight) {
		if (OK == ErrorLogger(ERR_INVWGHT, LOC,
			(sprintf(buffer, "%d", Weight), buffer), NULL))
			return (NULL);
	}

	return (mid);

} /** End of 'GetFacility_sub' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		CheckFacility					     **
 ** 									     **
 **   Description:	Check the passed string to be a valid combination    **
 **			of       <syslog_facility>.<syslog_level>	     **
 ** 									     **
 **   First Edition:	1995/12/21					     **
 ** 									     **
 **   Parameters:	char	*string		Input facility string	     **
 **			int	**facility	Buffer for the real facility **
 **			int	**level		Buffer for the real level    **
 ** 									     **
 **   Result:		int	1		Success			     **
 **				0		Failure. String not valid    **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

int CheckFacility(
	char *string,
	int *facility,
	int *level
) {
	int             x;
	uvec           *tmp;

    /**
     **  Create a vector of the components delimited by '.'
     **/

	if (!(tmp = str2uvec(".", string)))
		if (OK == ErrorLogger(ERR_UVEC, LOC, NULL))
			goto unwind0;
    /**
     **  This should be the facility
     **/
	if (-1 == (x = scan_facility(uvec_vector(tmp)[0], facility_names,
				     (sizeof(facility_names) /
				      sizeof(facility_names[0])))))
		goto unwind1;
	if (facility)
		*facility = x;
    /**
     **  This should be the level
     **/
	if (-1 == (x = scan_facility(uvec_vector(tmp)[1], level_names,
				     (sizeof(level_names) /
				      sizeof(level_names[0])))))
		goto unwind1;
	if (level)
		*level = x;
    /**
     **  Success
     **/
	uvec_dtor(&tmp);
	return (1);			/** -------- EXIT (SUCCESS) -------> **/

unwind1:
	uvec_dtor(&tmp);
unwind0:
	return (0);			/** -------- EXIT (FAILURE) -------> **/

} /** End of 'CheckFacility' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		scan_facility					     **
 ** 									     **
 **   Description:	Scan the passed facility names table for the given   **
 **			string and pass back the assigned token		     **
 ** 									     **
 **   First Edition:	1995/12/21					     **
 ** 									     **
 **   Parameters:	char		*s	String to be checked	     **
 **			FacilityNames	*table	Table of valid names and     **
 **						tokens			     **
 **			int		 size	Size of the table	     **
 ** 									     **
 **   Result:		int	-1		name not found in the table  **
 **				Otherwise	Assigned token		     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

static int scan_facility(
	char *s,
	FacilityNames * table,
	int size
) {
	FacilityNames  *low, *mid, *high, *save;

	low = table;
	high = table + size;
	save = (FacilityNames *) NULL;

	while (low < high) {
		int             x;
				/** Have to use this, because strcmp will    **/
				/** not return -1 and 1 on Solaris 2.x	     **/
		save = mid;
		mid = low + ((high - low) / 2);
		if (save == mid)
			low = mid = high;	/** To prevent endless loops **/
		if (mid == high)
			return -1;
		x = strcmp(mid->name, s);

		if (x < 0)
			low = mid;
		else if (x > 0)
			high = mid;
		else
			return (mid->token);
	} /** while **/

	return (!strcmp(mid->name, s) ? mid->token : -1);

} /** End of 'scan_facility' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		GetFacilityPtr					     **
 ** 									     **
 **   Description:	Scan the passed facility names table for the given   **
 **			string and pass back the assigned token		     **
 ** 									     **
 **   First Edition:	1995/12/21					     **
 ** 									     **
 **   Parameters:	char	*facility	Name of the facility	     **
 ** 									     **
 **   Result:		char**	NULL		Invalid facility name	     **
 **				Otherwise	Pointer to the facilty string**
 **						reference		     **
 ** 									     **
 ** ************************************************************************ **
 ++++*/

char          **GetFacilityPtr(
	char *facility
) {
	int             i, len;
	ErrMeasr       *measptr;
	char           *buf, *t;
	ErrFacilities  *facptr;
    /**
     **  Try to figure out the error weight at first
     **  Need the given weight in upper case for this
     **/
	len = strlen(facility);

	if (!(buf = stringer(NULL, 0, facility, NULL)))
		if (OK != ErrorLogger(ERR_ALLOC, LOC, NULL))
			goto unwind0;
	for (t = buf; *t; ++t)
		*t = toupper(*t);
    /**
     **  Now look up the measurements table for the uppercase weight
     **/
	i = sizeof(Measurements) / sizeof(Measurements[0]);
	measptr = Measurements;

	while (i) {
		if (!strncmp(measptr->message, buf, len))
			break;
		i--;
		measptr++;
	}

	null_free((void *)&buf);

	if (!i)				    /** not found		     **/
		goto unwind0;

    /**
     **  Now get the facility table entry
     **/
	if (!(facptr = GetFacility_sub(measptr->error_weight))) {
		ErrorLogger(ERR_INVWGHT_WARN, LOC, facility, NULL);
		goto unwind0;
	}
    /**
     **  Got it ... return the desired pointer
     **/
	return (&facptr->facility);	/** -------- EXIT (RESULT)  -------> **/

unwind0:
	return ((char **)NULL);		/** -------- EXIT (FAILURE) -------> **/

} /** End of 'GetFacilityPtr' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		PrintError					     **
 ** 									     **
 **   Description:	Print the error message				     **
 ** 									     **
 **   First Edition:	1995/08/06					     **
 ** 									     **
 **   Parameters:	char             *errbuffer	Buffer to hold the   **
 **							error messge	     **
 **			ErrType		  Type		Error type as passed **
 **			char		 *module	Module name	     **
 **			int		  lineno	Line number	     **
 **		  	ErrWeights	  Weight	Error Weight	     **
 **			char		 *WeightMsg	Printable Weight     **
 **			char		 *ErrMsgs	Error message	     **
 **			int		  argc		Number of arguments  **
 **			char		**argv		Argument array	     **
 ** 									     **
 **   Result:		int	1		Everything OK		     **
 **				0		Error occured while printing **
 ** 									     **
 **   Notes:	According to the error type, the passed module and line num- **
 **		ber will be handled as a module-file related one or depending**
 **		on the packages source code:				     **
 **									     **
 **		src -> ERR_IN_MODULEFILE -> modulefile -> ERR_INTERNAL -> src**
 ** 									     **
 ** ************************************************************************ **
 ++++*/

static int PrintError(
	char *errbuffer,
	ErrType Type,
	char *module,
	int lineno,
	ErrWeights Weight,
	char *WeightMsg,
	char *ErrMsgs,
	int argc,
	char **argv
) {
	char           *error_string;

    /**
     **  Build the error string at first. Note - we cannot alloc memory any
     **  more!
     **/
	if (ERR_ALLOC == Type)
		error_string = ErrMsgs;
	else if (!(error_string = ErrorString(ErrMsgs, argc, argv)))
		return (0);
    /**
     **  Print
     **/
	if (ERR_INTERNAL > Type && ERR_IN_MODULEFILE < Type &&
	    linenum && g_current_module)
		sprintf(errbuffer, "%s(%s):%s:%d: %s\n", g_current_module,
			(sprintf(buffer, "%d", linenum), buffer),
			WeightMsg, Type, (error_string ? error_string : ""));
	else
		sprintf(errbuffer, "%s(%s):%s:%d: %s\n",
			(module ? module : "??"),
			(lineno ? (sprintf(buffer, "%d", lineno), buffer) :
			 "??"), WeightMsg, Type,
			(error_string ? error_string : ""));
    /**
     **  Success
     **/
	return (1);

} /** End of 'PrintError' **/

/*++++
 ** ** Function-Header ***************************************************** **
 ** 									     **
 **   Function:		ErrorString					     **
 ** 									     **
 **   Description:	Format the error message			     **
 ** 									     **
 **   First Edition:	1995/08/06					     **
 ** 									     **
 **   Parameters:	char		 *ErrMsgs	Error message	     **
 **			int		  argc		Number of arguments  **
 **			char		**argv		Argument array	     **
 ** 									     **
 **   Result:		char*	NULL		Parse or alloc error	     **
 **				else		Pointer to the error string  **
 ** 									     **
 **   Attached Globals:	-						     **
 **									     **
 ** ************************************************************************ **
 ++++*/

static char    *ErrorString(
	char *ErrMsgs,
	int argc,
	char **argv
) {
	uvec	       *errvec;		/** error msg vector		     **/
	char           *X,*L;		/** gettext string pointer	     **/
	int             backslash = 0,	/** backslash found ?		     **/
		        idx = 0;	/** index into argv		     **/

	if (!ErrMsgs)
		return (NULL);
    /**
     **  Construct vector
     **/
	if (!(errvec = uvec_ctor(8))) {
		ErrorLogger(ERR_UVEC, LOC, NULL);
		return (NULL);
	}
	L = X = _(ErrMsgs);
    /**
     **  Scan the error strings to be printed
     **/
	while (*X) {
	/**
	 **  Check for special characters
	 **/
		switch (*X) {
		case '\\':
			if (!backslash) {
				backslash = 1;
				X++;
				continue;	/** while( *ErrMsgs) **/
			}
			break;			/** switch **/
		case '$':
			if (!backslash) {
				/* add the portion we have */
				uvec_nadd(errvec, L, X - L);
				X++;
				/* add the given parameter 1 - 9 */
				idx = *X - '1';
				uvec_add(errvec, argv[idx++]);
				if (X[1] == '+') {
					X++;
					/* grab all remaining args */
					while (idx < argc)
						uvec_add(errvec, argv[idx++]);
				}
				/* reset last string start */
				L = X + 1;
				continue;	/** while( *ErrMsgs) **/
			}
			break;		/** switch **/
		} /** switch **/
		X++;
		backslash = 0;
	} /** while( *ErrMsgs) **/

	/* append final little bit */
	uvec_add(errvec, L);
    /**
     **  Success. Clean-up and return a pointer to the newly created string
     **/
	error_line = uvec2str(errvec, "");
	uvec_dtor(&errvec);
	return (error_line);

} /** End of 'ErrorString' **/
