#!/usr/bin/python3.3
# 注意，请使用python3以上版本执行本程序

"""
这个脚本将根据ssi文件生成device_name.c文件
"""

class DeviceNameGen:
    """
    构造函数传入文件路径
    """
    def __init__(self,file_path):
        self.file_path = file_path
        self.file_object = open(self.file_path)
        self.device_names = []
        return
    """
    对文件进行解析
    """
    def parse(self):
        try:
            for line in self.file_object:
                #以空格分割
                self.device_names.append(line.split())
        finally:
            self.file_object.close()
        return
    """
    调用该函数生成相应代码
    """
    def gen(self):
        print(self.device_names)

        device_source_file = open("device_name.c","w+")
        header = r"""
/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 由python脚本程序自动生成
版本        : 1.0
创建日期    : 2013年12月9日 16:32:41
用途        : 
历史修改记录: v1.0    创建
**********************************************************************/

/*设备名称存放数组*/
const char* device_name[TOTAL_NAMES] = {
"""
        footer = r"};"

        device_source_file.write(header)
        for name in self.device_names:
            device_source_file.write("    \"" + name[1] + "\",\n")

        device_source_file.write(footer)

        return


if __name__ == "__main__":
    file_path = './bzz.ssi'
    device_name_gen = DeviceNameGen(file_path)
    device_name_gen.parse()
    device_name_gen.gen()

