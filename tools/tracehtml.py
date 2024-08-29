import re  
import os  
  
def generate_html_table(input_file_relative, output_filename):  
    # 获取当前脚本所在的目录  
    script_dir = os.path.dirname(os.path.abspath(__file__))  
      
    # 构建完整的input_file路径  
    input_file = os.path.join(script_dir, 'obj', input_file_relative)  
      
    # 构建完整的输出文件路径，包括html文件夹  
    output_dir = os.path.join(script_dir, 'html')  
    if not os.path.exists(output_dir):  
        os.makedirs(output_dir)  # 如果文件夹不存在，则创建它  
    output_file = os.path.join(output_dir, output_filename)  
      
    with open(input_file, 'r') as f:  
        lines = f.readlines()  
      
    table_data = []  
    for line in lines: 
        print(11) 
        match = re.match(r'\s*([0-9a-fA-F]+):\s+([0-9a-fA-F]+)\s+(.*?)<([^>]+)>', line) 
        if match:  
            address = match.group(1).strip()
            print(address)  
            instruction = match.group(2).strip()
            print(instruction)   
            asm = match.group(3).strip() 
            print(asm)   
            function_name = match.group(4).strip()
            print(function_name)   
            table_data.append((address, instruction, asm, function_name))  
      
    # 构建函数调用栈字符串  
    call_stack = " <- ".join(data[3] for data in table_data)  
      
    html = """  
    <!DOCTYPE html>  
    <html>  
    <head>  
        <title>RVBacktrace</title>  
        <style>  
            table {border-collapse: collapse; width: 100%;}  
            th, td {border: 1px solid black; padding: 8px; text-align: left;}  
            th {background-color: #f2f2f2;}  
        </style>  
    </head>  
    <body>  
        <h2>RVBacktrace/栈回溯信息</h2>  
        <table>  
            <tr>  
                <th>函数地址</th>  
                <th>指令编码</th>  
                <th>汇编</th>  
                <th>函数名称</th>  
            </tr>  
    """  
      
    for data in table_data:  
        html += f"<tr><td>{data[0]}</td><td>{data[1]}</td><td>{data[2]}</td><td>{data[3]}</td></tr>"  
      
    html += """  
        </table>  
        <h2>函数调用栈</h2>  
        <table>  
            <tr>  
                <th>调用栈</th>  
            </tr>  
            <tr>  
                <td>{call_stack}</td>  
            </tr>  
        </table>  
    </body>  
    </html>  
    """.format(call_stack=call_stack)  
      
    with open(output_file, 'w') as f:  
        f.write(html)  
  
# 调用函数，这里只传递文件名（相对于obj文件夹），而不是完整路径  
generate_html_table("info.txt", "rvbacktrace.html")