#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

//C++ {
//None of this code is changed by the compiler
//}

//@TODO:
//TYPE CHECKING OF IDENTIFIERS PASSED AS FUNCTION ARGUMENTS, ALSO MAKE SURE THEY'RE DECLARED.
//BIN OP TYPECHECKING
//Redecl of function
//Constant too big for type: i.e. 99999999999999 to i32 or 99542025498453278498234329843289423895982489234893429832983249832894 to i64
//No implicit conversions. Maybe even go as far as no int into a float without cast?
//Pushing state theorizing

using namespace std;

enum struct Token {
	VAR_IDENTIFIER, CONSTANT, PRIMITIVE, FUNC_IDENTIFIER, //0,1,2,3
	OPEN_CURLY_BRACE, CLOSE_CURLY_BRACE, //4,5
	BIN_OP, RETURN_PRECURSOR, FUNC, C_CODE, //6,7,8,9
	TOK_NONE //10
};

enum struct Primitive_Type { /*@Thought: Types of structs get a subtype?*/
	INTEGER, FLOAT, DOUBLE, LONG, VOID, //0,1,2,3,4
	PRIM_NONE //5
};

enum struct Compiler_State {
	FUNCTION_DECLARATIONS, OTHER
};

struct Code_Node {
	string content;
	Token token_type;
	unsigned short line;

	Code_Node::Code_Node(string content, Token token_type, unsigned short line) {
		this->content = content;
		this->token_type = token_type;
		this->line = line;
	}
};

struct Function {
	string name;
	vector<Primitive_Type> arguments;
	vector<string> arguments_name;
	Primitive_Type return_value;
	
	Function::Function() { }
	Function::Function(string name, vector<Primitive_Type> arguments, vector<string> arguments_name, Primitive_Type return_value) {
		this->name = name;
		this->arguments = arguments;
		this->arguments_name = arguments_name;
		this->return_value = return_value;
	}
};

struct Variable {
	string name, scope_name;
	Primitive_Type type;
	Variable::Variable(string name, string scope_name, Primitive_Type type) {
		this->name = name;
		this->scope_name = scope_name;
		this->type = type;
	}
};

const string INTEGER = "i32";
const string FLOAT = "f32";
const string DOUBLE = "f64";
const string LONG = "i64";
const string VOID = "_";

//@NOTE: Typecheck for variable delcarations
void typecheck(string argument, Primitive_Type required_type, unsigned int line);
Token is_identifier(string test);
Primitive_Type is_constant(string test);
string prim_to_string(Primitive_Type transliterate);
Primitive_Type string_to_prim(string transliterate);
void insert_semi(unsigned int index, string &program_out, vector<Code_Node> &nodes);
int find_first_whitespace(string test, unsigned int offset = 0);
void err(int line, string err_msg);
bool contains(string test, string test_to);
void trim(string &mutate);
bool str_empty(string test);

