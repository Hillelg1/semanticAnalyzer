#include <iostream>
#include <stdio.h>
#include <utility>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cctype>


using namespace std;


enum Label {
	op,
	int_const,
	identifier,
	float_const,
};

enum EntryType {TYPE_UNSET = -1,INTEGER = 0,FLOATING = 1};

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
	bool is_lhs;
	MkSnode(const Label lb, string cn, MkSnode* left, MkSnode* right)
		: label(lb), content(std::move(cn)), actualType(TYPE_UNSET), computedType(TYPE_UNSET), left_child(left), right_child(right), is_lhs(false) {}
};

struct StackEntry {
	union {
		int i_val;
		float f_val;
	};
	string ident;
};

struct ASMStack {
	private:
		StackEntry data[100] = {};
		int idx = 0;
	public:
	void push(StackEntry entry) {
		data[idx++] = std::move(entry);
	}
	void pop() {
		idx--;
	}
	StackEntry top() {
		return data[idx-1];
	}
};
auto ASM =  ASMStack();

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
void print(const unordered_map<string, TableEntry>& table);
MkSnode* assign_list();
void declare_list(bool isInt);
TableEntry declare(bool isInt);
MkSnode* assign();
MkSnode* term();
MkSnode* expr();
MkSnode* factor();
MkSnode* error();

void ftoi();
void itof();
void iadd();
void fadd();
void fmult();
void imult();
void fdiv();
void idiv();
void isub();
void fsub();
void doOp(MkSnode* root);
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

int main() {
	string testCases[1] = {"front.in1"};

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
	lex(); // consume int/float
	TableEntry ret = declare(isInt);
	lookupTable[ret.name] = ret;
	while (nextToken == COMMA) {
		lex(); // consume ,
		TableEntry next = declare(isInt);
		lookupTable[next.name] = next;
	}
}

TableEntry declare(const bool isInt) {
	if (nextToken != IDENT) {
		return {};
	}
	TableEntry ret = TableEntry();
	ret.is_identifier = true;
	ret.name = lexeme;
	ret.type = isInt ? INTEGER : FLOATING;

	if (isInt) ret.value.i_val = 0;
	else ret.value.f_val = 0.0;

	lex(); // consume ident

	if (nextToken != ASSIGNOP) {
		return ret; //just ident,
	}
	lex(); // consume =

	if (isInt) {
		ret.value.i_val = nextToken == INTLIT? stoi(lexeme) : static_cast<int> (stof(lexeme));
	}
	else {
		ret.value.f_val = nextToken == FLOATLIT? stof(lexeme) : static_cast<float> (stoi(lexeme));
	}
	lex();//consume const
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
		root-> actualType = lChild->actualType;
		return root;
	}
	if (rChild-> actualType == FLOATING || lChild-> actualType == FLOATING) {
		root-> actualType = FLOATING;
		return root;
	}
	root->actualType = INTEGER;
	return root;
}

void computedTypes(MkSnode* root) {
	if (!root) return;
	if (!root->right_child && !root->left_child) {
		return;
	}
	if (root-> content == "=" && root->left_child && root->right_child) {
		root-> right_child-> computedType = root->left_child->actualType;
		root->left_child -> is_lhs = true;
	}
	else {
		if (root-> right_child)root->right_child->computedType = root->actualType;
		if (root->left_child)root->left_child->computedType = root->actualType;
	}
	computedTypes(root->left_child);
	computedTypes(root->right_child);
}

void ilcg(MkSnode* root) {
	if (!root) return;
	ilcg(root->left_child);
	ilcg(root->right_child);
	StackEntry entry = StackEntry();
	if (root-> is_lhs) {
		entry.ident = root-> content;
		ASM.push(entry);
		cout <<"pushaddr("<<root->content<<")"<<endl;
		return;
	}
	if (root-> label == identifier) {
		TableEntry tableEntry = lookupTable[root->content];
		if (root-> actualType == INTEGER) {
			int num = tableEntry.value.i_val;
			entry.i_val = num;
			ASM.push(entry);
			cout << "push("<<root->content<<")" << endl;
			if (root-> computedType == FLOATING) {
				itof();
			}
			return;
		}
		if (root-> actualType == FLOATING) {
			float num = tableEntry.value.f_val;
			entry.f_val = num;
			ASM.push(entry);
			cout << "push("<<root->content<<")" << endl;
			if (root-> computedType == INTEGER) {
				ftoi();
			}
		}
		return;
	}
	if (root-> label == float_const) {
		const float fnum = stof(root->content);
		cout << "push("<<fnum<<")" << endl;
		entry.f_val = fnum;
		ASM.push(entry);
		return;
	}
	if (root-> label == int_const) {
		const int inum = stoi(root->content);
		cout << "push("<<inum<<")" << endl;
		entry.i_val = inum;
		ASM.push(entry);
		return;
	}
	if (root-> label == op) {
		doOp(root);
	}


}

