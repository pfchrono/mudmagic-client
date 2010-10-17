/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* iscript.c:                                                              *
*                2005 Shlykov Vasiliy ( vash@vasiliyshlykov.org )         *
*                                                                         *
*                Small BASIC interpreter.                                 *
*                                                                         *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
//#include <mudmagic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include "iscript.h"

// #define ISCRIPT_DEBUG

/** Maximum number of external variables */
#define IMAX_EXT_VARS       32
/** Maximum number of external functions */
#define IMAX_EXT_FUNCS      32
/** Maximum number of variables */
#define IMAX_VARS           64
/** Maximum length of variable name */
#define IMAX_VAR_NAME_SIZE  63
/** Maximum length of command name */
#define IMAX_COM_NAME_SIZE  31
/** Maximum length of token */
#define IMAX_TOKEN_LEN      127
/** Maximum depth of IF stack */
#define IMAX_IF_DEPTH       12

enum ITokenType { NONE, DELIMITER, EXT_VAR, VAR, ID, NUMBER,
                  COMMAND, EXT_COMMAND, STRING, OP, EOL, FIN, IF, THEN, ELSE, END };

static const char* debug_tok_type[] =
{
  "NONE", "DELIMITER", "EXT_VAR",
  "VAR", "ID", "NUMBER", "COMMAND", "EXT_COMMAND", "STRING", "OP",
  "EOL", "FIN", "IF", "THEN", "ELSE", "END"
};

enum IOp { LT = 128, GT, LE, GE, NE, EQ };

enum IIntCommand { PRINT = 1, REM, SEND };

enum IComRet { NORETURN, RETURN };

//typedef struct _IScript IScript;


typedef struct _IFlowCtrl   IFlowCtrl;
typedef struct _ICommand    ICommand;
typedef struct _IVar        IVar;
typedef struct _IF          IFC;
typedef IScriptExtFuncInfo  IFunction;

typedef IVar* (*IComExec_t) (IScript*);

struct _IFlowCtrl
{
    char        name[IMAX_COM_NAME_SIZE + 1];
    int         tok;
};

struct _ICommand
{
    char        name[IMAX_COM_NAME_SIZE + 1];
    int         tok; // IIntCommand
    IComExec_t  exec;
    int         mayret; // IComRet
};

enum IType
{
    UNDEF,
    INT,
    STR
};

struct _IVar
{
    char        name[IMAX_VAR_NAME_SIZE + 1];
    char*       sptr;
    int         type;
    int         ival;
    char*       sval;

};

struct _IF
{
    int cond;
    int part; // current flow: 1 - THEN, 0 - ELSE
};

struct _IScript
{
    /** Tokens table */
    char        token[IMAX_TOKEN_LEN + 1];

    /** Filled by #get_token */
    int         token_type;

    /** Current token - an operation */
    int         iop;

    /** Current token - a number */
    int         inumber;

    /** Current token - user variable */
    IVar*       ivar;

    /** Current token - a command */
    ICommand*   icommand;

    /** Current token - internal function */
    IFunction*  ifunc;

    /** List of internal commands */
    ICommand*   icommands;

    /** Variables table */
    IVar        ivars [IMAX_VARS];

    /** External variables table */
    IVar        iextvars [IMAX_EXT_VARS];

    /** External functions table */
    IScriptExtFuncInfo
                iextfuncs [IMAX_EXT_FUNCS];

    /* Sizes of related tables */
    int         ivars_size;
    int         iextvars_size;
    int         iextfuncs_size;

    /** Pointer to first symbol of program */
    const char* prog;

    /** Current symbol */
    const char* buf;

    /** Program output buffer (where result places) */
    char*       outbuf;

    /** Current position in output buffer */
    char*       optr;

    /** Size of output buffer */
    size_t      outbuf_size;

    /** Status */
    int         error;

    /** User error handler */
    IErrHandler_t
                err_handler;

    /** Data that should be placed to user error handler */
    void*       userdata;

    /** Line counter in source text */
    unsigned int cline;
};

static int   iscript_parse (IScript*, int skip_flow);

static IVar* eval_exp (IScript*);
static IVar* eval_exp1 (IScript*);
static IVar* eval_exp2 (IScript*);
static IVar* eval_exp3 (IScript*);
static IVar* eval_exp4 (IScript*);
static IVar* eval_exp5 (IScript*);
static IVar* eval_exp6 (IScript*);
static IVar* atom (IScript*);
static IVar* process_op (IScript*, int op, const IVar* v1, const IVar* v2);
static IVar* run_command (IScript*, const ICommand*);
static IVar* run_function (IScript*, const IFunction*);

static void  print_string (IScript*, const char*);
static void  print_var (IScript*, const IVar*);
static IVar* com_print (IScript*);
static IVar* com_rem (IScript*);

static IVar* com_if (IScript*, int skip_flow);

static void  assignment (IScript* is);
static void  set_var (IVar* dst, const IVar* src);
static void  free_var (IVar* var);
static IVar* create_var (IScript*, const char* name, int type, const char* ref);
static IVar* create_tmp_var (IScript*, int type);
static IVar* create_itmp (IScript*, int value);
static IVar* create_stmp (IScript*, const char* value);
static IVar* create_sconcat (IScript*, const char* str1, const char* str2);