int main() {

	ifstream in_file_handle("in.xx");
	string file_content;
	string cur_line;

	while (getline(in_file_handle, cur_line)) {
		file_content += cur_line + '\n';
	}
	in_file_handle.close();

	unsigned int expected_funcs = 0;

	vector<Code_Node> nodes;
	string cur_token;
	int nested_in_paren = 0;
	unsigned short line = 1;
	bool in_c = false;
	for (unsigned int i = 0; i < file_content.length(); i++) {
		if (file_content[i] == '(') {
			nested_in_paren++;
		}
		else if (file_content[i] == ')') {
			nested_in_paren--;
		}
		if (isspace(file_content[i]) && nested_in_paren == 0) {
			trim(cur_token);
			if (!str_empty(cur_token)) {
				if (in_c) {
					if (cur_token == "[/C_Code]") {
						in_c = false;
					}
					else {
						nodes.push_back(Code_Node(cur_token, Token::C_CODE, line));
					}
				}
				else {
					Token cur_node_token = Token::TOK_NONE;
					if (cur_token == "func") {
						cur_node_token = Token::FUNC;
					}
					else if (cur_token == "{") {
						cur_node_token = Token::OPEN_CURLY_BRACE;
					}
					else if (cur_token == "}") {
						cur_node_token = Token::CLOSE_CURLY_BRACE;
					}
					else if (cur_token == "->") {
						cur_node_token = Token::RETURN_PRECURSOR;
					}
					else if (cur_token == "[C_Code]") {
						in_c = true;
					}
					else if (cur_token == "=" || cur_token == "+=" || cur_token == "-=" || cur_token == "*=" || cur_token == "/=" || cur_token == "%=") {
						cur_node_token = Token::BIN_OP;
					}
					else if (cur_token == INTEGER || cur_token == FLOAT || cur_token == LONG || cur_token == VOID) {
						cur_node_token = Token::PRIMITIVE;
					}
					else if (is_constant(cur_token) != Primitive_Type::PRIM_NONE) {
						cur_node_token = Token::CONSTANT;
					}
					else {
						Token identifier_type = is_identifier(cur_token);
						if (identifier_type != Token::TOK_NONE) {
							if (identifier_type == Token::FUNC_IDENTIFIER) {
								cur_node_token = identifier_type;
								if (identifier_type == Token::FUNC_IDENTIFIER && nodes[nodes.size() - 1].token_type == Token::FUNC) {
									expected_funcs++;
								}
							}
							else {
								cur_node_token = identifier_type;
							}
						}
						else {
							err(line, "Tokenization problem: " + cur_token);
						}
					}
					nodes.push_back(Code_Node(cur_token, cur_node_token, line));
				}
				
			}
			cur_token = "";
		}
		else {
			cur_token += file_content[i];
		}
		if (file_content[i] == '\n') {
			line++;
		}
	}

	string program_out = "#include <stdint.h>\n#include <stdlib.h>\n#include <stdio.h>\n\ntypedef int32_t " + INTEGER + ";\ntypedef int64_t " + LONG + ";\ntypedef float " + FLOAT + ";\ntypedef double " + DOUBLE + ";\ntypedef void " + VOID + ";\n\n";

	Compiler_State current_state = Compiler_State::FUNCTION_DECLARATIONS;
	vector<Function> func_decls;
	vector<Variable> scoped_variables;
	string cur_scope_name = "global_scope"; //@TODO: No other function can have this name
	unsigned int scopes_deep = 0;
	unsigned int funcs_processed = 0;

	for (unsigned int i = 0; i < nodes.size(); i++) {
		Code_Node cur_node = nodes[i];
		switch (current_state) {
		case Compiler_State::FUNCTION_DECLARATIONS:
			if (cur_node.token_type == Token::FUNC) {
				if (nodes[i + 1].token_type != Token::FUNC_IDENTIFIER || nodes[i + 2].token_type != Token::RETURN_PRECURSOR) {
					err(cur_node.line, "Incorrect function delcaration.");
				}

				vector<Primitive_Type> args;
				vector<string> names;

				string begin_args_str = nodes[i + 1].content.substr(nodes[i + 1].content.find_first_of('(', 0) + 1, nodes[i + 1].content.find_first_of(')', 0) - nodes[i + 1].content.find_first_of('(', 0) - 1);

				if (begin_args_str.length() != 0) {
					trim(begin_args_str);

					int index = 0;
					int length = find_first_whitespace(begin_args_str, index);
					do {
						string arg = begin_args_str.substr(index, length);
						trim(arg);
						args.push_back(string_to_prim(arg));

						string cur_arg_name = begin_args_str.substr(index + length, begin_args_str.find_first_of(',', index + length) - (index + length));
						trim(cur_arg_name);
						names.push_back(cur_arg_name);

						index = begin_args_str.find_first_of(',', index) + 1;

						length = find_first_whitespace(begin_args_str, index + 1) - index;
					} while (index != 0);
				}

				func_decls.push_back(Function(nodes[i + 1].content.substr(0, nodes[i + 1].content.find_first_of('(', 0)), args, names, string_to_prim(nodes[i + 3].content)));
				funcs_processed++;

				if (funcs_processed == expected_funcs) {
					current_state = Compiler_State::OTHER;
					//@NOTE: Forward decls
					for (unsigned int j = 0; j < func_decls.size(); j++) {
						if (func_decls[j].name == "main") {
							program_out += "i32 main();\n";
						}
						else {
							program_out += prim_to_string(func_decls[j].return_value) + ' ' + func_decls[j].name + '(';
							for (unsigned int k = 0; k < func_decls[j].arguments.size(); k++) {
								program_out += prim_to_string(func_decls[j].arguments[k]) + ' ' + func_decls[j].arguments_name[k];
								if (k + 1 < func_decls[j].arguments.size()) {
									program_out += ", ";
								}
							}
							program_out += ");\n";
						}
					}
					program_out += '\n';
					i = -1;
				}
			}
			break;
		case Compiler_State::OTHER:
			bool nl = false, do_nothing = false;
			switch (cur_node.token_type) {
			case Token::FUNC_IDENTIFIER:
				if (nodes[i - 1].token_type != Token::FUNC) {
					bool found_func = false;
					Function matching_func;
					for (unsigned int j = 0; j < func_decls.size(); j++) {
						if (cur_node.content.substr(0, cur_node.content.find_first_of('(')) == func_decls[j].name) {
							found_func = true;
							matching_func = func_decls[j];
						}
					}
					if (!found_func) err(cur_node.line, "Undeclared function in use.");
					if (nodes[i - 1].token_type == Token::BIN_OP && matching_func.return_value == Primitive_Type::VOID) {
						err(cur_node.line, "Operating on a void return.");
					}

					string passed_args = cur_node.content.substr(cur_node.content.find_first_of('(') + 1, cur_node.content.find_first_of(')') - cur_node.content.find_first_of('(') - 1);

					int num_args = 0, pos = 0;

					while (pos != string::npos) {
						num_args++;
						pos = passed_args.find(',', pos + 1);
					}
					if (num_args != matching_func.arguments.size()) {
						err(cur_node.line, "Incorrect number of arguments supplied to function: " + matching_func.name + " (expected " + to_string(matching_func.arguments.size()) + ", got " + to_string(num_args) + ')');
					}

					if (matching_func.arguments.size() > 0) {
						int cur_arg_pos = 0;
						string cur_arg;
						int argument_index = 0;

						while (cur_arg_pos != string::npos) {
							cur_arg = passed_args.substr(cur_arg_pos + 1, passed_args.find_first_of(',', cur_arg_pos + 1) - cur_arg_pos - 1);
							cur_arg_pos = passed_args.find(',', cur_arg_pos + 1);
							trim(cur_arg);

							typecheck(cur_arg, matching_func.arguments[argument_index], cur_node.line);
							argument_index++;
						}
					}

					program_out += cur_node.content + ';';
				}
				else {
					string func_name = cur_node.content.substr(0, cur_node.content.find_first_of('('));
					if (func_name == "main") {
						program_out += "i32 " + cur_node.content;
					}
					else {
						program_out += nodes[i + 2].content + ' ' + cur_node.content;
					}
					cur_scope_name = func_name;
					i += 2;
				}
				insert_semi(i, program_out, nodes);
				break;
			case Token::VAR_IDENTIFIER:
			{
				program_out += cur_node.content;
				bool found = false;
				for (unsigned int j = 0; j < scoped_variables.size(); j++) {
					if (scoped_variables[j].name == cur_node.content) {
						found = true;
						break;
					}
				}
				if (!found) {
					if (nodes[i - 1].token_type != Token::PRIMITIVE) {
						err(cur_node.line, "Undeclared identifier.");
					}
					scoped_variables.push_back(Variable(cur_node.content, cur_scope_name, string_to_prim(nodes[i - 1].content)));
				}
				else {
					if (nodes[i - 1].token_type == Token::PRIMITIVE) {
						err(cur_node.line, "Redeclared identifier.");
					}
				}
			}
			insert_semi(i, program_out, nodes);
			break;
			case Token::CONSTANT:
				program_out += cur_node.content;
				insert_semi(i, program_out, nodes);
				break;
			case Token::PRIMITIVE:
				program_out += cur_node.content;
				break;
			case Token::OPEN_CURLY_BRACE:
				program_out += cur_node.content;
				nl = true;
				if (scopes_deep != 0) {
					cur_scope_name += '+';
				}
				scopes_deep++;
				break;
			case Token::CLOSE_CURLY_BRACE:
				program_out += '\n' + cur_node.content;
				nl = true;
				scopes_deep--;
				for (unsigned int j = 0; j < scoped_variables.size(); j++) {
					if (scoped_variables[j].scope_name == cur_scope_name) {
						scoped_variables.erase(scoped_variables.begin() + j);
						j--;
					}
				}

				if (scopes_deep == 0) {
					cur_scope_name = "global_scope";
				}
				else {
					cur_scope_name.erase(cur_scope_name.length() - 1);
				}
				break;
			case Token::BIN_OP:
				program_out += cur_node.content;
				break;
			case Token::RETURN_PRECURSOR:
				program_out += cur_node.content;
				break;
			case Token::FUNC:
				do_nothing = true;
				break;
			case Token::C_CODE:
				program_out += cur_node.content;
				break;
			}
			if (nl) {
				program_out += '\n';
			}
			else if (!do_nothing && program_out[program_out.size() - 1] != '\n') {
				program_out += ' ';
			}
		}
	}

	ofstream out_file_handle("out.c");
	out_file_handle << program_out;
	out_file_handle.close();

	system("gcc out.c -o out.exe -std=c11");

	cout << "Generated Code:\n" << endl;
	cout << program_out << endl;

	cout << "\nProgram Output:\n" << endl;

	system("out.exe");

	return 0;
}

