#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 input_file.c output_file_name"
    exit 1
fi

# 获取输入参数
input_file=$1
output_file=$2
c_ext=".c"
o_ext=".o"

# 执行编译命令（gcc）
lex -t "$input_file" > "$output_file$c_ext"
cc -c -o "$output_file$o_ext" "$output_file$c_ext"
cc -o "$output_file" "$output_file$o_ext" -lfl
./"$output_file"

# 检查编译结果是否成功
if [ $? -eq 0 ]; then
    echo "Compilation successful!"
else
    echo "Compilation failed!"
fi
