/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* iscript.c:                                                              *
*                2005 Shlykov Vasiliy ( vash@zmail.ru )                   *
*                                                                         *
*                Small BASIC interpreter.                                 *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef __ISCRIPT_H_
#define __ISCRIPT_H_

enum IErrMsg
{
    OK,			    // No error
    IS_ERR_SYNTAX,	    // Syntax error
    IS_ERR_UNDEF_ID,	    // Undefined identifier
    IS_ERR_UNEXP_END,	    // Unexpected end of programm
    IS_ERR_INVALID_OP,	    // Invalid operation
    IS_ERR_MANY_VARS,	    // Too many variables
    IS_ERR_UNDEF_EXT_ID,    // Undefined external variable
    IS_ERR_COM_NORET,	    // Command doesn't return a value
    IS_ERR_UNM_BRACKETS,    // Unmatched brackets
    IS_ERR_INVALID_TYPE,    // Invalid type of variable.
    IS_ERR_INV_NUMBER,	    // Invalid number format
    IS_ERR_INVALID_COND,    // Invalid condition
    IS_ERR_THEN_EXP,	    // THEN expected
    IS_ERR_UNEXP_END_IF,    // END without IF
    IS_ERR_FEW_PARMS,       // Too few parameters for function
    IS_ERR_MANY_PARMS,      // Too many parameters fot function
};

typedef struct _IScript             IScript;
typedef struct _IScriptExtFuncInfo  IScriptExtFuncInfo;

typedef void (*IErrHandler_t) (int line, int code, const char* msg, void* userdata);

typedef void (*IScriptExtFunc_t) (IScript* is, int argc, const char** argv, void** ret);

struct _IScriptExtFuncInfo
{
    char*            name;
    int              nargs;
    IScriptExtFunc_t exec;
    int              mayreturn;
};

IScript*
iscript_new (void);

void
iscript_init (IScript*);

void
iscript_set_err_handler (IScript*, IErrHandler_t, void* userdata);

void
iscript_clear (IScript*);

int
iscript_ext_var_add (IScript*, const char* name, const char* value);

int
iscript_ext_func_add (IScript*, IScriptExtFuncInfo*);

int
iscript_run (IScript*, const char* in, char* out, size_t out_size);

void
iscript_free (IScript*);


#endif