void typecheck(string argument, Primitive_Type required_type, unsigned int line) {
	Primitive_Type argument_type = is_constant(argument);
	if (required_type == Primitive_Type::INTEGER) {
		if (argument_type == Primitive_Type::FLOAT || argument_type == Primitive_Type::LONG) {
			err(line, "Type mismatch(expected " + prim_to_string(required_type) + ", got " + prim_to_string(argument_type) + ')');
		}
	}
	else if (required_type == Primitive_Type::LONG) {
		if (argument_type == Primitive_Type::FLOAT) {
			err(line, "Type mismatch(expected " + prim_to_string(required_type) + ", got " + prim_to_string(argument_type) + ')');
		}
	}
	else if (required_type == Primitive_Type::FLOAT || required_type == Primitive_Type::DOUBLE) {

	}
}

Token is_identifier(string test) {
	if (test[0] >= '0' && test[0] <= '9') {
		return Token::TOK_NONE;
	}
	if (contains(test, "(") && contains(test, ")")) {
		return Token::FUNC_IDENTIFIER;
	}
	for (unsigned int i = 0; i < test.length(); i++) {
		if ((test[i] > 'z' || test[i] < 'a') && (test[i] > 'Z' || test[i] < 'A') && (test[i] > '9' || test[i] < '0') && test[i] != '?' && test[i] != '_') {
			return Token::TOK_NONE;
		}
	}
	return Token::VAR_IDENTIFIER;
}