void doOp(MkSnode* root) {
	if (root-> content == "+") {
		if (root-> actualType == FLOATING) {
			fadd();
			if (root-> computedType == INTEGER) {
				ftoi();
			}
		}
		if (root-> actualType == INTEGER) {
			iadd();
			if (root-> computedType == FLOATING) {
				itof();
			}
		}
	}
	else if (root-> content == "-") {
		if (root-> actualType == FLOATING) {
			fsub();
			if (root-> computedType == INTEGER) {
				ftoi();
			}
		}
		if (root-> actualType == INTEGER) {
			isub();
			if (root-> computedType == FLOATING) {
				itof();
			}
		}
	}
	else if (root-> content == "*") {
		if (root-> actualType == FLOATING) {
			fmult();
			if (root-> computedType == INTEGER) {
				ftoi();
			}
		}
		if (root-> actualType == INTEGER) {
			imult();
			if (root-> computedType == FLOATING) {
				itof();
			}
		}
	}
	else if (root-> content == "/") {
		if (root-> actualType == FLOATING) {
			fdiv();
			if (root-> computedType == INTEGER) {
				ftoi();
			}
		}
		if (root-> actualType == INTEGER) {
			idiv();
			if (root-> computedType == FLOATING) {
				itof();
			}
		}
	}
	else if (root-> content == "=") {
		StackEntry rhs = ASM.top();ASM.pop();
		StackEntry lhs = ASM.top();ASM.pop();
		TableEntry entry = lookupTable[lhs.ident];
		if (entry.type == FLOATING) {
			entry.value.f_val = rhs.f_val;
		}
		else {
			entry.value.i_val = rhs.i_val;
		}
		lookupTable[lhs.ident] = entry;
		cout << "assign " << lhs.ident << endl;
		ASM.push(rhs);
		if (root-> computedType == FLOATING && root-> actualType == INTEGER) {itof();}
		if (root-> computedType == INTEGER && root-> actualType == FLOATING) {ftoi();}
	}

}
void ftoi() {
	cout << "fToi"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry oldEntry = ASM.top();
	ASM.pop();
	newEntry.i_val = static_cast<int> (oldEntry.f_val);
	ASM.push(newEntry);
}

void itof() {
	cout << "iTof"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry oldEntry = ASM.top();
	ASM.pop();
	newEntry.f_val = static_cast<float> (oldEntry.i_val);
	ASM.push(newEntry);
}

void fadd() {
	cout<< "fadd"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.f_val = var1.f_val + var2.f_val;
	ASM.push(newEntry);
}

void iadd() {
	cout << "iadd"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.i_val = var2.i_val + var1.i_val;
	ASM.push(newEntry);
}

void imult() {
	cout << "imult"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.i_val = var2.i_val * var1.i_val;
	ASM.push(newEntry);
}

void fmult() {
	cout << "fmult"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.f_val = var2.f_val * var1.f_val;
	ASM.push(newEntry);
}

void idiv() {
	cout << "idiv"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.i_val = var2.i_val / var1.i_val;
	ASM.push(newEntry);
}

void fdiv() {
	cout << "fdiv"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.f_val = var2.f_val / var1.f_val;
	ASM.push(newEntry);
}

void isub() {
	cout << "isub"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.i_val = var2.i_val - var1.i_val;
	ASM.push(newEntry);
}

void fsub() {
	cout << "fsub"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry var1 = ASM.top();
	ASM.pop();
	const StackEntry var2 = ASM.top();
	ASM.pop();
	newEntry.f_val = var2.f_val - var1.f_val;
	ASM.push(newEntry);
}

void search(MkSnode* root) {
	if (!root) return;
	search(root->left_child);
	cout<<root->content<< "COMPUTED: " <<root-> computedType<<" ACTUAL: " << root-> actualType<<endl;
	search(root->right_child);
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
	computedTypes(actualTypedTree);
	actualTypedTree->computedType = actualTypedTree->actualType;
	search(actualTypedTree);
	ilcg(actualTypedTree);
	print(lookupTable);
	return actualTypedTree;
}




/* expr
*/
MkSnode* expr() {
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
	return ret;
} /* End of function expr */

/* term
Parses strings in the language generated by the rule:
<term> -> <factor> {(* | /) <factor>)
*/
MkSnode* term() {
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
	return ret;

} /* End of function term */

/* factor
Parses strings in the language generated by the rule:
<factor> -> id | int
_
constant | ( <expr )
*/
MkSnode* factor() {
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
	return error();

	 /* End of else */

} /* End of function factor */

MkSnode* error() {
	cout<<"Error\n";
	return nullptr;
}

void print(const unordered_map<string, TableEntry>& table) {
	for (const auto& pair : table) {
		TableEntry entry = pair.second;
		string key = pair.first;
		if (key == "float" || key == "int") continue;
		if (entry.type == INTEGER) {
			cout<<"is_ident: "<< entry.is_identifier<<" name: "<<key << " value: "<<entry.value.i_val <<" type: INTEGER" << endl;
		}
		else {
			cout<<"is_ident: "<< entry.is_identifier<<" name: "<<key << " value: "<<entry.value.f_val <<" type: FLOAT" << endl;
		}
	}
}
