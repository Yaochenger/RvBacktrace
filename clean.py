import os  
import shutil  
  
# 获取当前脚本的目录  
current_dir = os.path.dirname(os.path.abspath(__file__))  
  
# 构造tools文件夹的路径  
tools_dir = os.path.join(current_dir, 'tools')  
  
# 构造html和obj文件夹的路径  
html_dir = os.path.join(tools_dir, 'html')  
obj_dir = os.path.join(tools_dir, 'obj')  
  
# 检查这些文件夹是否存在，如果存在则删除  
if os.path.exists(html_dir):  
    shutil.rmtree(html_dir)  
    print(f"Deleted {html_dir}")  
else:  
    print(f"{html_dir} does not exist.")  
  
if os.path.exists(obj_dir):  
    shutil.rmtree(obj_dir)  
    print(f"Deleted {obj_dir}")  
else:  
    print(f"{obj_dir} does not exist.")  
  
# 如果tools文件夹为空（可选），你也可以选择删除它  
# 注意：这将在html和obj文件夹被删除且tools内无其他文件时执行  
if os.path.exists(tools_dir) and not os.listdir(tools_dir):  
    shutil.rmtree(tools_dir)  
    print(f"Deleted {tools_dir} because it was empty.")  
else:  
    print(f"{tools_dir} still contains files or is not empty.")