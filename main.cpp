#include <iostream>
#include <stdio.h>
#include <utility>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cctype>
#include <variant>


using namespace std;


enum Label {
	op,
	int_const,
	identifier,
	float_const,
};

enum EntryType {INTEGER,FLOATING};
struct TableEntry {
	bool is_identifier;
	EntryType type;
	union {int i_val;float f_val;} value;
	string name;
};

unordered_map<string, TableEntry> lookupTable;


struct MkSnode {
	Label label;
	EntryType actualType;
	EntryType computedType;
	string content;
	MkSnode* left_child;
	MkSnode* right_child;
	MkSnode(const Label lb, string cn, MkSnode* left, MkSnode* right)
		: label(lb), content(std::move(cn)), left_child(left), right_child(right) {}
};

/* Global declarations */
/* Variables */
int charClass;
char lexeme [100];

//for the current line being parsed
string currentLine;
int idx = 0;

char nextChar;
int lexLen;
int token;
int nextToken;
FILE *infp;
/* Function declarations */
void addChar();
void getChar();
void getNonBlank();
int lex();
void program(ifstream &infile);
void print(unordered_map<string, TableEntry> table);
MkSnode* assign_list();
void declare_list(bool isInt);
TableEntry declare(bool isInt);
MkSnode* assign();
MkSnode* term();
MkSnode* expr();
MkSnode* factor();
MkSnode* error();
/* Character classes */
#define LETTER 0
#define DIGIT 1
#define UNKNOWN 99
/* Token codes */
#define INTLIT 10
#define IDENT 11
#define FLOATLIT 12
#define ASSIGNOP 20
#define ADDOP 21
#define SUBOP 22
#define MULTOP 23
#define DIVOP 24
#define LEFTPAREN 25
#define RIGHTPAREN 26
#define COMMA 27
#define INT 30
#define FLOAT 31
#define EOL 199

/******************************************************/

/* main driver */
int main() {
	string testCases[4] = {"front.in1","front.in2","front.in3","front.in4"};

	for (const auto& test: testCases) {
		TableEntry intEntry;
		TableEntry floatEntry;
		floatEntry.is_identifier = false;
		floatEntry.name = "float";
		lookupTable["float"] = floatEntry;
		intEntry.name = "int";
		intEntry.is_identifier = false;
		lookupTable["int"] = intEntry;

		ifstream infile(test);
		if (!infile) {
			cerr<<"cannot open: "<<test<<endl;
		}
		else {
			program(infile);
			print(lookupTable);
		}
		lookupTable.clear();
	}
}
/*****************************************************/
/* lookup - a function to lookup operators and parentheses
and return the token */
int lookup(char ch) {
	switch (ch) {
		case '(':
			addChar();
			nextToken = LEFTPAREN;
			break;
		case ')':
			addChar();
			nextToken = RIGHTPAREN;
			break;
		case '+':
			addChar();
			nextToken = ADDOP;
			break;
		case '-':
			addChar();
			nextToken = SUBOP;
			break;
		case '*':
			addChar();
			nextToken = MULTOP;
			break;
		case '/':
			addChar();
			nextToken = DIVOP;
			break;
		case '=':
			addChar();
			nextToken = ASSIGNOP;
			break;
		case ',':
			nextToken = COMMA;
			break;
		default:
			addChar();
			nextToken = EOL;
			break;
	}
	return nextToken;
}
/*****************************************************/
/* addChar - a function to add nextChar to lexeme */
void addChar() {
	if (lexLen <= 98) {
		lexeme[lexLen++] = nextChar;
		lexeme[lexLen] = 0;
	}
	else cout<<"Error - lexeme is too long \n";
}

/*****************************************************/
/* getChar - a function to get the next character of
input and determine its character class */
void getChar() {
	if (idx>= currentLine.length()) {
		charClass = EOL;
		nextChar = '\0';
		return;
	}
	nextChar = currentLine[idx++];
	if (isalpha(nextChar))
		charClass = LETTER;
	else if (isdigit(nextChar))
		charClass = DIGIT;
	else charClass = UNKNOWN;
}

/*****************************************************/
/* getNonBlank - a function to call getChar until it
returns a non-whitespace character */
void getNonBlank() {
	while (isspace(nextChar))getChar();
}

