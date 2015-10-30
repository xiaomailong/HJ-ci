# /bin/bash

# set -x

# 该脚本实现对目录下的shell脚本进行传输

declare -a remote_dirs
remote_dirs[0]='/hj03/home/zhangys/platformsoft/01slave'
remote_dirs[1]='/hj04/home/zhangys/platformsoft/02primary'
remote_dirs[2]='/hj04/home/zhangys/platformsoft/02slave'

for shell_file in $(ls *.sh)
do
    for remote_dir in ${remote_dirs[*]}
    do
        echo $shell_file "copy to->"$remote_dir/$shell_file
        cp $shell_file $remote_dir/$shell_file
        if [ $? -ne 0 ]
        then
            echo "   copy failed"
        fi
    done
done


echo "transfer finished"
