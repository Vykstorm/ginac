#include "G__ci.h"   /* G__atpause is defined in G__ci.h */
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "ginac/ginac.h"
#include "config.h"
#include <list>

#ifndef NO_NAMESPACE_GINAC
using namespace GiNaC;
#endif // ndef NO_NAMESPACE_GINAC

extern "C" G__value G__exec_tempfile G__P((char *file));
extern "C" void G__store_undo_position(void);

#define PROMPT1 "GiNaC> "
#define PROMPT2 "     > "

#ifdef OBSCURE_CINT_HACK

#include <strstream>

template<class T>
string ToString(const T & t)
{
    char buf[256];
    ostrstream(buf,sizeof(buf)) << t << ends;
    return buf;
}

basic * ex::last_created_or_assigned_bp=0;
basic * ex::dummy_bp=0;
long ex::last_created_or_assigned_exp=0;

#endif // def OBSCURE_CINT_HACK

G__value exec_tempfile(string const & command);
char * process_permanentfile(string const & command);
void process_tempfile(string const & command);
void greeting(void);
void helpmessage(void);
string preprocess(char const * const line, bool & comment, bool & single_quote,
                  bool & double_quote, unsigned & open_braces);
void cleanup(void);
void sigterm_handler(int n);
void initialize(void);
bool readlines(istream * is, string & allcommands);
bool readfile(string const & filename, string & allcommands);
void savefile(string const & filename, string const & allcommands);

typedef list<char *> cplist;
cplist filenames;

G__value exec_tempfile(string const & command)
{
    G__value retval;
    char *tmpfilename = tempnam(NULL,"ginac");
    ofstream fout;
    fout.open(tmpfilename);
    fout << "{" << endl << command << endl << "}" << endl;
    fout.close();
    G__store_undo_position();
    retval = G__exec_tempfile(tmpfilename);
    G__security_recover(stdout);
    remove(tmpfilename);
    free(tmpfilename);
    return retval;
}

char * process_permanentfile(string const & command)
{
    char *tmpfilename = tempnam(NULL,"ginac");
    cout << "creating file " << tmpfilename << endl;
    ofstream fout;
    fout.open(tmpfilename);
    fout << command << endl;
    fout.close();
    G__store_undo_position();
    G__loadfile(tmpfilename);
    G__security_recover(stdout);
    return tmpfilename;
}

void process_tempfile(string const & command)
{
#ifdef OBSCURE_CINT_HACK
    static G__value ref_symbol = exec_tempfile("symbol ginac_cint_internal_symbol; ginac_cint_internal_symbol;");
    static G__value ref_constant = exec_tempfile("constant ginac_cint_internal_constant; ginac_cint_internal_constant;");
    static G__value ref_function = exec_tempfile("sin(ginac_cint_internal_symbol);");
    static G__value ref_power = exec_tempfile("power(ex(ginac_cint_internal_symbol),ex(ginac_cint_internal_symbol));");
    static G__value ref_numeric = exec_tempfile("numeric ginac_cint_internal_numeric; ginac_cint_internal_numeric;");
    static G__value ref_ex = exec_tempfile("ex ginac_cint_internal_ex; ginac_cint_internal_ex;");
    static bool basic_type_warning_already_displayed=false;
#endif // def OBSCURE_CINT_HACK

    G__value retval = exec_tempfile(command);

#ifdef OBSCURE_CINT_HACK

    #define TYPES_EQUAL(A,B) (((A).type==(B).type) && ((A).tagnum==(B).tagnum))
    
    static unsigned out_count = 0;
    if (TYPES_EQUAL(retval,ref_ex)) {
        string varname = "Out"+ToString(++out_count);
        if (retval.obj.i!=ex::last_created_or_assigned_exp) {
            // an ex was returned, but this is not the ex which was created last
            // => this is not a temporary ex, but one that resides safely in memory
            
            // cout << "warning: using ex from retval (experimental)" << endl;
            ex::dummy_bp=((ex *)(void *)(retval.obj.i))->bp;
            exec_tempfile("ex "+varname+"(*ex::dummy_bp);");
        } else if (ex::last_created_or_assigned_bp_can_be_converted_to_ex()) {
            //string varfill;
            //for (int i=4-int(log10(out_count)); i>0; --i)
            //    varfill += ' ';
            exec_tempfile("ex "+varname+"(*ex::last_created_or_assigned_bp);");
        } else {
            cout << "warning: last_created_or_assigned_bp modified 0 or not evaluated or not dynallocated" << endl;
        }
        exec_tempfile(string()+"LLLAST=LLAST;\n"
                      +"LLAST=LAST;\n"
                      +"LAST="+varname+";\n"
                      +"cout << \""+varname+" = \" << "+varname+" << endl << endl;");
    } else if (TYPES_EQUAL(retval,ref_symbol)||
               TYPES_EQUAL(retval,ref_constant)||
               TYPES_EQUAL(retval,ref_function)||
               TYPES_EQUAL(retval,ref_power)||
               TYPES_EQUAL(retval,ref_numeric)) {
        if (!basic_type_warning_already_displayed) {
	    cout << endl
                 <<"WARNING: The return value of the last expression you entered was a symbol," << endl
                 << "constant, function, power or numeric, which cannot be safely displayed." << endl
                 << "To force the output, cast it explicitly to type 'ex' or use 'cout'," << endl
                 << "for example (assume 'x' is a symbol):" << endl
                 << PROMPT1 "ex(x);" << endl
                 << "OutX = x" << endl << endl
                 << PROMPT1 "cout << x << endl;" << endl
                 << "x" << endl << endl
                 << "This warning will not be shown again." << endl;
            basic_type_warning_already_displayed=true;
        }
    }
#endif // def OBSCURE_CINT_HACK
}