/* lex - a simple lexical analyzer for arithmetic
expressions */
int lex() {
	lexLen = 0;
	getNonBlank();
	switch (charClass) {
		/* Parse identifiers */
		case LETTER:
				addChar();
				getChar();
				while (charClass == LETTER || charClass == DIGIT) {
					addChar();
					getChar();
				}
				if (strcmp("int",lexeme)==0)nextToken = INT;
				else if (strcmp("float",lexeme)==0)nextToken = FLOAT;
				else nextToken = IDENT;
				break;
		case DIGIT: {
			bool isFloat = false;
			addChar();
			getChar();
			while (charClass == DIGIT) {
				addChar();
				getChar();
			}
			if (nextChar == '.') {
				isFloat = true;
				addChar();
				getChar();
				if (charClass != DIGIT){error(); nextToken = EOL;break;}
				while (charClass == DIGIT) {
					addChar();
					getChar();
				}
			}
			if (nextChar == 'E' || nextChar == 'e') {
				isFloat = true;
				addChar();
				getChar();
				if (nextChar == '-' || nextChar == '+') {addChar(); getChar();}
				if (charClass != DIGIT){error(); nextToken = EOL;break;}
				while (charClass == DIGIT) {
					addChar();
					getChar();
				}
			}
			nextToken = isFloat == true ? FLOATLIT : INTLIT;
			break;
		}
			/* Parentheses and operators */
		case UNKNOWN:
			lookup(nextChar);
			getChar();
			break;
		/* EOF */
		case EOL:
			nextToken = EOL;
			lexeme[0] = 'E';
			lexeme[1] = 'O';
			lexeme[2] = 'L';
			lexeme[3] = 0;
			break;
	} /* End of switch */
	return nextToken;
} /* End of function lex */


void program(ifstream &infile) {
	string line;

	while (getline(infile, line)) {
		if (line.empty())continue;
		currentLine = line;
		idx = 0;
		nextChar = 0;
		nextToken = 0;
		lexLen = 0;
		getChar();
		lex();
		if (nextToken == INT){declare_list(true);}
		else if (nextToken == FLOAT){declare_list(false);}
		else if (nextToken == IDENT){assign_list();}
		else error();
	}
}
void declare_list(bool isInt) {
	if (nextToken != INT && nextToken != FLOAT) {
		error();
		nextToken = EOF;
	}
	TableEntry ret = declare(isInt);
	lookupTable[ret.name] = ret;
	while (nextToken == COMMA) {
		TableEntry next = declare(isInt);
		lookupTable[next.name] = next;
		lex();
	}
}

TableEntry declare(bool isInt) {
	if (nextToken != IDENT)return {};
	TableEntry ret = TableEntry();
	ret.is_identifier = true;
	ret.name = lexeme;
	ret.type = isInt ? INTEGER : FLOATING;

	if (isInt) ret.value.i_val = 0;
	else ret.value.f_val = 0.0;

	lex();

	if (nextToken != ASSIGNOP) {
		return ret;
	}
	lex();
	if (!(nextToken == INTLIT || nextToken == FLOATLIT)) {return ret;}
	if (isInt) {
		ret.value.i_val = nextToken == INTLIT? stoi(lexeme) : static_cast<int> (stof(lexeme));
	}
	else {
		ret.value.f_val = nextToken == FLOATLIT? stof(lexeme) : static_cast<float> (stoi(lexeme));
	}
	return ret;
}


MkSnode* actualType(MkSnode* root) {
	if (!root) return nullptr;
	if (!root->right_child && !root->left_child) {
		if (root-> label == identifier) {
			const auto& entry = lookupTable[root->content];
			root-> actualType = entry.type;
			return root;
		}
		if (root-> label == float_const) {
			root-> actualType = FLOATING;
			return root;
		}
		if (root-> label == int_const) {
			root-> actualType = INTEGER;
			return root;
		}
	}
	MkSnode* lChild = actualType(root->left_child);
	MkSnode* rChild = actualType(root->right_child);
	if (!lChild && !rChild) return nullptr;

	if (root->content == "=") {
		root-> actualType = rChild->actualType;
		return root;
	}
	if (rChild-> actualType == FLOATING || lChild-> actualType == FLOATING) {
		root-> actualType = FLOATING;
		return root;
	}
	root->actualType = INTEGER;
	return root;
}