static int   get_token (IScript*);
static int   get_token_non_eol (IScript*);
static void  putback (IScript*);
static void  skip_until_eol (IScript*);
static char* conv_to_str (int i);

static IFlowCtrl flow_ctrl[] =
{
    { "IF",     IF },
    { "THEN",   THEN },
    { "ELSE",   ELSE },
    { "END",    END }
};

static ICommand int_commands[] =
{
    { "PRINT",  PRINT,  &com_print,     NORETURN },
    { "REM",    REM,    &com_rem,       NORETURN },
    { "SEND",   SEND,   &com_print,	NORETURN },
};

static const char* int_errmsg[] =
{
  "OK",
  "Syntax error",
  "Undefined identifier",
  "Unexpected end of program",
  "Invalid operation",
  "Too many variables",
  "Undefined external variable",
  "Command doen't return a value",
  "Unmatched bracket",
  "Invalid type",
  "Invalid number format",
  "Invalid condition",
  "THEN expected",
  "END without IF",
  "Too few parameters for function",
  "Too many parameters for function"
};

#ifdef ISCRIPT_DEBUG
static inline void
idebug (const char* fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  vfprintf (stderr, fmt, ap);

  va_end (ap);
}
#else
static inline void
idebug (const char* fmt, ...) {}
#endif // ISCRIPT_DEBUG

/**
 * ierror: Set error.
 *
 * @msg: Error id.
 *
 **/
static inline void
ierror (IScript* is, int msg)
{
  is->error = msg;

  idebug ("ERROR:%d: %s\n", is->cline, int_errmsg[msg]);

  if (is->err_handler != NULL)
    {
      (*is->err_handler) (is->cline, msg, int_errmsg[msg], is->userdata);
    }
}

/**
 * iscript_new: Creates new #IScript object. After use #iscript_free should
 *              be called.
 *
 * Return value: Pointer to new #IScript.
 *
 **/
IScript*
iscript_new (void)
{
  return (IScript*) malloc (sizeof (IScript));
}

/**
 * iscript_free: Frees memory used by #IScript. Always calls #iscript_clear.
 *
 * @is: A #IScript.
 *
 **/
void
iscript_free (IScript* is)
{
  if (NULL == is)
      return;

  iscript_clear (is);

  while (--is->iextvars_size >= 0)
    {
      idebug ("Freeing `%s`\n", is->iextvars[is->iextvars_size].name);
      //if (is->iextvars[is->iextvars_size].sval != NULL)
          free (is->iextvars[is->iextvars_size].sval);
    }

  free (is);
}

/**
 * iscript_init: Initializes given #IScript object before using.
 *
 * @is: A #IScript.
 *
 **/
void
iscript_init (IScript* is)
{
  if (NULL == is)
      return;

  is->token[0] = '\0';
  is->token_type = NONE;
  is->iop = 0;
  is->ivar = NULL;
  is->icommand = NULL;
  is->ifunc = NULL;
  is->icommands = int_commands;

  is->ivars_size = 0;
  is->iextvars_size = 0;
  is->iextfuncs_size = 0;
  is->cline = 1;
  is->error = OK;
  is->err_handler = NULL;
  is->userdata = NULL;
}

/**
 * iscript_clear: Cleares a #IScript object (including all variables and
 *                temporary results). All external variables and functions are
 *                will be saved. After clearing iscript could be reused.
 *
 * @is: A #IScript.
 *
 **/
void
iscript_clear (IScript* is)
{
  size_t i = 0,
         iev_saved,
         ief_saved;

  if (NULL == is)
      return;

  while (i < is->ivars_size)
    {
      free_var (&is->ivars[i++]);
    }

  iev_saved = is->iextvars_size;
  ief_saved = is->iextfuncs_size;
  iscript_init (is);
  is->iextvars_size = iev_saved;
  is->iextfuncs_size = ief_saved;
}

/**
 * iscript_set_err_handler: Set error handler for iscript.
 *
 * @is:  A #IScript.
 * @hnd: Callback funciton.
 *
 **/
void
iscript_set_err_handler (IScript* is, IErrHandler_t hnd, void* userdata)
{
  if (is == NULL)
      return;

  is->err_handler = hnd;
  is->userdata = userdata;
}

/**
 * iscript_ext_var_add: Adds a external variable to #IScript. Maximal length
 *                      of name is #IMAX_VAR_NAME_SIZE.
 *
 * @is:    A #IScript.
 * @name:  Variable name (without leading '$').
 * @value: Value (always interpreted as string).
 *
 * Return value: 0 if maximum number of external variables are reached,
 *               1 otherwise. See #IMAX_EXT_VARS
 *
 **/
int
iscript_ext_var_add (IScript* is, const char* name, const char* value)
{
  if (NULL == is || name == NULL)
      return 1;

  if (is->iextvars_size < IMAX_EXT_VARS)
    {
      size_t i = is->iextvars_size;

      *is->iextvars[i].name = '$';
      strncpy (is->iextvars[i].name + 1, name, IMAX_VAR_NAME_SIZE - 1);
      
      if (value != NULL)
        {
          is->iextvars[i].sval = strdup (value);
          is->iextvars[i].type = STR;
        }
      else
        {
          is->iextvars[i].sval = NULL;
          is->iextvars[i].type = 0;
        }

      ++is->iextvars_size;

      return 1;
    }
  else
    {
      return 0;
    }
}