void greeting(void)
{
    cout << "Welcome to GiNaC-cint (" << PACKAGE << " V" << VERSION << ")" << endl;
    cout << "This software is provided \"as is\" without any warranty.  Copyright of Cint is" << endl
         << "owned by Agilent Technologies Japan and Masaharu Goto.  Registration is" << endl
         << "  __,  _______  requested, at this moment, for commercial use.  Send e-mail to" << endl
         << " (__) *       | <MXJ02154@niftyserve.or.jp>.  The registration is free." << endl
         << "  ._) i N a C | The GiNaC framework is Copyright by Johannes Gutenberg Univ.," << endl
         << "<-------------' Germany and licensed under the terms and conditions of the GPL." << endl
         << "Type .help for help." << endl
         << endl;
}

void helpmessage(void)
{
    cout << "GiNaC-cint recognizes some special commands which start with a dot:" << endl << endl
         << "  .q, .quit, .exit, .bye   quit GiNaC-cint" << endl
         << "  .help                    the text you are currently reading" << endl
         << "  .function                define the body of a function (necessary due to a" << endl
         << "                           cint limitation)" << endl
         << "  .cint                    switch to cint interactive mode (see cint" << endl
         << "                           documentation for further details)" << endl
         << "  .read filename           read a file from disk and execute it in GiNaC-cint" << endl
         << "                           (recursive call is possible)" << endl
         << "  .save filename           save the commands you have entered so far in a file" << endl << endl
         << "Additionally you can exit GiNaC-cint with quit; exit; or bye;" << endl
         << endl;
}

string preprocess(char const * const line, bool & comment, bool & single_quote,
                  bool & double_quote, unsigned & open_braces)
{
    // "preprocess" the line entered to be able to decide if the command shall be
    // executed directly or more input is needed or this is a special command

    // all whitespace will be removed
    // all comments (/* */ and //) will be removed
    // open and close braces ( { and } ) outside strings will be counted 

    /*
    cout << "line=" << line << endl;
    cout << "comment=" << comment << ", single_quote=" << single_quote
         << ",double_quote=" << double_quote << ", open_braces=" << open_braces
         << endl;
    */
    
    string preprocessed;
    int pos=0;
    bool end=false;
    bool escape=false;
    bool slash=false;
    bool asterisk=false;
    while ((line[pos]!='\0')&&!end) {
        if (escape) {
            // last character was a \, ignore this one
            escape=false;
        } else if (slash) {
            // last character was a /, test if * or /
            slash=false;
            if (line[pos]=='/') {
                end=true;
            } else if (line[pos]=='*') {
                comment=true;
            } else {
                preprocessed += '/';
                preprocessed += line[pos];
            }
        } else if (asterisk) {
            // last character was a *, test if /
            asterisk=false;
            if (line[pos]=='/') {
                comment=false;
            } else if (line[pos]=='*') {
                preprocessed += '*';
                asterisk=true;
            }
        } else {
            switch (line[pos]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                // whitespace: ignore
                break;
            case '\\':
                // escape character, ignore next
                escape=true;
                break;
            case '"':
                if ((!single_quote)&&(!comment)) {
                    double_quote = !double_quote;
                }
                break;
                case '\'':
                    if ((!double_quote)&&(!comment)) {
                        single_quote = !single_quote;
                    }
                    break;
            case '{':
                if ((!single_quote)&&(!double_quote)&&(!comment)) {
                    open_braces++;
                }
                break;
            case '}':
                if ((!single_quote)&&(!double_quote)&&(!comment)&&(open_braces>0)) {
                    open_braces--;
                }
                break;
            case '/':
                slash=true;
                break;
            case '*':
                asterisk=true;
                break;
            default:
                preprocessed += line[pos];
            }
        }
        pos++;
    }

    /*
    cout << "preprocessed=" << preprocessed << endl;
    cout << "comment=" << comment << ", single_quote=" << single_quote
         << ",double_quote=" << double_quote << ", open_braces=" << open_braces
         << endl;
    */
    
    return preprocessed;
}

