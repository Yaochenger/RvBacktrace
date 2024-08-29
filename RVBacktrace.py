import subprocess  
import os  
  
# 定义tools目录的路径（相对于当前脚本）  
tools_dir = './tools'  
  
# 构造html目录的路径  
html_dir = os.path.join(tools_dir, 'html')  
  
# 假设html目录下有一个名为index.html的文件  
html_file = os.path.join(html_dir, 'rvbacktrace.html')  
  
# 确保html目录存在，如果脚本需要创建该目录或文件，可以在这里添加逻辑  
# 例如：os.makedirs(html_dir, exist_ok=True)  
  
# 定义要执行的脚本列表  
scripts = ['tracepath.py', 'tracefunction.py', 'traceinfo.py', 'tracehtml.py']  
  
# 遍历脚本列表，并调用它们  
for script in scripts:  
    full_path = os.path.join(tools_dir, script)  
    subprocess.run(['python', full_path], check=True)  
  
# 尝试在Windows上打开HTML文件  
# 使用start命令，并设置shell=True来在shell中执行它  
try:  
    subprocess.run(['start', html_file], shell=True, check=True)  
except subprocess.CalledProcessError:  
    # 如果start命令失败（虽然这种情况很少见，除非文件不存在或没有关联的浏览器）  
    print(f"Failed to open {html_file} with the default browser.")  
  
# 注意：使用shell=True时要非常小心，因为它可能会使你的代码容易受到shell注入攻击  
# 如果可能的话，最好避免使用shell=True，但在这种情况下，它是打开文件的必要方式