Primitive_Type is_constant(string test) { //@TODO: Deal with longs and doubles too big
	union value {
		int64_t int_value;
		double fract_value;
	};

	unsigned int place = 1;
	bool dots = false, use_fract = false;
	value test_value = { 0 };
	if (contains(test, ".")) {
		use_fract = true;
	}
	for (unsigned int i = 0; i < test.length(); i++) {
		if (test[i] == '.') {
			if (dots) {
				return Primitive_Type::PRIM_NONE;
			}
			else {
				dots = true;
				continue;
			}
		}
		if (test[i] < '0' || test[i] > '9') {
			return Primitive_Type::PRIM_NONE;
		}
		if (test[i] == '.') {
			cout << numeric_limits<int32_t>::max() << endl;
			dots = true;
			continue;
		}
		if (use_fract) {
			if (dots) {
				test_value.fract_value += ((double)test[i] - 48) / pow(10, place);
				place++;
			}
			else {
				test_value.fract_value = (test_value.fract_value * 10) + test[i] - 48;
			}
		}
		else {
			test_value.int_value = (test_value.int_value * 10) + test[i] - 48;
		}
	}
	if (use_fract) {
		if (test_value.fract_value >= numeric_limits<float>::max() || test_value.fract_value <= numeric_limits<float>::min()) {
			return Primitive_Type::DOUBLE;
		}
		return Primitive_Type::FLOAT;
	}
	if (test_value.int_value >= numeric_limits<int32_t>::min() || test_value.int_value <= numeric_limits<int32_t>::max()) { //Test
		return Primitive_Type::INTEGER;
	}
	return Primitive_Type::LONG;
}

