#include <iostream>
#include <utility>
#include <string>
#include <fstream>
#include <unordered_map>
#include <cstring>


using namespace std;

//this is for our mksnodes to identify what it is they are holding
enum Label {
	op,
	int_const,
	identifier,
	float_const,
};

//this is for our mksnode to label our actual and computed types
enum EntryType {TYPE_UNSET = -1,INTEGER = 0,FLOATING = 1};

//what a row in the table should look like;
struct TableEntry {
	bool is_identifier;
	EntryType type;
	union {int i_val;float f_val;} value;
	string name;
};

//our actual table
unordered_map<string, TableEntry> lookupTable;

//the nodes that we will build our tree with , we label what they are, their content will be what they hold, actual type is what they
// really are and computed is what we have to change them to, if a node is lhs then we know its a part of an assign
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

//this is for each element in our ASM we will either hold an int, a float, or an ident (for when its lhs)
struct StackEntry {
	union {
		int i_val;
		float f_val;
	};
	string ident;
};
//data structure for running our under the hood computations for each ilcg instruction
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
	void clear() {
		idx = 0;
	}
};

// this is for when we have things like y=l=j etc, we need a way to chain these together before entering the rhs so hold them in a stack
struct AssignStack{
	private:
		MkSnode* data[100] ={};
		int topIdx = 0;

	public:
		void push(MkSnode* n) {
			data[topIdx++] = n;
		}

		MkSnode* pop() {
			return data[--topIdx];
		}

		bool empty() const {
			return topIdx == 0;
		}

		int size() const {
			return topIdx;
		}