void cleanup(void)
{
    for (cplist::iterator it=filenames.begin(); it!=filenames.end(); ++it) {
        cout << "removing file " << *it << endl;
        remove(*it);
        free(*it);
    }
}

void sigterm_handler(int n)
{
    exit(1);
}

void initialize(void)
{
    if (isatty(0))
        greeting();

    atexit(cleanup);
    signal(SIGTERM,sigterm_handler);
    
    G__init_cint("cint");    /* initialize cint */

    // no longer needed as of cint 5.14.31
    // exec_tempfile("#include <string>\n");

#ifndef NO_NAMESPACE_GINAC
    exec_tempfile("using namespace GiNaC;");
#endif // ndef NO_NAMESPACE_GINAC
    
    exec_tempfile("ex LAST,LLAST,LLLAST;\n");
}    

bool readlines(istream * is, string & allcommands)
{
    char const * line;
    char prompt[G__ONELINE];
    string linebuffer;
    
    bool quit = false;
    bool eof = false;
    bool next_command_is_function=false;
    bool single_quote=false;
    bool double_quote=false;
    bool comment=false;
    unsigned open_braces=0;

    while ((!quit)&&(!eof)) {
        strcpy(prompt,PROMPT1);
        bool end_of_command = false;
        string command;
        string preprocessed;
        while (!end_of_command) {
            if (is==NULL) {
                line = G__input(prompt);
            } else {
                getline(*is,linebuffer);
                line=linebuffer.c_str();
            }
            command += line;
            command += "\n";
            preprocessed += preprocess(line,comment,single_quote,double_quote,open_braces);
            if ((open_braces==0)&&(!single_quote)&&(!double_quote)&&(!comment)) {
                unsigned l=preprocessed.length();
                if ((l==0)||
                    (preprocessed[0]=='#')||
                    (preprocessed[0]=='.')||
                    (preprocessed[l-1]==';')||
                    (preprocessed[l-1]=='}')) {
                    end_of_command=true;
                }
            }
            strcpy(prompt,PROMPT2);
        }
        if ((preprocessed=="quit;")||
            (preprocessed=="exit;")||
            (preprocessed=="bye;")||
            (preprocessed==".q")||
            (preprocessed==".quit")||
            (preprocessed==".exit")||
            (preprocessed==".bye")) {
            quit = true;
        } else if (preprocessed==".function") {
            cout << "next expression can be a function definition" << endl;
            next_command_is_function=true;
        } else if (preprocessed==".cint") {
            cout << endl << "switching to cint interactive mode" << endl;
            cout << "'h' for help, 'q' to quit, '{ statements }' or 'p [expression]' to evaluate" << endl;
            G__pause();
            cout << "back from cint" << endl;
        } else if (preprocessed==".help") {
            helpmessage();
        } else if (preprocessed.substr(0,5)==".read") {
            quit=readfile(preprocessed.substr(5),allcommands);
        } else if (preprocessed.substr(0,5)==".save") {
            command = "// "+command; // we do not want the .save command itself in saved files
            savefile(preprocessed.substr(5),allcommands);
        /* test for more special commands
        } else if (preprocessed==".xyz") {
            cout << "special command (TBD): " << command << endl;
        */
        } else {
            // cout << "now processing: " << command << endl;
            if (next_command_is_function) {
                next_command_is_function = false;
                filenames.push_back(process_permanentfile(command));
            } else {
                process_tempfile(command);
            }
        }
        if (is!=NULL) {
            // test for end of file if reading from a stream
            eof=is->eof();
        } else {
            // save commands only when reading from keyboard
            allcommands += command;
        }

    }
    return quit;
}    

bool readfile(string const & filename, string & allcommands)
{
    cout << "Reading commands from file " << filename << "." << endl;
    bool quit=false;
    ifstream fin;
    fin.open(filename.c_str());
    if (fin.good()) {
        quit=readlines(&fin,allcommands);
    } else {
        cout << "Cannot open " << filename << " for reading." << endl;
    }
    fin.close();
    return quit;
}

void savefile(string const & filename, string const & allcommands)
{
    cout << "Saving commands to file " << filename << "." << endl;
    ofstream fout;
    fout.open(filename.c_str());
    if (fout.good()) {
        fout << allcommands;
        if (!fout.good()) {
            cout << "Cannot save commands to " << filename << "." << endl;
        }
    } else {
        cout << "Cannot open " << filename << " for writing." << endl;
    }
    fout.close();
}

int main(int argc, char ** argv) 
{
    string allcommands;
    initialize();

    bool quit=false;
    bool argsexist=argc>1;

    if (argsexist) {
        allcommands="/* Files given as command line arguments:\n";
    }
    
    argc--; argv++;
    while (argc && !quit) {
        allcommands += *argv;
        allcommands += "\n";
        quit=readfile(*argv,allcommands);
        argc--; argv++;
    }

    if (argsexist) {
        allcommands += "*/\n";
    }

    if (!quit) {
        readlines(NULL,allcommands);
    }

    return 0;
}