string prim_to_string(Primitive_Type transliterate) {
	if (transliterate == Primitive_Type::INTEGER) {
		return INTEGER;
	}
	else if (transliterate == Primitive_Type::LONG) {
		return LONG;
	}
	else if (transliterate == Primitive_Type::FLOAT) {
		return FLOAT;
	}
	else if (transliterate == Primitive_Type::DOUBLE) {
		return DOUBLE;
	}
	else if (transliterate == Primitive_Type::VOID) {
		return VOID;
	}
	err(-1, "Invalid conversion from Primitive_Type to string.");
	return "(NULL)";
}

Primitive_Type string_to_prim(string transliterate) {
	if (transliterate == INTEGER) {
		return Primitive_Type::INTEGER;
	}
	else if (transliterate == FLOAT) {
		return Primitive_Type::FLOAT;
	}
	else if (transliterate == DOUBLE) {
		return Primitive_Type::DOUBLE;
	}
	else if (transliterate == LONG) {
		return Primitive_Type::LONG;
	}
	else if (transliterate == VOID) {
		return Primitive_Type::VOID;
	}
	err(-1, "Invalid conversion from string to Primitive_Type.");
	return Primitive_Type::PRIM_NONE;
}

void insert_semi(unsigned int index, string &program_out, vector<Code_Node> &nodes) {
	if (nodes[index - 1].token_type == Token::BIN_OP && nodes[index + 1].token_type != Token::BIN_OP) {
		program_out += ";\n";
	}
}

int find_first_whitespace(string test, unsigned int offset) {
	for (unsigned int i = offset; i < test.length(); i++) {
		if (isspace(test[i])) {
			return i;
		}
	}
	return -1;
}

void err(int line, string err_msg) {
	cout << "Error on line " << line << ": " << err_msg << endl;
	exit(EXIT_FAILURE);
}

bool contains(string test, string test_to) {
	int checks = 0;
	for (unsigned int i = 0; i < test.length(); i++) {
		if (test[i] == test_to[checks]) {
			checks++;
		}
		else {
			checks = 0;
		}
		if (checks == test_to.length()) {
			return true;
		}
	}
	return false;
}

void trim(string &mutate) {
	if (str_empty(mutate)) {
		return;
	}
	int remove_index = 0;
	while (isspace(mutate[remove_index])) {
		mutate.erase(remove_index, 1);
	}
	remove_index = mutate.length() - 1;
	while (isspace(mutate[remove_index])) {
		mutate.erase(remove_index, 1);
	}
	for (unsigned int i = 0; i < mutate.length(); i++) {
		if (isspace(mutate[i]) && (isspace(mutate[i - 1]) || isspace(mutate[i + 1]))) {
			mutate.erase(i, 1);
			i--;
		}
	}
}

bool str_empty(string test) {
	for (unsigned int i = 0; i < test.length(); i++) {
		if (!isspace(test[i])) {
			return false;
		}
	}
	return true;
}