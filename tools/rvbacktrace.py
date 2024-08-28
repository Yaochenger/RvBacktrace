import os  
import re  
  
# 用户输入目录路径  
directory_input = input("请输入txt文件的目录路径（例如：E:\\RvBacktrace\\tools\\txt）: ")  
directory = os.path.abspath(directory_input)  
  
# 确保路径存在且为目录  
if not os.path.exists(directory) or not os.path.isdir(directory):  
    print("指定的路径不存在或不是一个目录。")  
    exit()  
  
# 栈帧计数器  
frame_number = 0  
  
# 写入结果的文件名  
output_filename = 'stackpc.txt'  
  
# 尝试打开（或创建）输出文件  
with open(output_filename, 'w', encoding='utf-8') as output_file:  
    # 遍历目录下的所有.txt文件  
    for filename in os.listdir(directory):  
        if filename.endswith('.txt'):  
            file_path = os.path.join(directory, filename)  
  
            try:  
                with open(file_path, 'r', encoding='utf-8') as file:  
                    for line in file:  
                        # 检查行中是否包含"pc"（不区分大小写）  
                        if 'pc' in line.lower():  
                            # 在"pc"之后搜索以"0x"开头的十六进制数据  
                            hex_match = re.search(r'pc\s*0x[0-9a-fA-F]+', line)  
                            if hex_match:  
                                # 提取十六进制数据部分  
                                hex_data = hex_match.group(0).split(' ', 1)[-1].strip()  
                                # 去除前导空格和非十六进制字符（仅保留"0x"及其后的内容）  
                                hex_data = re.search(r'\b0x[0-9a-fA-F]+\b', hex_data).group(0)  
                                # 打印栈帧级别和栈帧地址（可选，用于控制台输出）  
                                print(f"第{frame_number + 1}级栈帧，栈帧地址: {hex_data}")  
                                # 将栈帧地址写入输出文件  
                                output_file.write(hex_data + '\n')  
                                # 更新栈帧计数器  
                                frame_number += 1  
  
            except Exception as e:  
                print(f"无法读取文件 {file_path}: {e}")  
  
# 程序执行完毕  
print("所有txt文件中的栈帧地址已写入 stackpc.txt 文件。")