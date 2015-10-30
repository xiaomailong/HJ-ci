#!/usr/bin/python3
#-*- coding: utf-8 -*-

import sys
import codecs
import re

def get_timer_interval(file_name):
    #file_log = codecs.open(file_name,"r",'utf-8')
    file_log = open(file_name,"r")
    last_time = 0
    count = 0
    total = 0
    min_time = 0xffffffffff
    max_time = 0
    for line in file_log:
        if line[-10:-1] != "cpu_timer":
            continue
        else:
            this_time = float(line[1:18]) * (10 ** 6)
            this_time = int(this_time)
            if last_time != 0:
                t_time = this_time - last_time
                print(t_time / 1000,end = ",")
                total += t_time
                if min_time > t_time:
                    min_time = t_time
                if max_time < t_time:
                    max_time = t_time
            last_time = this_time
            count += 1
    print("")
    print("total:",total)
    print("count:",count)
    print("max:",max_time / 1000)
    print("min:",min_time / 1000)
    print("average:",total / (count * 1000),"ms")
    return

def get_interval(pattern_start,pattern_end,file_name):
    #file_log = codecs.open(file_name,"r",'utf-8')
    file_log = open(file_name,"r")
    last_time = 0
    count = 0
    total = 0
    min_time = 0xffffffffff
    max_time = 0
    regex_start = re.compile(pattern_start)
    regex_end = re.compile(pattern_end)
    start_time = 0
    for line in file_log:
        match_start = regex_start.search(line)
        if match_start:
            t_time = float(line[1:18]) * (10 ** 6)
            start_time = int(t_time)
            continue
        match_end = regex_end.search(line)
        if match_end:
            if start_time == 0:
                print("error")
                continue
            t_time = float(line[1:18]) * (10 ** 6)
            end_time = int(t_time)
            interval_time = end_time - start_time
            print(interval_time / 1000,end = ",")
            total += interval_time
            if min_time > interval_time:
                min_time = interval_time
            if max_time < interval_time:
                max_time = interval_time
            start_time = 0
            count += 1
    print("")
    print("total:",total)
    print("count:",count)
    print("max:",max_time / 1000)
    print("min:",min_time / 1000)
    print("average:",total / (count * 1000),"ms")

    return

def get_fsm_cycle(pattern,file_name):
    #file_log = codecs.open(file_name,"r",'utf-8')
    #file_log = codecs.open(file_name,"r",'utf-8-sig')
    file_log = open(file_name,"r")
    last_time = 0
    count = 0
    total = 0
    min_time = 0xffffffffff
    max_time = 0
    regex = re.compile(pattern)
    start_time = 0
    for line in file_log:
        match = regex.search(line)
        if match:
            t_time = float(line[1:18]) * (10 ** 6)
            if start_time == 0:
                start_time = int(t_time)
            else:
                end_time = int(t_time)
                interval_time = end_time - start_time
                print(interval_time / 1000,end = ",")
                total += interval_time
                if min_time > interval_time:
                    min_time = interval_time
                if max_time < interval_time:
                    max_time = interval_time
                start_time = 0
                count += 1
    print("")
    print("total:",total)
    print("count:",count)
    print("max:",max_time / 1000)
    print("min:",min_time / 1000)
    print("average:",total / (count * 1000),"ms")
    return

def get_dpram_write_interval(pattern_start,pattern_end,file_name):
    #file_log = codecs.open(file_name,"r",'utf-8')
    #file_log = codecs.open(file_name,"r",'utf-8-sig')
    file_log = open(file_name,"r")
    last_time = 0
    count = 0
    total = 0
    min_time = 0xffffffffff
    max_time = 0
    regex_start = re.compile(pattern_start)
    regex_end = re.compile(pattern_end)
    start_time = 0
    for line in file_log:
        match_start = regex_start.search(line)
        if match_start:
            t_time = float(line[34:45]) * (10 ** 6)
            start_time = int(t_time)
            continue
        match_end = regex_end.search(line)
        if match_end:
            if start_time == 0:
                print("error")
                continue
            t_time = float(line[34:45]) * (10 ** 6)
            end_time = int(t_time)
            interval_time = end_time - start_time
            #print(interval_time / 1000,end = ",")
            total += interval_time
            if min_time > interval_time:
                min_time = interval_time
            if max_time < interval_time:
                max_time = interval_time
            start_time = 0
            count += 1
    #print("")
    #print("total:",total)
    #print("count:",count)
    #print("max:",max_time / 1000)
    #print("min:",min_time / 1000)
    if count != 0:
        average = total / (count * 1000)
    else:
        average = 0
    #print("average:",average,"ms")

    return average

def print_dpram_write_all_interval(file_name):
    for i in range(1,0xffff,0xff):
        pattern_start = "%#x:begin" % i
        #print(pattern_start)
        pattern_end = "%#x:end" % i
        average = get_dpram_write_interval(pattern_start,pattern_end,file_name)
        print(average,end = ',')
    return

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage:./parser_log.py log_file_name")
        quit()
    file_name = sys.argv[1]
    get_timer_interval(file_name)

    # series input interval
    #get_interval("双系正在发送输入同步数据$","双系发送输入同步数据成功$",file_name)

    # cpu input interval
    #get_interval("双CPU正在发送输入同步数据$","双CPU发送输入同步数据成功$",file_name)

    # cpu recv input interval
    #get_interval("双CPU开始接收输入同步数据$","双CPU接收输入数据成功$",file_name)

    # seris new cycle interval
    #get_interval("双系正在发送新周期同步数据$","双系发送新周期同步数据成功$",file_name)

    # cpu new cycle interval
    #get_interval("双CPU正在发送新周期同步数据$","双CPU发送新周期数据成功$",file_name)

    # series result interval
    # get_interval("双系正在发送结果数据$","双系发送结果数据成功$",file_name)

    # cpu send result interval
    #get_interval("双CPU正在发送结果同步数据$","双CPU发送结果同步数据成功$",file_name)

    # cpu recv result interval
    #get_interval("双CPU开始接收结果同步数据$","双CPU接收结果同步数据成功$",file_name)

    # recv eeu interval
    #get_interval("正在尝试从电子单元A口接收数据","电子模块A口接收数据失败",file_name)

    # fsm interval
    #get_interval("fsm_begin_new_cycle$","fsm_terminal_state$",file_name)

    # get fsm cycle
    #get_fsm_cycle("fsm_begin_new_cycle$",file_name)

    # spare series recv input
    #get_interval("开始接收主系输入同步数据$","接收双系输入数据成功$",file_name)

    #get_interval("copy begin$","copy end$",file_name)

    #print_dpram_write_all_interval(file_name)

    # for test
    #get_interval("begin","end",file_name)