		void clear() {
			topIdx = 0;
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
void print(const unordered_map<string, TableEntry>& table);

//syntax analysis functions
void program(ifstream &infile);
MkSnode* assign_list();
void declare_list(bool isInt);
TableEntry declare(bool isInt);
MkSnode* term();
MkSnode* expr();
MkSnode* factor();
MkSnode* error();

//ilcg instructions
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
//we can now have floating point nums
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
	//declare testcases
	string testCases[2] = {"front.in1","front.in2"};
	// loop through each test case
	for (const auto& test: testCases) {
		cout<< "ENTERING TESTCASE: "<< test << endl;
		//populate table with initial float and int values
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
			//loop through the contents of each test case
			program(infile);
		}
		//clear stack and table
		ASM.clear();
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
		// now we can have a , for a declare list
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
				//find out if we are dealing with an ident int or float, look into the lexeme to see
				if (strcmp("int",lexeme)==0)nextToken = INT;
				else if (strcmp("float",lexeme)==0)nextToken = FLOAT;
				else nextToken = IDENT;
				break;
		case DIGIT: {
			//default is int
			bool isFloat = false;
			addChar();
			getChar();
			//build out the initial part of float or the entire digit
			while (charClass == DIGIT) {
				addChar();
				getChar();
			}
			//weve encountered a float
			if (nextChar == '.') {
				isFloat = true;
				//consume the .
				addChar();
				getChar();
				//this means we have somethign like 1.
				if (charClass != DIGIT){error(); nextToken = EOL;break;}
				while (charClass == DIGIT) {
					addChar();
					getChar();
				}
			}
			// for the case of 1e or 1.1E etc
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

//program -> program{declare_list}|program{assign_list } we said for the sake of the project that we will assume that we always have a declare list to start
void program(ifstream &infile) {
	string line;

	while (getline(infile, line)) {
		//loop line by line in the testcase
		if (line.empty())continue;
		//reset variables
		currentLine = line;
		idx = 0;
		nextChar = 0;
		nextToken = 0;
		lexLen = 0;
		getChar();
		lex();
		//we are declaring a list of integers or floats
		if (nextToken == INT){declare_list(true);}
		else if (nextToken == FLOAT){declare_list(false);}
		//if we start with ident we know its an assign list
		else if (nextToken == IDENT) {
			assign_list();
		}
		else error();
	}
}

//declare_list -> (int|float) <ident> = [<expr>] {,<ident> = [<expr>]
void declare_list(const bool isInt) {
	if (nextToken != INT && nextToken != FLOAT) {
		error();
		nextToken = EOF;
	}
	lex(); // consume int/float
	//initial call;
	TableEntry ret = declare(isInt);
	lookupTable[ret.name] = ret;
	//encountering a , means we have another var to declare
	while (nextToken == COMMA) {
		lex(); // consume ,
		TableEntry next = declare(isInt);
		lookupTable[next.name] = next;
	}
}

// a singular declare <ident> = const
TableEntry declare(const bool isInt) {
	if (nextToken != IDENT) {
		return {};
	}
	//table row to populate table with
	auto ret = TableEntry();
	ret.is_identifier = true;
	ret.name = lexeme;
	ret.type = isInt ? INTEGER : FLOATING;

	if (isInt) ret.value.i_val = 0;
	else ret.value.f_val = 0.0;
	lex(); // consume ident

	if (nextToken != ASSIGNOP) {
		return ret; //just ident, no ident =
	}
	lex(); // consume =

	//assign our values to the table entry
	if (isInt) {
		ret.value.i_val = nextToken == INTLIT? stoi(lexeme) : static_cast<int> (stof(lexeme));
	}
	else {
		ret.value.f_val = nextToken == FLOATLIT? stof(lexeme) : static_cast<float> (stoi(lexeme));
	}
	lex();//consume const
	return ret;
}

// these are my semantic analysis functions
//for assigning actual types we need to do a dfs and assign based on child
MkSnode* actualType(MkSnode* root) {
	if (!root) return nullptr;
	//this means we need to get the actual type based on the node itself since its a leaf
	if (!root->right_child && !root->left_child) {
		//if its an identifier we need to look it up in the table
		if (root-> label == identifier) {
			const auto& entry = lookupTable[root->content];
			root-> actualType = entry.type;
			return root;
		}
		//float or int means actual type is float or int
		if (root-> label == float_const) {
			root-> actualType = FLOATING;
			return root;
		}
		if (root-> label == int_const) {
			root-> actualType = INTEGER;
			return root;
		}
	}
	//dfs if we didnt hit a leaf node
	MkSnode* lChild = actualType(root->left_child);
	MkSnode* rChild = actualType(root->right_child);
	if (!lChild && !rChild) return nullptr;

	//assign the typing based on lchild for the edge case of having =
	if (root->content == "=") {
		root-> actualType = lChild->actualType;
		return root;
	}
	//case 2 if either are a float then the operations actual type must be float
	if (rChild-> actualType == FLOATING || lChild-> actualType == FLOATING) {
		root-> actualType = FLOATING;
		return root;
	}
	//otherwise we have an integer if both are actual ints
	root->actualType = INTEGER;
	return root;
}

//this is the pre order search through the tree to label our expected types
void computedTypes(MkSnode* root) {
	// edge base case
	if (!root) return;
	//real base case, since we are doing pre order, theres nothing to do by the children
	if (!root->right_child && !root->left_child) {
		return;
	}
	//label our lhs in this case since we have an = we also make the right childs
	//computed type be based on lchild actual and not parent
	if (root-> content == "=" && root->left_child && root->right_child) {
		root-> right_child-> computedType = root->left_child->actualType;
		root->left_child -> is_lhs = true;
	}
	//otherwise just assign the childrens computed type based on parents actual type
	else {
		if (root-> right_child)root->right_child->computedType = root->actualType;
		if (root->left_child)root->left_child->computedType = root->actualType;
	}
	computedTypes(root->left_child);
	computedTypes(root->right_child);
}

//this ends the semantic analysis functions we are now entering the ilcg part of compiler
//for generating ilcg we do a dfs search and then generate code based on what we return
void ilcg(MkSnode* root) {
	//base case nullptr
	if (!root) return;
	ilcg(root->left_child);
	ilcg(root->right_child);
	StackEntry entry = StackEntry();
	//if we are on the lhs of an expr we do the special case of populating the stack with the adress
	if (root-> is_lhs) {
		entry.ident = root-> content;
		ASM.push(entry);
		cout <<"pushaddr("<<root->content<<")"<<endl;
		return;
	}
	//if were not lhs but we are by an identifier we have to populate the stack with the values we get from the table
	if (root-> label == identifier) {
		TableEntry tableEntry = lookupTable[root->content];
		//populate with an int based on table entry
		if (root-> actualType == INTEGER) {
			int num = tableEntry.value.i_val;
			entry.i_val = num;
			ASM.push(entry);
			cout << "push("<<root->content<<")" << endl;
			//if computed != actual we have to convert
			if (root-> computedType == FLOATING) {
				itof();
			}
			return;
		}
		//same logic as int but inverted for float
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
	//otherwise if we are dealing with a float const we just push the value that the node itself holds
	//we dont lookup in the table because theres no entry for that
	if (root-> label == float_const) {
		const float fnum = stof(root->content);
		cout << "push("<<fnum<<")" << endl;
		entry.f_val = fnum;
		ASM.push(entry);
		return;
	}
	//same logic as float but we do have to coerce the int to float if needed since float takes priority
	//in our arithmetic typing
	if (root-> label == int_const) {
		const int inum = stoi(root->content);
		cout << "push("<<inum<<")" << endl;
		entry.i_val = inum;
		ASM.push(entry);
		if (root-> computedType == FLOATING) {
			itof();
		}
		return;
	}
	if (root-> label == op) {
		doOp(root);
	}
}

// seperate the logic of handling +-/*= into one function for better reading and maintainability
void doOp(MkSnode* root) {
	switch (root->content[0]) {
		case '+':
			//add our nodes and convert based on type
			if (root->actualType == FLOATING) {
				fadd();
				if (root->computedType == INTEGER) ftoi();
			}
			else { // INTEGER
				iadd();
				if (root->computedType == FLOATING) itof();
			}
			break;

		case '-':
			//subtract our nodes and convert based on type
			if (root->actualType == FLOATING) {
				fsub();
				if (root->computedType == INTEGER) ftoi();
			}
			else { // INTEGER
				isub();
				if (root->computedType == FLOATING) itof();
			}
			break;

		case '*':
			//multiply our nodes and convert based on type
			if (root->actualType == FLOATING) {
				fmult();
				if (root->computedType == INTEGER) ftoi();
			}
			else { // INTEGER
				imult();
				if (root->computedType == FLOATING) itof();
			}
			break;

		case '/':
			//divide our nodes and convert based on type
			if (root->actualType == FLOATING) {
				fdiv();
				if (root->computedType == INTEGER) ftoi();
			}
			else { // INTEGER
				idiv();
				if (root->computedType == FLOATING) itof();
			}
			break;

		case '=': {
			//case of = we need to do an assign operation
			const StackEntry rhs = ASM.top(); ASM.pop();
			const StackEntry lhs = ASM.top(); ASM.pop();

			//get row from loopup table
			TableEntry entry = lookupTable[lhs.ident];

			//assign value of rhs to the value column of our row
			if (entry.type == FLOATING) {
				entry.value.f_val = rhs.f_val;
			} else {
				entry.value.i_val = rhs.i_val;
			}
			//refresh the row
			lookupTable[lhs.ident] = entry;
			cout << "assign " << lhs.ident << endl;
			//push the rhs back onto the stack for cases of chained =
			ASM.push(rhs);
			//convert if needed
			if (root->computedType == FLOATING && root->actualType == INTEGER) itof();
			if (root->computedType == INTEGER && root->actualType == FLOATING) ftoi();
			break;
		}
		default:
			// no-op
			break;
	}
}

//convertion float to int
void ftoi() {
	cout << "fToi"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry oldEntry = ASM.top();
	ASM.pop();
	newEntry.i_val = static_cast<int> (oldEntry.f_val);
	ASM.push(newEntry);
}

//convertion int to float
void itof() {
	cout << "iTof"<<endl;
	StackEntry newEntry = StackEntry();
	const StackEntry oldEntry = ASM.top();
	ASM.pop();
	newEntry.f_val = static_cast<float> (oldEntry.i_val);
	ASM.push(newEntry);
}
//add floats
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
//add ints
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
//multiply ints
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
//multiple floats
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
//divide ints
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
//divide floats
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
//subtract ints
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
//subtract floats
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

//not needed was for debugging
/*void search(MkSnode* root) {
	if (!root) return;
	search(root->left_child);
	cout<<root->content<< "COMPUTED: " <<root-> computedType<<" ACTUAL: " << root-> actualType<<endl;
	search(root->right_child);
}*/

//<assign_list> -> <ident> = {ident =} expr()
MkSnode* assign_list() {
	//this is for chaining all of our = together, theres no parsing to run on them
	//we just need to create the children ordering from them
	AssignStack chain;

	if (nextToken != IDENT) {
		error();
		nextToken = EOF;
		return nullptr;
	}

	while (true) {
		if (nextToken != IDENT) break;

		// Save the full lexer state so we can roll back safely whe we have an expr instead of ident = again
		string savedLexeme = lexeme;
		int    savedToken  = nextToken;
		int    savedIdx    = idx;
		int    savedClass  = charClass;
		char   savedChar   = nextChar;
		int    savedLexLen = lexLen;

		// Candidate LHS identifier
		string name = lexeme;

		// Look ahead one token
		lex();  // try to move past the ident

		if (nextToken != ASSIGNOP) {
			// Not actually "IDENT =": restore state and stop chaining.
			idx       = savedIdx;
			charClass = savedClass;
			nextChar  = savedChar;
			nextToken = savedToken;
			strcpy(lexeme, savedLexeme.c_str());
			lexLen    = savedLexLen;
			break;  // RHS starts with this IDENT
		}

		// We have "IDENT =": commit this name to assignment chain
		chain.push(new MkSnode(identifier, name, nullptr, nullptr));

		// Consume '=' and continue; after this lex(), we're at the first token of RHS (or next LHS in chain)
		lex();
	}

	// Parse the RHS expression starting from the current token
	MkSnode* rhs = expr();
	if (!rhs) return nullptr;

	// Build the '=’ chain right-associatively: a = b = expr → a = (b = expr)
	for (int i = static_cast<int>(chain.size()) - 1; i >= 0; --i) {
		rhs = new MkSnode(op, "=", chain.pop(), rhs);
	}

	MkSnode* typed = actualType(rhs);
	computedTypes(typed);
	typed->computedType = typed->actualType;

	ilcg(typed);
	print(lookupTable);

	return typed;
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
		const int tempToken = nextToken;
		const string tempStr = lexeme;
		lex();
		//make our leaf nodes based on the content of lexeme and next token
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

//print out the table
void print(const unordered_map<string, TableEntry>& table) {
	cout << "\n===== SYMBOL TABLE =====\n";

	for (const auto& pair : table) {
		string key = pair.first;
		TableEntry entry = pair.second;
		if (key == "float" || key == "int") continue;

		cout << "name: " << key
			 << " | is_ident: " << entry.is_identifier
			 << " | value: ";

		if (entry.type == INTEGER) {
			cout << entry.value.i_val << " | type: INTEGER";
		}
		else {
			cout << entry.value.f_val << " | type: FLOAT";
		}

		cout << endl;
	}

	cout << "========================\n\n";
}