/**
 * iscript_ext_func_add: Adds a external function to #IScript. Maximal length
 *                      of name is #IMAX_VAR_NAME_SIZE. Name of function should
 *                      be without leading '$'.
 *
 * @is:    A #IScript.
 * @finof: A #IScriptExtFuncInfo.
 *
 * Return value: 0 if maximum number of external functions are reached,
 *               1 otherwise. See #IMAX_EXT_FUNCS
 *
 **/
int
iscript_ext_func_add (IScript* is, IScriptExtFuncInfo* finfo)
{
  if (NULL == is || finfo == NULL)
      return 1;

  if (is->iextfuncs_size < IMAX_EXT_FUNCS)
    {
      is->iextfuncs [is->iextfuncs_size++] = *finfo;
      return 1;
    }
  else
    {
      return 0;
    }
}

/**
 * iscript_run: Runs script with given body. Result will be stored in out buffer.
 *
 * @is:       A #IScript.
 * @in:       Script text.
 * @out:      Output buffer.
 * @out_size: Length of output buffer.
 *
 * Return value: 1 if script executed successfuly,
 *               0 if an errors was occured.
 *
 **/
int
iscript_run (IScript* is, const char* in, char* out, size_t out_size)
{
  if (NULL == is || NULL == in)
      return 0;

  idebug ("Parsing: '%s'\n", in);

  is->buf = is->prog = in;
  is->optr = out;
  is->outbuf = out;
  is->outbuf_size = out_size;

  iscript_parse (is, 0);

  is->outbuf[is->outbuf_size] = '\0';

  idebug ("ERROR: %d\n", is->error);

  return ! is->error;
}

/**
 * skip_simple:
 *
 */
static void
skip_simple (IScript* is)
{
   while (1)
    {
      switch (*is->buf)
        {
          case '\0' : return;
          case ' '  :
          case '\t' : break;
          default : return;
        }
      ++is->buf;
    }
}

/**
 * parse_number:
 *
 **/
static void parse_number (IScript* is)
{
  unsigned int i = 0;

  while (*is->buf && isdigit (*is->buf))
      is->token[i++] = *is->buf++;

  is->token[i] = '\0';

  if (! sscanf (is->token, "%d", &is->inumber))
    {
      ierror (is, IS_ERR_INV_NUMBER);
    }

  idebug ("\tPARSE_NUMBER:%s:%d\n", is->token, is->inumber);
}

/**
 * parse_alpha:
 *
 **/
static void
parse_alpha (IScript* is)
{
  unsigned int i = 0;

  while (*is->buf && (isalpha (*is->buf) || *is->buf == '_' || isdigit (*is->buf)))
      is->token[i++] = *is->buf++;

  is->token[i] = '\0';
}

/**
 * parse_str:
 *
 **/
static void
parse_str (IScript* is)
{
  unsigned int i = 0;

  *is->buf++; // Skip leading "

  while (*is->buf && *is->buf != '"')
      is->token[i++] = *is->buf++;

  is->buf++; // Skip '"'

  is->token[i] = '\0';
}

/**
 * parse_internal:
 *
 **/
static void
parse_internal (IScript* is)
{
  unsigned int i = 0;

  is->token[i++] = '$';

  while (*is->buf && (isalpha (*is->buf) || *is->buf == '_' || isdigit (*is->buf)))
      is->token[i++] = *is->buf++;

  is->token[i] = '\0';
}

/**
 * create_tmp_var: Creates a temporary variable with given type. Always return
 *                 non-NULL. But when number of exist variables is greate than max.
 *                 possible "behaviour is undefined". Enjoy!
 *
 * @is:     a #IScript.
 * @type:   Type of variable.
 *
 * Return value: New variable.
 *
 **/
static inline IVar*
create_tmp_var (IScript* is, int type)
{
  return create_var (is, "TMP", type, NULL);
}

/**
 * create_itmp: Creates temp. integer variable.
 *
 * @is:     a #IScript.
 * @value:  Initial value for variable.
 *
 * Return value: Pointer to created variable.
 *
 **/
static inline IVar*
create_itmp (IScript* is, int value)
{
  IVar* tmp = create_tmp_var (is, INT);

  idebug ("CREATE_ITEMP:%d\n", value);

  tmp->ival = value;

  return tmp;
}

/**
 * create_stmp: Creates temp. string variable. Value of variable should be freed
 *              after use.
 *
 * @is   a #IScript.
 * @str: Initial value for variable.
 *
 * Return value: Pointer to created variable.
 *
 **/
static inline IVar*
create_stmp (IScript* is, const char* str)
{
  IVar* tmp = create_tmp_var (is, STR);

  tmp->sval = strdup (str);

  return tmp;
}

/**
 * lookup_int: Finds variable or command with given name (non-case sensitive).
 *
 * @is: A #IScript.
 * @n:  Name of a variable/command.
 *
 * Return value: Type of found entity (variable/command).
 *
 **/
