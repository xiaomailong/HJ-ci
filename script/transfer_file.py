# !  /usr/local/bin/python3.3

import os
import shutil
# 该文件将本程序的所有文件上传至服务器上

class Transfer:
    # 服务器的地址可以是多个
    remote_dirs = []

    def __init__(self,local_dir):
        self.local_dir = local_dir
    """
    将文件从from_path传输到to_path当中
    """
    def src_transfer(self):
        for remote_dir in self.remote_dirs:
            shutil.copytree(self.local_dir + '/src',remote_dir + '/src' )
        return
    """
    该文件将目录下的shell脚本传输上去
    """
    def shell_transfer(self):
        list_file = os.listdir(self.local_dir)
        suffix = self.endWith('.sh')
        shells = filter(suffix,list_file)

        for remote_dir in self.remote_dirs:
            for shell_file in shells:
                shutil.copy(self.local_dir + '/' + shell_file,remote_dir + '/' + shell_file)
        return
    """
    根据后缀过滤文件
    """
    def endWith(self,*endstring):
        ends = endstring
        def run(s):
            f = map(s.endswith,ends)
            if True in f: return s
        return run
    """
    添加添加要上传的服务器目录
    """
    def add_remote_dir(self,remote_dir):
        self.remote_dirs.append(remote_dir)

        return

if __name__ == '__main__':
    transfer = Transfer(".")
    #transfer.add_remote_dir('/hj03/home/zhangys/platformsoft/01primary')
    transfer.add_remote_dir('/hj03/home/zhangys/platformsoft/01slave')
    # 在这里有意的将传输源文件放在前面是因为copytree可以创建目录，而copy不可以，这样可以避免出错
    transfer.src_transfer()
    transfer.shell_transfer()
