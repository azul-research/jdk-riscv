#include <iostream>

using namespace std;

void prolog() {
	cout << "package  java/lang;\n"
					"\n"
					"super public class T0\n"
					"\tversion 50:0\n"
					"{\n"
		 			"\n";
}

void epilog() {
	cout << "\n"
					"} // end Class T0";
}

void normal_binary() {
	string operation[] = {"add", "sub", "mul", "div", "rem"};
	string type[] = {"i", "l", "f", "d"};

	for (const string &op : operation) {
		for (string t : type) {
			string T = "0";
			T[0] = t[0] - 'a' + 'A';
			if (T[0] == 'L') T[0] = 'J';
			string size = (t == "i" || t == "f") ? "1" : "2";
			string locals = size == "1" ? "2" : "4";
			cout << "\tpublic static Method test" + T + op + ":\"(" + T + T + ")" + T + "\"\n"
							"\t\tstack " +  locals + " locals " + locals + "\n"
							"\t{\n"
			 				"\t\t" + t + "load_0;\n"
			 				"\t\t" + t + "load_" + size + ";\n"
							"\t\t" + t + op + ";\n"
							"\t\t" + t + "return;\n"
							"\t}\n";
		}
	}
}

void neg() {
	string type[] = {"i", "l", "f", "d"};

	for (string t : type) {
		string T = "0";
		T[0] = t[0] - 'a' + 'A';
		if (T[0] == 'L') T[0] = 'J';
    string size = (t == "i" || t == "f") ? "1" : "2";
		cout << "\tpublic static Method test" + T + "neg:\"(" + T + ")" + T + "\"\n"
						"\t\tstack " + size + " locals " + size + "\n"
						"\t{\n"
						"\t\t" + t + "load_0;\n"
						"\t\t" + t + "neg;\n"
						"\t\t" + t + "return;\n"
						"\t}\n";
	}
}

void shift() {
	string operation[] = {"shl", "shr", "ushr"};
	string type[] = {"i", "l"};


	for (const string &op : operation) {
		for (string t : type) {
			string T = "0";
			T[0] = t[0] - 'a' + 'A';
			if (T[0] == 'L') T[0] = 'J';
			string size = t == "i" ? "1" : "2";
			string locals = size == "1" ? "2" : "3";
			cout << "\tpublic static Method test" + T + op + ":\"(" + T + "I)" + T + "\"\n"
              "\t\tstack " + locals + " locals " + locals + "\n"
              "\t{\n"
              "\t\t" + t + "load_0;\n"
              "\t\tiload_" + size + ";\n"
              "\t\t" + t + op + ";\n"
              "\t\t" + t + "return;\n"
              "\t}\n";
		}
	}
}

void bitwise() {
	string operation[] = {"or", "and", "xor"};
	string type[] = {"i", "l"};

	for (const string &op : operation) {
		for (string t : type) {
				string T = "0";
				T[0] = t[0] - 'a' + 'A';
				if (T[0] == 'L') T[0] = 'J';
				string size = t == "i" ? "1" : "2";
				string locals = size == "1" ? "2" : "4";
				cout << "\tpublic static Method test" + T + op + ":\"(" + T + T + ")" + T + "\"\n"
                "\t\tstack " + locals + " locals " + locals + "\n"
                "\t{\n"
                "\t\t" + t + "load_0;\n"
                "\t\t" + t + "load_" + size + ";\n"
                "\t\t" + t + op + ";\n"
                "\t\t" + t + "return;\n"
                "\t}\n";
			}
	}
}

void iinc() {
  string consts[] = {"-128", "-127", "-1", "0", "1", "127"};
  for (const string &c : consts) {
      string name = c;
      if (name[0] == '-') name[0] = 'm';
      cout << "\tpublic static Method testIinc" + name + ":\"(I)I\"\n"
              "\t\tstack 1 locals 1\n"
              "\t{\n"
              "\t\tiinc 0, " + c + ";\n"
              "\t\tiload_0;\n"
              "\t\tireturn;\n"
              "\t}\n";
  }
}

void cmp_float() {
	string operation[] = {"cmpl", "cmpg"};
	string type[] = {"f", "d"};

	for (const string &op : operation) {
		for (string t : type) {
				string T = "0";
				T[0] = t[0] - 'a' + 'A';
				string size = t == "f" ? "1" : "2";
				string locals = size == "1" ? "2" : "4";
				cout << "\tpublic static Method test" + T + op + ":\"(" + T + T + ")I\"\n"
                "\t\tstack " + locals + " locals " + locals + "\n"
                "\t{\n"
                "\t\t" + t + "load_0;\n"
                "\t\t" + t + "load_" + size + ";\n"
                "\t\t" + t + op + ";\n"
                "\t\tireturn;\n"
                "\t}\n";
			}
	}
}

void lcmp() {
	cout << "\tpublic static Method testJcmp:\"(JJ)I\"\n"
          "\t\tstack 4 locals 4\n"
          "\t{\n"
          "\t\tlload_0;\n"
          "\t\tlload_2;\n"
          "\t\tlcmp;\n"
          "\t\tireturn;\n"
          "\t}\n";
}

void math() {
	cout << "\tpublic static Method testDsqrt:\"(D)D\"\n"
					"\t\tstack 2 locals 2\n"
					"\t{\n"
					"\t\tdload_0;\n"
					"\t\tinvokestatic  java/lang/Math.sqrt:\"(D)D\";\n"
					"\t\tdreturn;\n"
					"\t}\n"
					"\n"
					"\tpublic static Method testFabs:\"(F)F\"\n"
					"\t\tstack 1 locals 1\n"
					"\t{\n"
					"\t\tfload_0;\n"
					"\t\tinvokestatic  java/lang/Math.abs:\"(F)F\";\n"
					"\t\tfreturn;\n"
					"\t}\n"
					"\n"
					"\tpublic static Method testDabs:\"(D)D\"\n"
					"\t\tstack 2 locals 2\n"
					"\t{\n"
					"\t\tdload_0;\n"
					"\t\tinvokestatic  java/lang/Math.abs:\"(D)D\";\n"
					"\t\tdreturn;\n"
					"\t}\n"
					"\n"
					"\tpublic static Method testFfma:\"(FFF)F\"\n"
					"\t\tstack 3 locals 3\n"
					"\t{\n"
					"\t\tfload_0;\n"
					"\t\tfload_1;\n"
					"\t\tfload_2;\n"
					"\t\tinvokestatic  java/lang/Math.fma:\"(FFF)F\";\n"
					"\t\tfreturn;\n"
					"\t}\n"
					"\n"
					"\tpublic static Method testDfma:\"(DDD)D\"\n"
					"\t\tstack 6 locals 6\n"
					"\t{\n"
					"\t\tdload_0;\n"
					"\t\tdload_2;\n"
					"\t\tdload 4;\n"
					"\t\tinvokestatic  java/lang/Math.fma:\"(DDD)D\";\n"
					"\t\tdreturn;\n"
					"\t}\n"
					"\n"
					"\tpublic static Method testDsin:\"(D)D\"\n"
					"\t\tstack 2 locals 2\n"
					"\t{\n"
					"\t\tdload_0;\n"
					"\t\tinvokestatic  java/lang/Math.sin:\"(D)D\";\n"
					"\t\tdreturn;\n"
					"\t}\n";
}

int main() {
	prolog();
	normal_binary();
	neg();
	shift();
	bitwise();
	iinc();
	cmp_float();
	lcmp();
	math();
	epilog();
	return 0;
}