static int
lookup_int (IScript* is, const char* n)
{
  size_t i,
         sz = is->ivars_size;

  idebug ("\tlookup_int: '%s'\n", n);

  for (i = 0; i < sz; ++i)
    {
      if (! strcasecmp (is->ivars[i].name, n))
        {
          is->ivar = is->ivars + i;
          return VAR;
        }
    }

  sz = sizeof(int_commands)/sizeof(int_commands[0]);

  for (i = 0; i < sz; ++i)
    {
      if (! strcasecmp (is->icommands[i].name, n))
        {
          is->icommand = is->icommands + i;
          return COMMAND;
        }
    }

  sz = sizeof(flow_ctrl)/sizeof(flow_ctrl[0]);

  for (i = 0; i < sz; ++i)
    {
      if (! strcasecmp (flow_ctrl[i].name, n))
        {
          return flow_ctrl[i].tok;
        }
    }

  idebug ("\tlookup_int: NOT FOUND\n");

  return ID;
}

/**
 * lookup_ext: Finds a external variable with given name (non-case sensitive).
 *
 * @is: A #IScript.
 * @n:  Name.
 *
 * Return value: EXT_VAR if found, NONE otherwise. Pointer of founded variable
 *               stored in IScript::ivar.
 *
 **/
static int
lookup_ext (IScript* is, const char* n)
{
  int i,
      sz = is->iextvars_size;

  for (i = 0; i < sz; ++i)
    {
      idebug ("LOOKUP_EXT: '%s' with '%s'\n", is->iextvars[i].name, n);
      if (! strcasecmp (is->iextvars[i].name, n))
        {
          is->ivar = is->iextvars + i;
          return EXT_VAR;
        }
    }

  sz = is->iextfuncs_size;
  for (i = 0; i < sz; ++i)
    {
      if (! strcasecmp (is->iextfuncs[i].name, (n+1)))
        {
          is->ifunc = is->iextfuncs + i;
          return EXT_COMMAND;
        }
    }

  return UNDEF;
}

/**
 * get_token: Parses token.
 *
 * @is: A #IScript.
 *
 */
static int
get_token (IScript* is)
{
  int ret;

  is->iop = 0;

  skip_simple (is);

  is->token[0] = *is->buf;
  is->token[1] = '\0';

  idebug ("\tget_token:start:%d: %c\n", is->cline, *is->buf);

  if (*is->buf == '\0')
    {
      is->token_type = FIN;
    }
  else if (*is->buf == '\n')
    {
//      for (;*is->buf == '\n'; is->buf++)
     
      is->buf++;
          is->cline++;
      is->token_type = EOL;
    }
  else if (isdigit (*is->buf))
    {
      parse_number (is);
      is->token_type = NUMBER;
    }
  else if (isalpha (*is->buf))
    {
      parse_alpha (is);
      ret = lookup_int (is, is->token);

      is->token_type = ret;
    }
  else if (*is->buf == '"')
    {
      parse_str (is);
      is->token_type = STRING;
    }
  else
    {
      switch (*is->buf++)
        {
          case '<' :
                      {
                        if (*is->buf == '>') 
                          {
                            ++is->buf; is->iop = NE;
                          }
                        else if (*is->buf == '=')
                          {
                            ++is->buf; is->iop = LE;
                          }
                        else
                          {
                            is->iop = LT;
                          }
                        is->token_type = OP;
                      } break;

          case '>' :
                      {
                        is->iop = GT;
                        is->token_type = OP;
                      } break;

          case '=' :
                     {
                       if (*is->buf == '>')
                         {
                           ++is->buf; is->iop = GE;
                         }
                       else
                         {
                           is->iop = EQ;
                         }
                        is->token_type = OP;
                     } break;

          case '$' : 
                     {
			int n, grab_num;
			char foo;

                       	parse_internal (is);

			//we want assigned variables to be numeric in nature too for calculations
                       	n = sscanf(is->buf, "%d%c", &grab_num, &foo );
                       	//1 is a valid integer
                       	if( n == 1 )
                       	{
                                parse_number (is);
				is->token_type = NUMBER;
		       	}
		       	else
		       	{
                       		ret = lookup_ext (is, is->token);
                       		if (ret == NONE)
                           		ierror (is, IS_ERR_UNDEF_EXT_ID);
                       		is->token_type = ret;
		       	}
                     } break;
          case '+' :
          case '-' :
          case '*' :
          case '/' :
          case '(' :
          case ')' : is->iop = *(is->buf - 1); is->token_type = OP; break;

	  case '.' :
          case ';' :
          case ',' : *is->token = *(is->buf - 1); is->token_type = DELIMITER; break;

          default : ierror (is, IS_ERR_SYNTAX);
                    return is->token_type = NONE;
        }
    }

  idebug ("\tget_token:end: %s (%d)\n", debug_tok_type[is->token_type], is->token_type);

  return is->token_type;
}

/**
 * get_token_non_eol: Gets first non-EOL token.
 *
 * @is: A #IScript.
 *
 **/
static inline int
get_token_non_eol (IScript* is)
{
  int ret = get_token (is);

  while (ret == EOL)
      ret = get_token (is);
  
  return ret;
}

