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
  
def find_matches_in_asm(asm_path, hex_values):  
    """在ASM文件中查找与给定十六进制数值列表中的每个值相等的行，并按给定列表的顺序输出匹配的行"""  
    with open(asm_path, 'r', encoding='utf-8') as file:  
        asm_lines = file.readlines()  # 一次性读取所有行到列表中  
  
    for hex_value in hex_values:  
        hex_int = int(hex_value, 16)  
        for line in asm_lines:  
            # 查找冒号前的十六进制数并比较  
            match = re.match(r'([0-9a-fA-F]+):.*', line.strip())  
            if match and int(match.group(1), 16) == hex_int:
                print(line, end='')
         
def main():  
    # 指定文件路径  
    stackpc_path = 'E:\\RvBacktrace\\tools\\pc\\stackpc.txt'  
    asm_path = 'E:\\RvBacktrace\\tools\\asm\\rtthread.asm'  
      
    # 从stackpc.txt中提取所有十六进制数  
    hex_values = extract_hex_from_file(stackpc_path)  
      
    # 检查是否成功提取到数据  
    if not hex_values:  
        print("stackpc.txt中没有找到以0x开头的有效十六进制数。")  
    else:  
        # 在rtthread.asm中查找匹配的行并按stackpc.txt中的顺序输出  
        find_matches_in_asm(asm_path, hex_values)  
  
if __name__ == "__main__":  
    main()