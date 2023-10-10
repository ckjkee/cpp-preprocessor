#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(istream& input, ostream& output, const path& curr_file, const vector<path>& include_directories) {
    static regex local (R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex global (R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    smatch m;
    string str;
    int counter = 0;
    while ( getline(input, str) ) {
        counter++;
        bool external = false;
        path new_path;
        if (regex_match(str, m, local)) {
            new_path = curr_file.parent_path() / string(m[1]);
            if (filesystem::exists(new_path)) {
                ifstream read(new_path, ios::in);
                if (read.is_open()) {
                    if (!Preprocess(read, output, new_path, include_directories)) {
                        return false;
                    }
                    continue;
                }
                else {
                     cout << "unknown include file "s << new_path.filename().string()   << " at file " << curr_file.string() << " at line "s << counter << endl;
                    return false;
                }
            }
            else {
                external = true;
            }
        }
        if (external || regex_match(str, m, global)) {
            bool valid = false;
            for (const auto& dir : include_directories) {
                new_path = dir / string(m[1]);
                if (filesystem::exists(new_path)) {
                    ifstream read(new_path, ios::in);
                    if (read.is_open()) {
                        if (!Preprocess(read, output, new_path, include_directories)) {
                            return false;
                        }
                        valid = true;
                        break;
                    }
                    else {
                        cout << "unknown include file "s << new_path.filename().string()   << " at file " << curr_file.string() << " at line "s << counter << endl;
                        return false;
                    }
                }
            }
            if (!valid) {
                cout << "unknown include file "s << new_path.filename().string()   << " at file " << curr_file.string() << " at line "s << counter << endl;
                return false;
            }
            continue;
        }
        output << str << endl;
    }
    return true;
}
// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){
    if(!filesystem::exists(in_file)){
        return false;
    }
    
    ifstream in(in_file, ios::in);
    if(!in){
        return false;
    }
    ofstream out(out_file, ios::out);
    
    return Preprocess(in, out, in_file, include_directories);
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}