/**
 * putback: Puts back current token.
 *
 **/
static void
putback (IScript* is)
{
  char* t = is->token;

  for (; *t; t++)
    {
      if (*t == '\n')
          is->cline--;
      is->buf--;
    }

  idebug ("PUTBACK:'%s'\n", is->token);

  // Paranoid
  is->ivar = NULL;
  is->icommand = NULL;
}

/**
 * skip_until_eol: Skips current line completely.
 *
 * @is: A #IScript.
 *
 **/
static void
skip_until_eol (IScript* is)
{
  while (*is->buf != '\0' && *is->buf != '\n') is->buf++;

  is->buf++; // Skip last EOL
}

/**
 * iscript_parse: The interpretator.
 *
 **/
static int
iscript_parse (IScript* is, int skip_flow)
{
  int tok;

  do
    {
      tok = get_token (is);

      switch (tok)
        {
          case IF   : com_if (is, skip_flow); break;
          case ELSE :
          case END  : return tok;
        }

      if (! skip_flow)
        {
          switch (tok)
            {
              case ID       : is->ivar = create_var (is, is->token, UNDEF, is->buf); // no break here
              case VAR      : assignment (is); break;
              case COMMAND  : run_command (is, is->icommand); break;
              case EXT_COMMAND : run_function (is, is->ifunc); break;
            }
        }
    }
  while (tok != FIN && is->error == 0);

  return tok;
}

/**
 * assignment: In is->token should be stored name, in is->ivar - pointer to var.
 *
 **/
static void
assignment (IScript* is)
{
  int tok;
  IVar *right, *left;

  assert (is->ivar);
  left = is->ivar;

  idebug ("ASSIGNMENT:start:left %s\n", left->name);

  tok = get_token (is);
  if (tok != OP || is->iop != EQ)
    {
      ierror (is, IS_ERR_INVALID_OP);
      return;
    }

  right = eval_exp (is);
  set_var (left, right);

  idebug ("ASSIGNMENT:end:left %s(type %d)\n", left->name, left->type);
}

/**
 * eval_exp1: Evalutes complex exressions.
 *
 * @is: a #IScript
 *
 * Return value: Result of operation.
 *
 **/
static IVar*
eval_exp (IScript* is)
{
  IVar* ret;

  if (get_token (is) == FIN)
    {
      ierror (is, IS_ERR_UNEXP_END);
      return NULL;
    }

  ret = eval_exp1 (is);
  putback (is);

  return ret;
}

/**
 * eval_exp1: Evalutes conditional operations.
 *
 * @is: a #IScript
 *
 * Return value: Result of operation.
 *
 **/
static IVar*
eval_exp1 (IScript* is)
{
  int op;
  IVar *ret, *tmp;

  ret = eval_exp2 (is);

  op = is->iop;
  if (op <= EQ && op >= LT)
    {
      idebug ("EVAL_EXP1:start:%d op %d\n", is->token_type, op);

      get_token (is);
      tmp = eval_exp1 (is);

      return process_op (is, op, ret, tmp);
    }

  return ret;
}

/**
 * eval_exp2: Evalutes '+' and '-' operations.
 *
 * @is: a #IScript
 *
 * Return value: Result of operation.
 *
 **/
static IVar*
eval_exp2 (IScript* is)
{
  int op;
  IVar *ret, *tmp;

  ret = eval_exp3 (is);

  op = is->iop;
  while (op == '+' || op == '-')
    {
      idebug ("EVAL_EXP2:start:%d\n", is->token_type);

      get_token (is);
      tmp = eval_exp3 (is);

      return process_op (is, op, ret, tmp);
    }

  return ret;
}

/**
 * eval_exp3: Evalutes '*' and '/' operations.
 *
 * @is: a #IScript
 *
 * Return value: Result of operation.
 *
 **/
static IVar*
eval_exp3 (IScript* is)
{
  int op;
  IVar *ret, *tmp;

  ret = eval_exp4 (is);

  op = is->iop;
  while (op == '*' || op == '/')
    {
      get_token (is);
      tmp = eval_exp4 (is);

      return process_op (is, op, ret, tmp);
    }

  return ret;
}

/**
 * eval_exp4: Evalute '^' operation.
 *
 * @is: a #IScript
 *
 * Return value: Result of operation.
 *
 **/
static IVar*
eval_exp4 (IScript* is)
{
  IVar *ret; //, *tmp;

  //  idebug ("EVAL_EXP4:start:%d\n", is->token_type);

  ret = eval_exp5 (is);

  //if (is->token_type == OP && is->iop == '^')
  //  {
  //  }

  return ret;
}

/**
 * eval_exp5: Evalutes unary '-' and unary '+' operations.
 *
 * @is: a #IScript
 *
 * Return value: Result of operation.
 *
 **/
static IVar*
eval_exp5 (IScript* is)
{
  int op = 0;
  IVar *ret;

  if (is->token_type == OP && (is->iop == '+' || is->iop == '-'))
    {
      op = is->iop;
      get_token (is);
    }

  ret = eval_exp6 (is);

  if (op == '-' && ret != NULL)
    {
      if (ret->type == INT)
        {
          ret->ival = - ret->ival;
        }
      else
        {
	  idebug("Invalid op: %d", ret->type);
          ierror (is, IS_ERR_INVALID_OP);
        }
    }

  return ret;
}

