/*******************************************************************************
** Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
**
** This library is free software; you can redistribute it and/or modify it under
** the terms of the GNU Lesser General Public License as published by the Free
** Software Foundation; either version 2.1 of the License, or (at your option)
** any later version.
**
** This library is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
** details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this library; if not, write to the Free Software Foundation, Inc.,
** 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

// The CommandObject

// types of results that can be returned in a CommandObject
enum cmd_result_type_enum {
  CMD_RESULT_NULL,
  CMD_RESULT_UINT,
  CMD_RESULT_INT,
  CMD_RESULT_FLOAT,
  CMD_RESULT_STRING,
  CMD_RESULT_RAWSTRING,
  CMD_RESULT_FUNCTION,
  CMD_RESULT_STATEMENT,
  CMD_RESULT_LINKEDOBJECT,
  CMD_RESULT_TITLE,
  CMD_RESULT_COLUMN_HEADER,
  CMD_RESULT_COLUMN_VALUES,
  CMD_RESULT_COLUMN_ENDER,
  CMD_RESULT_EXTENSION,
};

class CommandResult {
  cmd_result_type_enum Result_Type;

 private:
  CommandResult () {
    Result_Type = CMD_RESULT_NULL; }

 public:
  CommandResult (cmd_result_type_enum T) {
    Result_Type = T; }
  cmd_result_type_enum Type () {
    return Result_Type;
  }

  virtual void Value (char *&C) {
    C = NULL;
  }
  virtual std::string Form () {
    return std::string("");
  }
  virtual PyObject * pyValue () {
    return Py_BuildValue("");
  }
  virtual void Print (ostream &to, int64_t fieldsize=20, bool leftjustified=false) {
    to << (leftjustified ? std::setiosflags(std::ios::left) : std::setiosflags(std::ios::right))
       << std::setw(fieldsize) << "(none)";
  }
};

class CommandResult_Uint : public CommandResult {
  uint64_t uint_value;

 public:
  CommandResult_Uint () : CommandResult(CMD_RESULT_UINT) {
    uint_value = 0;
  }
  CommandResult_Uint (uint64_t U) : CommandResult(CMD_RESULT_UINT) {
    uint_value = U;
  }

  static void Accumulate_Uint (CommandResult_Uint *A, CommandResult_Uint *B) {
    A->uint_value += B->uint_value;
  }


  virtual void Value (uint64_t &U) {
    U = uint_value;
  };
  virtual std::string Form () {
    char s[20];
    sprintf ( s, "%lld", uint_value);
    return std::string (s);
  }
  virtual PyObject * pyValue () {
    return Py_BuildValue("l", uint_value);
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    to << (leftjustified ? std::setiosflags(std::ios::left) : std::setiosflags(std::ios::right))
       << std::setw(fieldsize) << uint_value;
  }
};

class CommandResult_Int : public CommandResult {
  int64_t int_value;

 public:
  CommandResult_Int () : CommandResult(CMD_RESULT_INT) {
    int_value = 0;
  }
  CommandResult_Int (CommandResult_Uint *U) : CommandResult(CMD_RESULT_INT) {
    uint64_t Uvalue;
    U->Value(Uvalue);
    int_value = Uvalue;
  }
  CommandResult_Int (int64_t I) : CommandResult(CMD_RESULT_INT) {
    int_value = I;
  }

  static void Accumulate_Int (CommandResult_Int *A, CommandResult_Int *B) {
    A->int_value += B->int_value;
  }


  virtual void Value (int64_t &I) {
    I = int_value;
  };
  virtual std::string Form () {
    char s[20];
    sprintf ( s, "%lld", int_value);
    return std::string (s);
  }
  virtual PyObject * pyValue () {
    return Py_BuildValue("l", int_value);
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    to << (leftjustified ? std::setiosflags(std::ios::left) : std::setiosflags(std::ios::right))
       << std::setw(fieldsize) << int_value;
  }
};

class CommandResult_Float : public CommandResult {
  double float_value;

 public:
  CommandResult_Float () : CommandResult(CMD_RESULT_FLOAT) {
    float_value = 0.0;
  }
  CommandResult_Float (CommandResult_Uint *U) : CommandResult(CMD_RESULT_FLOAT) {
    uint64_t Uvalue;
    U->Value(Uvalue);
    float_value = Uvalue;
  }
  CommandResult_Float (CommandResult_Int *I) : CommandResult(CMD_RESULT_FLOAT) {
    int64_t Ivalue;
    I->Value(Ivalue);
    float_value = Ivalue;
  }
  CommandResult_Float (double f) : CommandResult(CMD_RESULT_FLOAT) {
    float_value = f;
  }

  static void Accumulate_Float (CommandResult_Float *A, CommandResult_Float *B) {
    A->float_value += B->float_value;
  }


  virtual void Value (double &F) {
    F = float_value;
  };
  virtual std::string Form () {
    char F[20];
    F[0] = *("%"); // left justify in field
    sprintf(&F[1], "%lld.%lldf\0", OPENSS_VIEW_FIELD_SIZE, OPENSS_VIEW_PRECISION);

    char s[20];
    sprintf ( s, &F[0], float_value);
    return std::string (s);
  }
  virtual PyObject * pyValue () {
    return Py_BuildValue("d", float_value);
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    to << (leftjustified ? std::setiosflags(std::ios::left) : std::setiosflags(std::ios::right))
       << std::setw(fieldsize) << Form ();
  }
};

class CommandResult_String : public CommandResult {
  std::string string_value;

 public:
  CommandResult_String (std::string S) : CommandResult(CMD_RESULT_STRING) {
    string_value = S;
  }
  CommandResult_String (char *S) : CommandResult(CMD_RESULT_STRING) {
    string_value = std::string(S);
  }

  static void Accumulate_String (CommandResult_String *A, CommandResult_String *B) {
    A->string_value += B->string_value;
  }

  virtual void Value (std::string &S) {
    S = string_value;
  }
  virtual std::string Form () {
    return string_value;
  }
  virtual PyObject * pyValue () {
    return Py_BuildValue("s",string_value.c_str());
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    if (leftjustified) {
     // Left justification is only done on the last column of a report.
     // Don't truncate the string if it is bigger than the field size.
     // This is done to make sure everything gets printed.

      to << std::setiosflags(std::ios::left) << string_value;

     // If there is unused space in the field, pad with blanks.
      if ((string_value.length() < fieldsize) &&
          (string_value[string_value.length()-1] != *("\n"))) {
        for (int64_t i = string_value.length(); i < fieldsize; i++) to << " ";
      }

    } else {
     // Right justify the string in the field.
     // Don't let it exceed the size of the field.
     // Also, limit the size based on our internal buffer size.
      to << std::setiosflags(std::ios::right) << std::setw(fieldsize)
         << ((string_value.length() <= fieldsize) ? string_value : string_value.substr(0, fieldsize));
    }
  }
};

class CommandResult_RawString : public CommandResult {
  std::string string_value;

 public:
  CommandResult_RawString (std::string S) : CommandResult(CMD_RESULT_RAWSTRING) {
    string_value = S;
  }
  CommandResult_RawString (char *S) : CommandResult(CMD_RESULT_RAWSTRING) {
    string_value = std::string(S);
  }

  static void Accumulate_RawString (CommandResult_RawString *A, CommandResult_RawString *B) {
    A->string_value += B->string_value;
  }

  virtual void Value (std::string &S) {
    S = string_value;
  }
  virtual std::string Form () {
    return string_value;
  }
  virtual PyObject * pyValue () {
    return Py_BuildValue("s",string_value.c_str());
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    // Ignore fieldsize and leftjustified specifications and just dump the
    // the raw string to the output stream.
    to << string_value;
  }
};

class CommandResult_Function : public CommandResult, Function {

 public:

  CommandResult_Function (Function F) :
       Framework::Function(F),
       CommandResult(CMD_RESULT_FUNCTION) {
  }

  virtual void Value (std::string &F) {
    F = getName();
  };
  virtual std::string Form () {
    std::string S = getName();
    LinkedObject L = getLinkedObject();
    std::set<Statement> T = getDefinitions();

    S = S + "(" + ((OPENSS_VIEW_FULLPATH)
                      ? L.getPath()
                      : L.getPath().getBaseName());

    if (T.size() > 0) {
      std::set<Statement>::const_iterator ti;
      for (ti = T.begin(); ti != T.end(); ti++) {
        if (ti != T.begin()) {
          S += "  &...";
          break;
        }
        Statement s = *ti;
        char l[20];
        sprintf( &l[0], "%lld", (int64_t)s.getLine());
        S = S + ": "
              + ((OPENSS_VIEW_FULLPATH)
                      ? s.getPath()
                      : s.getPath().getBaseName())
              + "," + &l[0];
      }
    }
    S += ")";

    return S;
  }
  virtual PyObject * pyValue () {
    std::string F = Form ();
    return Py_BuildValue("s",F.c_str());
  }

  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    std::string string_value = Form ();
    if (leftjustified) {
     // Left justification is only done on the last column of a report.
     // Don't truncate the string if it is bigger than the field size.
     // This is done to make sure everything gets printed.

      to << std::setiosflags(std::ios::left) << string_value;

     // If there is unused space in the field, pad with blanks.
      if ((string_value.length() < fieldsize) &&
          (string_value[string_value.length()-1] != *("\n"))) {
        for (int64_t i = string_value.length(); i < fieldsize; i++) to << " ";
      }

    } else {
     // Right justify the string in the field.
     // Don't let it exceed the size of the field.
     // Also, limit the size based on our internal buffer size.
      to << std::setiosflags(std::ios::right) << std::setw(fieldsize)
         << ((string_value.length() <= fieldsize) ? string_value : string_value.substr(0, fieldsize));
    }
  }
};

class CommandResult_Statement : public CommandResult, Statement {

 public:

  CommandResult_Statement (Statement S) :
       Framework::Statement(S),
       CommandResult(CMD_RESULT_STATEMENT) {
  }

  virtual void Value (int64_t& L) {
    L = (int64_t)getLine();
  };
  virtual std::string Form () {
    char l[20];
    sprintf( &l[0], "%lld", (int64_t)getLine());
    std::string S = (OPENSS_VIEW_FULLPATH)
                      ? std::string(l) + " " + getPath()
                      : std::string(l) + " " + getPath().getBaseName();
    return S;
  }
  virtual PyObject * pyValue () {
    std::string S = Form ();
    return Py_BuildValue("s",S.c_str());
  }

  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    std::string string_value = Form ();
    if (leftjustified) {
     // Left justification is only done on the last column of a report.
     // Don't truncate the string if it is bigger than the field size.
     // This is done to make sure everything gets printed.

      to << std::setiosflags(std::ios::left) << string_value;

     // If there is unused space in the field, pad with blanks.
      if ((string_value.length() < fieldsize) &&
          (string_value[string_value.length()-1] != *("\n"))) {
        for (int64_t i = string_value.length(); i < fieldsize; i++) to << " ";
      }

    } else {
     // Right justify the string in the field.
     // Don't let it exceed the size of the field.
     // Also, limit the size based on our internal buffer size.
      to << std::setiosflags(std::ios::right) << std::setw(fieldsize)
         << ((string_value.length() <= fieldsize) ? string_value : string_value.substr(0, fieldsize));
    }
  }
};

class CommandResult_LinkedObject : public CommandResult, LinkedObject {

 public:

  CommandResult_LinkedObject (LinkedObject F) :
       Framework::LinkedObject(F),
       CommandResult(CMD_RESULT_LINKEDOBJECT) {
  }

  virtual void Value (std::string &L) {
    L = (OPENSS_VIEW_FULLPATH)
                      ? getPath()
                      : getPath().getBaseName();
  };
  virtual std::string Form () {
    std::string S = (OPENSS_VIEW_FULLPATH)
                      ? getPath()
                      : getPath().getBaseName();
    return S;
  }
  virtual PyObject * pyValue () {
    std::string F = Form ();
    return Py_BuildValue("s",F.c_str());
  }

  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    std::string string_value = Form ();
    if (leftjustified) {
     // Left justification is only done on the last column of a report.
     // Don't truncate the string if it is bigger than the field size.
     // This is done to make sure everything gets printed.

      to << std::setiosflags(std::ios::left) << string_value;

     // If there is unused space in the field, pad with blanks.
      if ((string_value.length() < fieldsize) &&
          (string_value[string_value.length()-1] != *("\n"))) {
        for (int64_t i = string_value.length(); i < fieldsize; i++) to << " ";
      }

    } else {
     // Right justify the string in the field.
     // Don't let it exceed the size of the field.
     // Also, limit the size based on our internal buffer size.
      to << std::setiosflags(std::ios::right) << std::setw(fieldsize)
         << ((string_value.length() <= fieldsize) ? string_value : string_value.substr(0, fieldsize));
    }
  }
};

class CommandResult_Title : public CommandResult {
  std::string string_value;

 public:
  CommandResult_Title (std::string S) : CommandResult(CMD_RESULT_TITLE) {
    string_value = S;
  }

  virtual void Value (std::string &S) {
    S = string_value;
  }
  virtual std::string Form () {
    return string_value;
  }
  virtual PyObject * pyValue () {
    return Py_BuildValue("s",string_value.c_str());
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    to << string_value;
  }
};

class CommandResult_Headers : public CommandResult {

 int64_t number_of_columns;
 std::list<CommandResult *> Headers;

 public:
  CommandResult_Headers () : CommandResult(CMD_RESULT_COLUMN_HEADER) {
    number_of_columns = 0;
  }
  void Add_Header (CommandResult *R) {
    number_of_columns++;
    Headers.push_back(R);
  }

  virtual void Value (std::list<CommandResult *> &R) {
    R = Headers;
  }
  virtual std::string Form () {
    std::string S;
    std::list<CommandResult *>::iterator cri = Headers.begin();
    for (cri = Headers.begin(); cri != Headers.end(); cri++) {
      std::string R = (*cri)->Form ();
      if (R.size () > OPENSS_VIEW_FIELD_SIZE) {
        R.resize (OPENSS_VIEW_FIELD_SIZE);
      } else if (R.size () < OPENSS_VIEW_FIELD_SIZE) {
        std::string T;
        T.resize ((OPENSS_VIEW_FIELD_SIZE - R.size ()), *" ");
        R = T + R;
      }
      if (S.size() > 0) S.append (std::string("  ")); // 2 spaces between string
      S += R;
    }
    return S;
  }
  virtual PyObject * pyValue () {
    PyObject *p_object = PyList_New(0);
    std::list<CommandResult *>::iterator cri = Headers.begin();
    for (cri = Headers.begin(); cri != Headers.end(); cri++) {
      PyObject *p_item = (*cri)->pyValue ();
      PyList_Append(p_object,p_item);
    }
    return p_object;
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    std::list<CommandResult *>::iterator coi;
    int64_t num_results = 0;
    for (coi = Headers.begin(); coi != Headers.end(); coi++) {
      if (num_results++ != 0) to << "  ";
      (*coi)->Print (to, fieldsize, (num_results >= number_of_columns) ? true : false);
    }

  }
};

class CommandResult_Enders : public CommandResult {

 int64_t number_of_columns;
 std::list<CommandResult *> Enders;

 public:
  CommandResult_Enders () : CommandResult(CMD_RESULT_COLUMN_ENDER) {
    number_of_columns = 0;
  }
  void Add_Ender (CommandResult *R) {
    number_of_columns++;
    Enders.push_back(R);
  }

  virtual void Value (std::list<CommandResult *> &R) {
    R = Enders;
  }
  virtual std::string Form () {
    std::string S;
    std::list<CommandResult *>::iterator cri = Enders.begin();
    for (cri = Enders.begin(); cri != Enders.end(); cri++) {
      std::string R = (*cri)->Form ();
      if (R.size () > OPENSS_VIEW_FIELD_SIZE) {
        R.resize (OPENSS_VIEW_FIELD_SIZE);
      } else if (R.size () < OPENSS_VIEW_FIELD_SIZE) {
        std::string T;
        T.resize ((OPENSS_VIEW_FIELD_SIZE - R.size ()), *" ");
        R = T + R;
      }
      if (S.size() > 0) S.append (std::string("  ")); // 2 spaces between string
      S += R;
    }
    return S;
  }
  virtual PyObject * pyValue () {
    PyObject *p_object = PyList_New(0);
    std::list<CommandResult *>::iterator cri = Enders.begin();
    for (cri = Enders.begin(); cri != Enders.end(); cri++) {
      PyObject *p_item = (*cri)->pyValue ();
      PyList_Append(p_object,p_item);
    }
    return p_object;
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    std::list<CommandResult *>::iterator coi;
    int64_t num_results = 0;
    for (coi = Enders.begin(); coi != Enders.end(); coi++) {
      if (num_results++ != 0) to << "  ";
      (*coi)->Print (to, fieldsize, (num_results >= number_of_columns) ? true : false);
    }

  }
};

class CommandResult_Columns : public CommandResult {

 int64_t number_of_columns;
 std::list<CommandResult *> Columns;

 public:
  CommandResult_Columns (int64_t C = 0) : CommandResult(CMD_RESULT_COLUMN_VALUES) {
    number_of_columns = 0;
  }
  void Add_Column (CommandResult *R) {
    number_of_columns++;
    Columns.push_back(R);
  }

  virtual void Value (std::list<CommandResult *>  &R) {
    R = Columns;
  }
  virtual std::string Form () {
    std::string S;
    std::list<CommandResult *>::iterator cri = Columns.begin();
    for (cri = Columns.begin(); cri != Columns.end(); cri++) {
      std::string R = (*cri)->Form ();
      if (R.size () > OPENSS_VIEW_FIELD_SIZE) {
        R.resize (OPENSS_VIEW_FIELD_SIZE);
      } else if (R.size () < OPENSS_VIEW_FIELD_SIZE) {
        std::string T;
        T.resize ((OPENSS_VIEW_FIELD_SIZE - R.size ()), *" ");
        R = T + R;
      }
      if (S.size() > 0) S.append (std::string("  ")); // 2 spaces between string
      S += R;
    }
    return S;
  }
  virtual PyObject * pyValue () {
    PyObject *p_object = PyList_New(0);
    std::list<CommandResult *>::iterator cri = Columns.begin();
    for (cri = Columns.begin(); cri != Columns.end(); cri++) {
      PyObject *p_item = (*cri)->pyValue ();
      PyList_Append(p_object,p_item);
    }
    return p_object;
  }
  virtual void Print (ostream &to, int64_t fieldsize, bool leftjustified) {
    std::list<CommandResult *>::iterator coi;
    int64_t num_results = 0;
    for (coi = Columns.begin(); coi != Columns.end(); coi++) {
      if (num_results++ != 0) to << "  ";
      (*coi)->Print (to, fieldsize, (num_results >= number_of_columns) ? true : false);
    }

  }
};

inline bool CommandResult_lt (CommandResult *lhs, CommandResult *rhs) {
  Assert (lhs->Type() == rhs->Type());
  switch (lhs->Type()) {
   case CMD_RESULT_UINT:
    uint64_t Uvalue1, Uvalue2;
    ((CommandResult_Uint *)lhs)->Value(Uvalue1);
    ((CommandResult_Uint *)rhs)->Value(Uvalue2);
    return Uvalue1 < Uvalue2;
   case CMD_RESULT_INT:
    int64_t Ivalue1, Ivalue2;
    ((CommandResult_Int *)lhs)->Value(Ivalue1);
    ((CommandResult_Int *)rhs)->Value(Ivalue2);
    return Ivalue1 < Ivalue2;
   case CMD_RESULT_FLOAT:
    double Fvalue1, Fvalue2;
    ((CommandResult_Float *)lhs)->Value(Fvalue1);
    ((CommandResult_Float *)rhs)->Value(Fvalue2);
    return Fvalue1 < Fvalue2;
  }
  return false;
}

inline bool CommandResult_gt (CommandResult *lhs, CommandResult *rhs) {
  Assert (lhs->Type() == rhs->Type());
  switch (lhs->Type()) {
   case CMD_RESULT_UINT:
    uint64_t Uvalue1, Uvalue2;
    ((CommandResult_Uint *)lhs)->Value(Uvalue1);
    ((CommandResult_Uint *)rhs)->Value(Uvalue2);
    return Uvalue1 > Uvalue2;
   case CMD_RESULT_INT:
    int64_t Ivalue1, Ivalue2;
    ((CommandResult_Int *)lhs)->Value(Ivalue1);
    ((CommandResult_Int *)rhs)->Value(Ivalue2);
    return Ivalue1 > Ivalue2;
   case CMD_RESULT_FLOAT:
    double Fvalue1, Fvalue2;
    ((CommandResult_Float *)lhs)->Value(Fvalue1);
    ((CommandResult_Float *)rhs)->Value(Fvalue2);
    return Fvalue1 > Fvalue2;
  }
  return false;
}

inline void Accumulate_CommandResult (CommandResult *A, CommandResult *B) {
  if (!A || !B) return;
  Assert (A->Type() == B->Type());
  switch (A->Type()) {
  case CMD_RESULT_UINT:
    CommandResult_Uint::Accumulate_Uint ((CommandResult_Uint *)A, (CommandResult_Uint *)B);
    break;
  case CMD_RESULT_INT:
    CommandResult_Int::Accumulate_Int ((CommandResult_Int *)A, (CommandResult_Int *)B);
    break;
  case CMD_RESULT_FLOAT:
    CommandResult_Float::Accumulate_Float ((CommandResult_Float *)A, (CommandResult_Float *)B);
    break;
  case CMD_RESULT_STRING:
    CommandResult_String::Accumulate_String ((CommandResult_String *)A, (CommandResult_String *)B);
    break;
  case CMD_RESULT_RAWSTRING:
    CommandResult_RawString::Accumulate_RawString ((CommandResult_RawString *)A, (CommandResult_RawString *)B);
    break;
  }
}

inline CommandResult *Calculate_Percent (CommandResult *A, CommandResult *B) {

  if (B == NULL) {
    return NULL;
  }
  if (A == NULL) {
    return new CommandResult_Float (0.0);
  }

  double Avalue;
  double Bvalue;

  switch (A->Type()) {
   case CMD_RESULT_UINT:
    uint64_t Uvalue;
    ((CommandResult_Uint *)A)->Value(Uvalue);
    Avalue = Uvalue;
    break;
   case CMD_RESULT_INT:
    int64_t Ivalue;
    ((CommandResult_Int *)A)->Value(Ivalue);
    Avalue = Ivalue;
    break;
   case CMD_RESULT_FLOAT:
    ((CommandResult_Float *)A)->Value(Avalue);
    break;
  }
  switch (B->Type()) {
   case CMD_RESULT_UINT:
    uint64_t Uvalue;
    ((CommandResult_Uint *)B)->Value(Uvalue);
    Bvalue = Uvalue;
    break;
   case CMD_RESULT_INT:
    int64_t Ivalue;
    ((CommandResult_Int *)B)->Value(Ivalue);
    Bvalue = Ivalue;
    break;
   case CMD_RESULT_FLOAT:
    ((CommandResult_Float *)B)->Value(Bvalue);
    break;
  }

  if (Bvalue <= 0.0) {
    return NULL;
  }

  double percent = (Avalue *100) / Bvalue;
  // if (percent > 100.0) percent = 100.0;
  if (percent < 0.0) percent = 0.0;
  return new CommandResult_Float (percent);
}

// Overloaded utility that will generate the right CommandResult object.
inline CommandResult *CRPTR (uint64_t& V) { return new CommandResult_Uint (V); }
inline CommandResult *CRPTR (int64_t& V) { return new CommandResult_Int (V); }
inline CommandResult *CRPTR (double& V) { return new CommandResult_Float (V); }
inline CommandResult *CRPTR (std::string& V) { return new CommandResult_String (V); }
inline CommandResult *CRPTR (Function& V) { return new CommandResult_Function (V); }
inline CommandResult *CRPTR (Statement& V) { return new CommandResult_Statement (V); }
inline CommandResult *CRPTR (LinkedObject& V) { return new CommandResult_LinkedObject (V); }


enum Command_Status
{
  CMD_UNKNOWN,
  CMD_PARSED,
  CMD_EXECUTING,
  CMD_COMPLETE,
  CMD_ERROR,
  CMD_ABORTED 
};

class CommandObject
{
  InputLineObject *Associated_Clip; // The input line that caused generation of this object.
  int64_t Seq_Num; // The order this object was generated in from the input line.
  Command_Status Cmd_Status;
  oss_cmd_enum Cmd_Type; // A copy of information in the Parse_Result.
  OpenSpeedShop::cli::ParseResult *PR;
  bool result_needed_in_python;  // Don't Print to ostream if this is set!
  bool results_used; // Once used, this object can be deleted!
  std::list<CommandResult *> CMD_Result;
  std::list<CommandResult_RawString *> CMD_Annotation;
  pthread_cond_t wait_on_dependency;

  void Associate_Input ()
  {
    Associated_Clip = Current_ILO;
    Link_Cmd_Obj_to_Input (Associated_Clip, this);
  }

  void Attach_Result (CommandResult *R) {
    CMD_Result.push_back(R);
  }

public:
  CommandObject()
  {
    Associated_Clip = NULL;
    Seq_Num = 0;
    Cmd_Status = CMD_ERROR;
    Cmd_Type =  CMD_HEAD_ERROR;
    PR = NULL;
    result_needed_in_python = false;
    results_used = false;
    pthread_cond_init(&wait_on_dependency, (pthread_condattr_t *)NULL);
  }
  CommandObject(OpenSpeedShop::cli::ParseResult *pr, bool use_by_python)
  {
    this->Associate_Input ();
    Cmd_Status = CMD_PARSED;
    Cmd_Type =  pr->getCommandType();
    PR = pr;
    result_needed_in_python = use_by_python;
    results_used = false;
    pthread_cond_init(&wait_on_dependency, (pthread_condattr_t *)NULL);
  }
  ~CommandObject() {
   // Destroy ParseResult object
    delete PR;

   // Reclaim results
    std::list<CommandResult *>::iterator cri;
    for (cri = CMD_Result.begin(); cri != CMD_Result.end(); ) {
      CommandResult *CR = (*cri);
      cri++;
      delete CR;
    }

   // Reclaim annotations.
    std::list<CommandResult_RawString *>::iterator crri;
    for (crri = CMD_Annotation.begin(); crri != CMD_Annotation.end(); ) {
      CommandResult_RawString *CR = (*crri);
      crri++;
      delete CR;
    }

   // Safety check.
    pthread_cond_destroy (&wait_on_dependency);
  }

  InputLineObject *Clip () { return Associated_Clip; }
  Command_Status Status () { return Cmd_Status; }
  oss_cmd_enum Type () { return Cmd_Type; }
  bool Needed_By_Python () { return result_needed_in_python; }
  bool Results_Used () { return results_used; }
  OpenSpeedShop::cli::ParseResult *P_Result () { return PR; }
  // command_t *P_Result () { return Parse_Result; }
  //command_type_t *P_Result () { return Parse_Result; }

  void Wait_On_Dependency (pthread_mutex_t &exp_lock) {
   // Suspend processing of the command.

   // Release the lock and wait for the all-clear signal.
    Assert(pthread_cond_wait(&wait_on_dependency,&exp_lock) == 0);

   // Release the recently acquired lock and continue processing the command.
    Assert(pthread_mutex_unlock(&exp_lock) == 0);
  }
  void All_Clear () {
   // Release the suspended command.
    Assert(pthread_cond_signal(&wait_on_dependency) == 0);
  }
    
  void SetSeqNum (int64_t a) { Seq_Num = a; }
  void set_Status (Command_Status S); // defined in CommandObject.cxx
  void set_Results_Used () { results_used = true; }

  void Result_Uint (uint64_t U) {
    Attach_Result (new CommandResult_Uint(U));
  }
  void Result_Int (int64_t I) {
    Attach_Result (new CommandResult_Int(I));
  }
  void Result_Float (double F) {
    Attach_Result (new CommandResult_Float(F));
  }
  void Result_String (std::string S) {
    Attach_Result (new CommandResult_String (S));
  }
  void Result_String (char *C) {
    Attach_Result (new CommandResult_String (C));
  }
  void Result_RawString (std::string S) {
    Attach_Result (new CommandResult_RawString (S));
  }
  void Result_RawString (char *C) {
    Attach_Result (new CommandResult_RawString (C));
  }
  void Result_Predefined (CommandResult *C) {
    Attach_Result (C);
  }

  std::list<CommandResult *> Result_List () {
    return CMD_Result;
  }

  void Result_Annotation (std::string S) {
    CMD_Annotation.push_back(new CommandResult_RawString (S));
  }

  std::list<CommandResult_RawString *> Annotation_List () {
    return CMD_Annotation;
  }

 // The following are defined in CommandObject.cxx

 // The simple Print is for dumping information to a trace file.
  void Print (ostream &mystream);
 // The Print_Results routine is for sending results to the user.
 // The result returned is "true" if there was information printed.
  bool Print_Results (ostream &to, std::string list_seperator, std::string termination_char);
};
