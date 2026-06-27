#include <iostream>
#include <fstream>
#include <string>

int main() {
using namespace std;
cout << "=== 字体转16进制头文件工具 (unsigned int版) ===\n" << endl;
string input_file;
cout << "输入字体文件路径: ";
cin >> input_file;
ifstream fin(input_file, ios::binary);
if (!fin) {
cout << "错误: 无法打开文件 '" << input_file << "'" << endl;
cout << "请确保文件在当前目录下!" << endl;
return 1;
}
fin.seekg(0, ios::end);
int file_size = fin.tellg();
fin.seekg(0, ios::beg);
cout << "文件大小: " << file_size << " 字节" << endl;
char* buffer = new char[file_size];
fin.read(buffer, file_size);
fin.close();
size_t dot_pos = input_file.find_last_of(".");
string base_name = (dot_pos != string::npos) ? 
   input_file.substr(0, dot_pos) : input_file;
string output_file = base_name + "_font.h";
ofstream fout(output_file);
if (!fout) {
cout << "错误: 无法创建输出文件" << endl;
delete[] buffer;
return 1;
}
// 写入头文件注释（和你图里格式对齐）
fout << "// File: '" << input_file << "' (" << file_size << " bytes)\n";
fout << "// Exported using binary_to_compressed_cpp.cpp\n\n";
// 写入大小定义（和你图里格式对齐）
fout << "static const unsigned int font_size = " << file_size << ";\n";
fout << "static const unsigned int font_data[" << (file_size / 4) << "] =\n";
fout << "{\n";
int bytes_per_line = 8; // 每行8个uint，和你图里一样
int uint_count = 0;
// 按4字节一组打包成unsigned int
for (int i = 0; i < file_size; i += 4) {
unsigned int value = 0;
// 小端序组合4个字节（和你图里的存储方式一致）
for (int j = 0; j < 4; j++) {
if (i + j < file_size) {
unsigned char byte = static_cast<unsigned char>(buffer[i + j]);
value |= (static_cast<unsigned int>(byte) << (j * 8));
}
}
// 输出16进制，前缀0x
fout << "0x" << hex << value;
if (i + 4 < file_size) {
fout << ",";
uint_count++;
if (uint_count % bytes_per_line == 0) {
fout << "\n";
} else {
fout << " ";
}
}
}
fout << "\n};\n";
fout.close();
delete[] buffer;
cout << "\n✅ 转换完成!" << endl;
cout << "生成文件: " << output_file << endl;
return 0;
}