/**
 * eval_exp6: Evalutes expressions in brackets '(' ')'.
 *
 * @is: a #IScript
 *
 * Return value: Result of operation.
 *
 **/
static IVar*
eval_exp6 (IScript* is)
{
  IVar* res;

  if (is->token_type == OP && is->iop == '(')
    {
      idebug ("EVAL_EXP6:start: %c\n", is->iop);

      get_token (is);
      res = eval_exp2 (is);
      if (is->token_type != OP || is->iop != ')')
        {
          ierror (is, IS_ERR_UNM_BRACKETS);
        }

      get_token (is);

      idebug ("EVAL_EXP6:end: %c\n", is->iop);
      return res;
    }
  else
    {
      return atom (is);
    }
}

/**
 * atom: Wrap a value or number in #IVar struct.
 *
 * @is: a #IScript
 *
 * Return value: a #IVar.
 *
 **/
static IVar*
atom (IScript* is)
{
  IVar* ret;

  idebug ("ATOM:start:%s (%d)\n", debug_tok_type[is->token_type], is->token_type);

  switch (is->token_type)
    {
      case VAR :
                 ret = is->ivar;
                 break;

      case NUMBER :
                 ret = create_itmp (is, is->inumber);
                 break;

      case STRING :
                 ret = create_stmp (is, is->token);
                 break;

      case EXT_VAR :
                 ret = is->ivar;
                 break;

      case COMMAND : {
                         if (is->icommand->mayret == RETURN)
                             ret = run_command (is, is->icommand);
                         else
                           {
                             ierror (is, IS_ERR_COM_NORET);
                             ret = NULL;
                           }
                     } break;
      
      case ID :  ierror (is, IS_ERR_UNDEF_ID); ret = NULL;

      default :
                 ierror (is, IS_ERR_SYNTAX);
                 return NULL;

    }

  get_token (is);

  idebug ("ATOM:end:type %d\n", ret ? ret->type : 0);

  return ret;
}

/**
 * parse_func: Parses function's call.
 *
 *
 * Return value: 1 if no errors occured,
 *               0 otherwise.
 **/
static int
parse_func (IScript* is, const IFunction* ifunc, int* argc, const char*** argv)
{
  IVar* exp;
  int   tok = 0;
  *argc = 0;

  idebug ("parse_func:start:%s\n", ifunc->name);

  do {
      tok = get_token (is);

      if (tok == EOL || tok == FIN)
          break;

      if (tok == STRING)
        {
          (*argv)[(*argc)++] = strdup (is->token);
          tok = get_token (is);
        } // STRING
      else
        {
          putback (is);
          exp = eval_exp (is);
          if (NULL != exp)
            {
              if (exp->type == STR)
                (*argv)[(*argc)++] = strdup (exp->sval);
              else
                (*argv)[(*argc)++] = conv_to_str (exp->ival);
            }
          else
              break; // Syntax error occured

          tok = get_token (is);
        } // EXPRESSION

      if (tok == DELIMITER)
        {
          if (*is->token != ',')
            {
              break;
            }
        } // DELIMITER

  } while (*argc < ifunc->nargs && tok != EOL && tok != FIN);

  idebug ("parse_func:parsed args %d, need %d\n", *argc, ifunc->nargs);

  if (*argc != ifunc->nargs)
    {
      if (tok == EOL && *argc < ifunc->nargs)
        {
          ierror (is, IS_ERR_FEW_PARMS);
        }
      else if (tok == DELIMITER && *is->token == ',')
        {
          ierror (is, IS_ERR_MANY_PARMS);
        }
      else
        {
          ierror (is, IS_ERR_SYNTAX);
        }
      skip_until_eol (is);
      return 0;
    }
  else
    {
      if (tok != EOL && tok != FIN)
        {
          if (tok == DELIMITER && *is->token == ',')
            {
              ierror (is, IS_ERR_MANY_PARMS);
            }
          else
            {
              ierror (is, IS_ERR_SYNTAX);
            }
          skip_until_eol (is);
          return 0;
        }
      return 1;
    }
}

/**
 * run_function: Runs specified function.
 *
 * Return value: Result returned by function.
 *
 **/
static IVar*
run_function (IScript* is, const IFunction* ifunc)
{
  int i, argc;
  const char** argv;
  IVar* ret = NULL;

  assert (ifunc);

  argv = malloc (sizeof (char*) * ifunc->nargs);
  for (i = 0; i < ifunc->nargs; i++)
      argv[i] = NULL;

  if (parse_func (is, ifunc, &argc, &argv))
    {
      void* result = NULL;
      (*ifunc->exec) (is, argc, argv, &result);
      if (result != NULL)
          ret = create_stmp (is, (char*)result);
    }

  for (i = 0; i < ifunc->nargs; i++)
      free ((char*)argv[i]);

  free (argv);
  
  return ret;
}

/**
 * com_if: Interprets the IF BASIC command.
 *
 * @is: A #IScript.
 *
 * Return value: None.
 *
 **/
