import os
"""
根据后缀过滤文件
"""
def endWith(*endstring):
    ends = endstring
    def run(s):
        f = map(s.endswith,ends)
        if True in f: return s
    return run

list_file = os.listdir('.')
suffix = endWith('.sh','.py')
shells = filter(suffix,list_file)

shells = [i for i in shells]
print(shells)
