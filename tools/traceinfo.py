import os  
import re  
  
def extract_hex_from_file(file_path):  
    """从指定文件中提取所有以0x开头的十六进制数据（去除'0x'前缀）"""  
    hex_values = []  
    with open(file_path, 'r', encoding='utf-8') as file:  
        for line in file:  
            if line.strip().startswith('0x'):  
                hex_values.append(line.strip()[2:])  # 去除'0x'并保留整个十六进制数  
    return hex_values  

def read_asm_path_from_file(path_txt_file):  
    """从path.txt文件中读取asm_path字段的值"""  
    asm_path = None  
    with open(path_txt_file, 'r', encoding='utf-8') as file:  
        for line in file:  
            if line.strip().startswith('asm_path = '):  
                # 假设路径是被单引号或双引号包围的  
                match = re.match(r'asm_path = ([\'"])(.*?)\1', line.strip())  
                if match:  
                    asm_path = match.group(2)  
                    break  
    return asm_path  
  
def find_matches_in_asm(asm_path, hex_values, output_dir='obj'):  
    """在ASM文件中查找与给定十六进制数值列表中的每个值相等的行，  
    并根据冒号后数据的十六进制值是否小于0x10000来决定输出当前行还是下一行，  
    最后将信息输出到指定目录下的info.txt文件中。  
    """  
    # 确保输出目录存在  
    if not os.path.exists(output_dir):  
        os.makedirs(output_dir)  
  
    # 构建完整的输出文件路径  
    output_file_path = os.path.join(output_dir, 'info.txt')  
  
    # 尝试以写入模式打开info.txt文件  
    with open(output_file_path, 'w', encoding='utf-8') as info_file:  
        with open(asm_path, 'r', encoding='utf-8') as file:  
            asm_lines = file.readlines()  # 一次性读取所有行到列表中  
  
        for hex_value in hex_values:  
            hex_int = int(hex_value, 16)  
            for i, line in enumerate(asm_lines):  
                match = re.match(r'([0-9a-fA-F]+):\s*([0-9a-fA-F]+).*', line.strip())  
                if match and int(match.group(1), 16) == hex_int:  
                    data_hex = match.group(2)  
                    data_int = int(data_hex, 16)  
  
                    if data_int < 0x10000:  
                        if i + 1 < len(asm_lines):  
                            next_line = asm_lines[i + 1]  
                            info_file.write(next_line)  
                    else:  
                        info_file.write(line)  
  
def main():  
    # 获取当前工作目录  
    current_dir = os.getcwd()  
    # 构建obj文件夹的路径  
    obj_dir = os.path.join(current_dir, 'obj')  
    # 构建path.txt的完整路径  
    path_txt_file = os.path.join(obj_dir, 'path.txt')  
      
    # 从path.txt文件中读取asm_path  
    asm_path = read_asm_path_from_file(path_txt_file)  
    
    if not asm_path:  
        print("无法在path.txt文件中找到asm_path字段。")  
        return  
      
    # 假设function_pc.txt也位于obj文件夹中  
    function_pc_path = os.path.join(obj_dir, 'function_pc.txt')  
      
    # 从function_pc.txt中提取所有十六进制数  
    hex_values = extract_hex_from_file(function_pc_path)  
      
    # 检查是否成功提取到数据  
    if not hex_values:  
        print("function_pc.txt中没有找到以0x开头的有效十六进制数。")  
    else:  
        # 在rtthread.asm中查找匹配的行并按function_pc.txt中的顺序输出  
        find_matches_in_asm(asm_path, hex_values)  
  
if __name__ == "__main__":  
    main()