static IVar*
com_if (IScript* is, int glob_skip_flow)
{
  int   tok;
  IVar* cond = NULL;
  int   loc_skip_flow = glob_skip_flow;
  
  if (! loc_skip_flow)
    {
      cond = eval_exp (is);

      if (cond == NULL || cond->type != INT)
        {
          ierror (is, IS_ERR_INVALID_COND);
          return NULL;
        }

      //IFC f = {cond->ival, 1};
      tok = get_token_non_eol (is);

      if (tok != THEN)
        {
          ierror (is, IS_ERR_THEN_EXP);
          return NULL;
        }

      loc_skip_flow = cond->ival == 0;

      idebug ("IF:THEN %s\n", loc_skip_flow ? "skip" : "run");
    }

  do
    {
      // Parsing then or else part
      tok = iscript_parse (is, loc_skip_flow);

      idebug ("IF:After then part\n");

      if (tok == ELSE && ! glob_skip_flow)
        {
          loc_skip_flow = cond->ival == 1;
        }

    } while (tok != FIN && tok != END);

//  if (tok == FIN)
//      ierror (is, IS_ERR_UNEXP_END);

  return NULL;
}

/**
 * com_rem: Interprets the REM BASIC command.
 *
 * @is: A #IScript.
 *
 * Return value: NULL.
 *
 **/

static IVar*
com_rem (IScript* is)
{
  skip_until_eol (is);

  return NULL;
}

/**
 * com_print: Interprets the PRINT BASIC command.
 *
 * @is: A #IScript.
 *
 * Return value: NULL.
 *
 **/
static IVar*
com_print (IScript* is)
{
  IVar *exp;
  int tok = 0;
  int last_delim = 0;

  idebug ("PRINT:start:\n");

  do {
      tok = get_token (is);

      if (tok == EOL || tok == FIN)
          break;

      if (tok == STRING)
        {
          print_string (is, is->token);
          tok = get_token (is);
        } // STRING
      else
        {
          putback (is);
          exp = eval_exp (is);
          if (NULL != exp)
            {
              print_var (is, exp);
            }
          else
              break; // Syntax error occured

          tok = get_token (is);
        } // EXPRESSION

      last_delim = *is->token;

      if (tok == DELIMITER)
        {
          if (*is->token == ',')
            {
              is->optr += sprintf (is->optr, "\t");
            }
          else if (*is->token == ';')
            {
              is->optr += sprintf (is->optr, " ");
            }
	  else if (*is->token == '.') //concatenation
            {
              is->optr += sprintf (is->optr, "");
            }
          else
            {
              break;
            }

        } // DELIMITER

  } while (tok == DELIMITER && (*is->token == ',' || *is->token == ';' || *is->token == '.' ));

  if (! (last_delim == ';' || last_delim == ',' || last_delim == '.' ))
    {
      idebug ("PRINT:Adding NL\n");
      is->optr += sprintf (is->optr, "\n");
    }

  idebug ("PRINT:end: %s (%d)\n", debug_tok_type[tok], tok);

  if (tok != EOL && tok != FIN)
    {
      ierror (is, IS_ERR_SYNTAX);
    }

  return NULL;
}

/**
 * print_string: Parses and prints given string to output buffer.
 *
 * @is:  A #IScript.
 * @str: String.
 *
 **/
static void
print_string (IScript* is, const char* str)
{
  const char* ptr = str;

  idebug ("print_string: '%s'\n", ptr);

  while (*ptr)
    {
      if (*ptr == '$')
        {
          int ret;
          char   var[IMAX_VAR_NAME_SIZE];
          size_t it = 1;

          *var = '$';
          for (++ptr; *ptr != ' ' && *ptr != '\t' && *ptr != '\0'
                        && it < IMAX_VAR_NAME_SIZE; ++it)
              var[it] = *ptr++;
        
          var[it] = '\0';

          ret = lookup_ext (is, var);

          if (ret == EXT_VAR)
            {
              print_var (is, is->ivar);
            }
          else
            {
              strcpy (is->optr, var);
              is->optr += strlen (var);
            }
        }
      else
        {
          *is->optr++ = *ptr++;
        }
    }
}

/**
 * print_var: Prints value of variable.
 *
 * @is:  A #IScript.
 * @var: Variable.
 *
 **/
static void
print_var (IScript* is, const IVar* var)
{
  if (var == NULL)
      return;

  idebug ("print_var:start:%s:type %d\n", var->name, var->type);

  if (var->type == INT)
    {
      is->optr += sprintf (is->optr, "%d", var->ival);
    }
  else if (var->type == STR)
    {
      is->optr += sprintf (is->optr, "%s", var->sval);
    }
  else
    {
      ierror (is, IS_ERR_INVALID_TYPE);
    }
}

/**
 * create_var: Creates a #IVar with given name, type and reference. Always return
 *             non-NULL. But when number of exist variables is greate than max.
 *             possible "behaviour is undefined". Enjoy!
 *
 * @is:     a #IScript.
 * @name:   Name.
 * @type:   Type of variable.
 * @ref:    Reference to source text.
 *
 * Return value: New variable.
 *
 **/
