import os  
  
# 获取当前脚本文件所在路径  
current_script_path = os.path.abspath(__file__)  
current_script_dir = os.path.dirname(current_script_path)  
  
# 设定obj文件夹的路径，基于当前脚本所在目录  
obj_folder_path = os.path.join(current_script_dir, 'obj')  
  
# path.txt文件的完整路径  
path_txt_file = os.path.join(obj_folder_path, 'path.txt')  
  
# 检查obj文件夹是否存在，如果不存在则创建  
if not os.path.exists(obj_folder_path):  
    os.makedirs(obj_folder_path)  
  
# 检查path.txt文件是否存在以及是否包含asm_path和info_path字段  
def check_and_get_existing_paths(file_path):  
    existing_paths = {}  
    if os.path.exists(file_path):  
        with open(file_path, 'r') as file:  
            for line in file:  
                stripped_line = line.strip()  
                if stripped_line.startswith(('asm_path = ', 'info_path = ')):  
                    key, value = stripped_line.split(' = ', 1)  
                    existing_paths[key] = value.strip("'")  
    return existing_paths  
  
# 获取已存在的路径  
existing_paths = check_and_get_existing_paths(path_txt_file)  
  
# 处理asm_path  
asm_path_found = 'asm_path' in existing_paths  
if not asm_path_found:  
    directory_path = input("[RV] 请输入包含汇编文件的目录路径: ")  
    if not os.path.isdir(directory_path):  
        print("[RV] 错误：输入的不是一个有效的目录。")  
        exit()  
  
    assembly_files = []  
    for root, dirs, files in os.walk(directory_path):  
        for file in files:  
            if file.endswith(('.s', '.S', '.ASM', '.asm')):  
                assembly_files.append(os.path.abspath(os.path.join(root, file)))  
  
    if assembly_files:  
        with open(path_txt_file, 'a') as file:  # 使用追加模式  
            if os.path.getsize(path_txt_file) > 0:  # 如果文件不为空，则添加换行符  
                file.write('\n')  
            for file_path in assembly_files:  
                # 保存每个汇编文件的完整路径  
                file.write(f"asm_path = '{file_path}'\n")  
        print(f"[RV] 汇编文件的完整路径已保存到: {path_txt_file}")  
    else:  
        print("[RV] 在指定目录下未找到汇编文件。")  
else:  
    print("[RV] 路径文件path.txt已存在且包含asm_path字段，无需重新输入目录。") 
  
# 处理info_path（逻辑与asm_path类似，但这里保留列出所有文件的逻辑）  
info_path_found = 'info_path' in existing_paths  
if not info_path_found:  
    directory_path = input("[RV] 请输入包含串口打印文件的目录路径: ")  
    if not os.path.isdir(directory_path):  
        print("错误：输入的不是一个有效的目录。")  
        exit()  
  
    info_files = []  
    for root, dirs, files in os.walk(directory_path):  
        for file in files:  
            if file.endswith(('.txt', '.TXT')):  
                info_files.append(os.path.abspath(os.path.join(root, file)))  
  
    if info_files:  
        with open(path_txt_file, 'a') as file:  # 使用追加模式  
            if os.path.getsize(path_txt_file) > 0 and not file.tell() == 0:  # 如果文件不为空且不是在文件开头  
                file.write('\n')  
            for file_path in info_files:  
                file.write(f"info_path = '{file_path}'\n")  
        print(f"[RV] 串口打印文件的绝对路径已保存到: {path_txt_file}")  
    else:  
        print("在指定目录下未找到串口打印文件。")  
else:  
    print("[RV] 路径文件path.txt已存在且包含info_path字段，无需重新输入目录。")