MkSnode* computedTypes(MkSnode* root) {

	return root;
}

MkSnode* assign_list() {
	vector<MkSnode*> chain;
	if (nextToken != IDENT) {
		error();
		nextToken = EOF;
		return nullptr;
	}
	while (nextToken == IDENT) {
		string temp = lexeme;
		lex();
		if (nextToken != ASSIGNOP) {
			error();
			nextToken = EOF;
			return nullptr;
		}
		lex();
		chain.push_back(new MkSnode(identifier, temp,nullptr ,nullptr ));
	}
	MkSnode* rhs = expr();
	if (rhs == nullptr) {return nullptr;}
	for (int i=chain.size()-1; i>=0; i--) {
		rhs = new  MkSnode(op,"=", chain[i],rhs);
	}
	MkSnode* actualTypedTree = actualType(rhs);
	return rhs;
}



MkSnode* assign() {
	if (nextToken != IDENT) {
		error();
		nextToken = EOF;
		return nullptr;
	}
	string temp = lexeme;
	MkSnode* ident = new MkSnode(identifier, lexeme, nullptr, nullptr);
	lex();
	if (nextToken != ASSIGNOP) {
		error();
		nextToken = EOF;
		return nullptr;
	}

	MkSnode* e = expr();
	if (e==nullptr) {
		error();
		nextToken = EOF;
		return nullptr;
	}
	return new MkSnode(op, "=", ident, e);
}
/* expr
*/
MkSnode* expr() {
	cout<<"Enter <expr>\n";
	/* Parse the first term */
	MkSnode* ret = term();
	/* As long as the next token is + or -
	, get
	the next token and parse the next term */
	while (nextToken == ADDOP || nextToken == SUBOP) {
		int tempToken = nextToken;
		lex();
		MkSnode* temp = term();
		if (temp == nullptr) return temp;
		ret = tempToken == ADDOP ?  new MkSnode(op, "+", ret, temp) : new MkSnode(op, "-", ret, temp);
	}
	cout<<"Exit <expr>\n";
	return ret;
} /* End of function expr */

/* term
Parses strings in the language generated by the rule:
<term> -> <factor> {(* | /) <factor>)
*/
MkSnode* term() {
	cout<<"Enter <term>\n";
	/* Parse the first factor */
	MkSnode* ret = factor();
	/* As long as the next token is * or /, get the
	next token and parse the next factor */
	while (nextToken == MULTOP || nextToken == DIVOP) {
		int tempToken = nextToken;
		lex();
		MkSnode* temp = factor();
		if (temp == nullptr)return temp;
		ret = tempToken == MULTOP ? new MkSnode(op,"*",ret,temp) : new MkSnode(op, "/", ret, temp);
	}
	cout<<"Exit <term>\n";
	return ret;

} /* End of function term */

/* factor
Parses strings in the language generated by the rule:
<factor> -> id | int
_
constant | ( <expr )
*/
MkSnode* factor() {
	cout<<"Enter <factor>\n";
	/* Determine which RHS */
	if (nextToken == IDENT || nextToken == INTLIT|| nextToken == FLOATLIT) {
		int tempToken = nextToken;
		string tempStr = lexeme;
		lex();
		return tempToken == IDENT ? new MkSnode(identifier, tempStr, nullptr, nullptr) : tempToken == INTLIT ? new MkSnode(int_const, tempStr, nullptr, nullptr) : new MkSnode(float_const,tempStr,nullptr,nullptr);
	}
		/* Get the next token */

	/* If the RHS is ( <expr> ), call lex to pass over the
	left parenthesis, call expr, and check for the right
	parenthesis */

	if (nextToken == LEFTPAREN) {
		lex();
		MkSnode* ret = expr();
		if (nextToken == RIGHTPAREN) {
			lex();
			return ret;
		}

		else
			return error();
	} /* End of if (nextToken ==
	...
	*/
	/* It was not an id, an integer literal, or a left
	parenthesis */
	else return error();

	 /* End of else */
	cout<<"Exit <factor>\n";

} /* End of function factor */

MkSnode* error() {
	cout<<"Error\n";
	return nullptr;
}

void print(unordered_map<string, TableEntry> table) {

}