static IVar*
create_var (IScript* is, const char* name, int type, const char* ref)
{
  IVar* var;

  // Last variable reserved because this function must
  // return non-NULL Ivar.
  if (is->ivars_size >= (IMAX_VARS - 1))
    {
      ierror (is, IS_ERR_MANY_VARS);
      var = is->ivars + (IMAX_VARS - 1);
    }
  else
    {
      var = is->ivars + is->ivars_size++;
    }

  strncpy(var->name, name, IMAX_VAR_NAME_SIZE);
  var->type = type;
  var->sptr = (char*) ref;
  var->sval = NULL;

  return var;
}

/**
 * free_var: Frees memory occupied by var's internal data.
 *
 * @var: A #IVar;
 *
 **/
static void
free_var (IVar* var)
{
  free (var->sval);
  var->sval = NULL;
}

/**
 * set_var: Assign new value to exist variable. Types will be overrided.
 *
 * @dst: Destination variable.
 * @src: Source variable.
 *
 **/
static void set_var (IVar* dst, const IVar* src)
{
  int n, grab_num;
  char foo;

  if (dst == NULL || src == NULL)
      return;

  if (dst->sval != NULL)
    {
      free (dst->sval);
      dst->sval = NULL;
    }

  //if its a numerical source - we'll make the destination
  //variable a VAR for inline calculations
  if( src->sval != NULL )
  {
  	n = sscanf(src->sval, "%d%c", &grab_num, &foo );
  	//1 is a valid integer and the source is not an already assigned numerical VAR
  	if( n == 1 && src->type != 3 ) 
  	{
		dst->type = 1;
		dst->ival = grab_num;
		dst->sval = strdup (src->sval);
  	}
  	else
  	{
  		dst->type = src->type;
  		dst->ival = src->ival;
		dst->sval = strdup (src->sval);
  	}
  }
  else
  {
	dst->type = src->type;
        dst->ival = src->ival;
  }
}

/**
 * create_sconcat: Creates temp. string variable with concatenation's result.
 *
 * @is:   a #IScript.
 * @str1: First string.
 * @str2: Second string.
 *
 * Return value: Pointer to created variable.
 *
 **/
static IVar*
create_sconcat (IScript* is, const char* str1, const char* str2)
{
  IVar* tmp = create_tmp_var (is, STR);

  size_t sz = strlen (str1) + strlen (str2) + 1;

  tmp->sval = (char*) malloc (sz);
  strcpy (tmp->sval, str1);
  strcat (tmp->sval, str2);

  return tmp;
}

/**
 * run_command: Rund BASIC command.
 *
 * @is:   A #IScript.
 * @icom: Command to run.
 *
 * Return value: Return value of a command. NULL if this command not return anything.
 *
 **/
static IVar*
run_command (IScript* is, const ICommand* icom)
{
# ifdef ISCRIPT_DEBUG
  if (icom->exec == NULL)
    {
      idebug ("Attempt to run NULL command: %s\n", icom->name);
      exit (25);
    }
# else
  assert (icom->exec);
# endif
  return (icom->exec) (is);
}

/**
 * precess_op: Calculates result of binary operation with given values. If one of
 *             the operands is NULL then NULL will return. If operation and/or
 *             operand's types is incompatible then NULL will return too.
 *
 * @is: A #IScript.
 * @op: Binary operation.
 * @v1: Left operand.
 * @v2: Right operand.
 *
 * Return value: Pointer to the result.
 *
 **/
static IVar*
process_op (IScript* is, int op, const IVar* v1, const IVar* v2)
{
  if (NULL == v1 || NULL == v2)
      return NULL;

  idebug ("PROCESS_OP: %s(%d)=>%s %c %s(%d)=>%s\n", v1->name, v1->type, v1->sval, op, v2->name, v2->type, v2->sval);

  switch (op)
  {
      case LT : {
		    idebug("less than called for: %d %c %d", v1->ival, op, v2->ival);
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival < v2->ival);
                } break;

      case GT : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival > v2->ival);
                } break;

      case LE : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival <= v2->ival);
                } break;

      case GE : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival >= v2->ival);
                } break;

      case NE : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival != v2->ival);

                    if (v1->type == STR && v2->type == STR)
                        return create_itmp (is, strcmp (v1->sval, v2->sval));
                } break;

      case EQ : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival == v2->ival);

                    if (v1->type == STR && v2->type == STR)
                        return create_itmp (is, ! strcmp (v1->sval, v2->sval));
                } break;

      case '+' : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival + v2->ival);

                    if (v1->type == STR && v2->type == STR)
                        return create_sconcat (is, v1->sval, v2->sval);
                } break;

      case '-' : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival - v2->ival);
                } break;

      case '*' : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival * v2->ival);
                } break;

      case '/' : {
                    if (v1->type == INT && v2->type == INT)
                        return create_itmp (is, v1->ival / v2->ival);
                } break;

    } // switch

  ierror (is, IS_ERR_INVALID_OP);
  return NULL;
}

/**
 * conv_to_str: Converts integer to string.
 *
 * Return value: Newly allocated string.
 *
 **/
static char*
conv_to_str (int i)
{
  char str[IMAX_TOKEN_LEN];

  sprintf (str, "%d", i);

  return strdup (str);
}
