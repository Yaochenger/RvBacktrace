import subprocess  
import os  
import time  
  
def check_file_modified(file_path, last_modified_time):  
    """检查文件是否自上次检查后被修改过"""  
    if not os.path.exists(file_path):  
        return False  # 如果文件不存在，则认为没有被修改  
    current_modified_time = os.path.getmtime(file_path)  
    return current_modified_time != last_modified_time  
  
def read_info_path_from_file(file_path):  
    """从文件中读取info_path字段的值，并尝试将其转换为绝对路径（如果可能）"""  
    try:  
        with open(file_path, 'r') as file:  
            for line in file:  
                if line.startswith('info_path ='):  
                    value = line.split('=', 1)[1].strip()  
                    # 去除字符串两端的引号（如果有）  
                    cleaned_value = value.strip("'\"")  
                    # 尝试将路径转换为绝对路径（如果它是相对路径）  
                    return os.path.abspath(cleaned_value) if not os.path.isabs(cleaned_value) else cleaned_value  
        return None  # 如果没有找到info_path，则返回None  
    except FileNotFoundError:  
        print(f"文件 {file_path} 未找到。")  
        return None  
  
def execute_scripts(tools_dir):  
    """执行指定目录下的脚本"""  
    scripts = ['tracepath.py', 'tracefunction.py', 'traceinfo.py', 'tracehtml.py']  
    for script in scripts:  
        full_path = os.path.join(tools_dir, script)  
        subprocess.run(['python', full_path], check=True)  
  
def open_html_file(html_file):  
    """尝试在默认浏览器中打开HTML文件"""  
    try:  
        subprocess.run(['start', html_file], shell=True, check=True)  
    except subprocess.CalledProcessError:  
        print(f"Failed to open {html_file} with the default browser.")  
  
def main():  
    script_dir = os.path.dirname(os.path.abspath(__file__))  # 获取当前脚本的目录  
    tools_dir = os.path.join(script_dir, 'tools')  
    html_dir = os.path.join(tools_dir, 'html')  
    html_file = os.path.join(html_dir, 'rvbacktrace.html')  
  
    # 确保html目录存在  
    os.makedirs(html_dir, exist_ok=True)  
  
    # 执行脚本  
    execute_scripts(tools_dir)  
  
    # 尝试打开HTML文件  
    open_html_file(html_file)  
  
    # 读取info_path并监控文件更改  
    path_txt_file = os.path.join(script_dir, 'tools', 'obj', 'path.txt')  
    info_path = read_info_path_from_file(path_txt_file) 
    print("\n[RV] 修改栈回溯信息文本后，脚本将自动重新生成栈回溯HTML文件。")  
    print("--输入Ctrl+C退出脚本--\n") 
    if info_path:  
        last_modified_time = os.path.getmtime(info_path)  
  
        while True:  
            time.sleep(1)  # 每秒检查一次  
  
            if check_file_modified(info_path, last_modified_time):  
                print(f"[RV] 栈回溯文件 {info_path} 已被修改。")  
                last_modified_time = os.path.getmtime(info_path)  
                print("[RV] 重新生成栈回溯HTML文件...")  
                main()  # 递归调用main()，但请注意潜在的堆栈溢出  
                break  # 如果不希望无限递归，可以在检测到更改后退出循环（但这将停止监控）  
  
    else:  
        print("无法从文件中读取info_path或文件不存在。")  
  
if __name__ == "__main__":  
    try:  
        main()  
    except RecursionError:  
        print("发生递归错误，可能是由于文件被频繁修改导致的无限递归调用。")  
    except Exception as e:  
        print(f"发生错误：{e}")